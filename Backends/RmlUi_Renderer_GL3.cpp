/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "RmlUi_Renderer_GL3.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/GeometryUtilities.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <string.h>

#if defined(RMLUI_PLATFORM_WIN32) && !defined(__MINGW32__)
	// function call missing argument list
	#pragma warning(disable : 4551)
	// unreferenced local function has been removed
	#pragma warning(disable : 4505)
#endif

#if defined RMLUI_PLATFORM_EMSCRIPTEN
	#define RMLUI_SHADER_HEADER_VERSION "#version 300 es\nprecision highp float;\n"
	#include <GLES3/gl3.h>
#elif defined RMLUI_GL3_CUSTOM_LOADER
	#define RMLUI_SHADER_HEADER_VERSION "#version 330\n"
	#include RMLUI_GL3_CUSTOM_LOADER
#else
	#define RMLUI_SHADER_HEADER_VERSION "#version 330\n"
	#define GLAD_GL_IMPLEMENTATION
	#include "RmlUi_Include_GL3.h"
#endif

// Determines the anti-aliasing quality when creating layers. Enables better-looking visuals, especially when transforms are applied.
static constexpr int NUM_MSAA_SAMPLES = 2;

#define RMLUI_PREMULTIPLIED_ALPHA 1
#define BLUR_SIZE 7
#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

#define RMLUI_STRINGIFY_IMPL(x) #x
#define RMLUI_STRINGIFY(x) RMLUI_STRINGIFY_IMPL(x)

#define RMLUI_SHADER_HEADER     \
	RMLUI_SHADER_HEADER_VERSION \
	"#define RMLUI_PREMULTIPLIED_ALPHA " RMLUI_STRINGIFY(RMLUI_PREMULTIPLIED_ALPHA) "\n"

static const char* shader_vert_main = RMLUI_SHADER_HEADER R"(
uniform vec2 _translate;
uniform mat4 _transform;

in vec2 inPosition;
in vec4 inColor0;
in vec2 inTexCoord0;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
	fragTexCoord = inTexCoord0;
	fragColor = inColor0;

#if RMLUI_PREMULTIPLIED_ALPHA
	// Pre-multiply vertex colors with their alpha.
	fragColor.rgb = fragColor.rgb * fragColor.a;
#endif

	vec2 translatedPos = inPosition + _translate;
	vec4 outPos = _transform * vec4(translatedPos, 0.0, 1.0);

    gl_Position = outPos;
}
)";
static const char* shader_frag_texture = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	vec4 texColor = texture(_tex, fragTexCoord);
	finalColor = fragColor * texColor;
}
)";
static const char* shader_frag_color = RMLUI_SHADER_HEADER R"(
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	finalColor = fragColor;
}
)";

static const char* shader_vert_passthrough = RMLUI_SHADER_HEADER R"(
in vec2 inPosition;
in vec2 inTexCoord0;

out vec2 fragTexCoord;

void main() {
	fragTexCoord = inTexCoord0;
    gl_Position = vec4(inPosition, 0.0, 1.0);
}
)";
static const char* shader_frag_passthrough = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
	finalColor = texture(_tex, fragTexCoord);
}
)";
static const char* shader_frag_color_matrix = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
uniform mat4 _color_matrix;

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
	vec4 texColor = texture(_tex, fragTexCoord);
	finalColor = _color_matrix * texColor;
}
)";

#define RMLUI_SHADER_BLUR_HEADER \
	RMLUI_SHADER_HEADER "\n#define BLUR_SIZE " RMLUI_STRINGIFY(BLUR_SIZE) "\n#define BLUR_NUM_WEIGHTS " RMLUI_STRINGIFY(BLUR_NUM_WEIGHTS)

static const char* shader_vert_blur = RMLUI_SHADER_BLUR_HEADER R"(
uniform vec2 _texelOffset;

in vec3 inPosition;
in vec2 inTexCoord0;

out vec2 fragTexCoord[BLUR_SIZE];

void main() {
	for(int i = 0; i < BLUR_SIZE; i++)
		fragTexCoord[i] = inTexCoord0 - float(i - BLUR_NUM_WEIGHTS + 1) * _texelOffset;
    gl_Position = vec4(inPosition, 1.0);
}
)";
static const char* shader_frag_blur = RMLUI_SHADER_BLUR_HEADER R"(
uniform sampler2D _tex;
uniform float _weights[BLUR_NUM_WEIGHTS];
uniform vec2 _texCoordMin;
uniform vec2 _texCoordMax;

in vec2 fragTexCoord[BLUR_SIZE];
out vec4 finalColor;

void main() {    
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	for(int i = 0; i < BLUR_SIZE; i++)
		color += texture(_tex, clamp(fragTexCoord[i], _texCoordMin, _texCoordMax)) * _weights[abs(i - BLUR_NUM_WEIGHTS + 1)];
	finalColor = color;
}
)";

enum class ProgramId {
	None,
	Color,
	Texture,
	Passthrough,
	ColorMatrix,
	Blur,
	Count,
};
enum class VertShaderId {
	Main,
	Passthrough,
	Blur,
	Count,
};
enum class FragShaderId {
	Color,
	Texture,
	Passthrough,
	ColorMatrix,
	Blur,
	Count,
};
enum class UniformId {
	Translate,
	Transform,
	Tex,
	ColorMatrix,
	TexelOffset,
	TexCoordMin,
	TexCoordMax,
	Weights,
	Count,
};

namespace Gfx {

static const char* const program_uniform_names[(size_t)UniformId::Count] = {"_translate", "_transform", "_tex", "_color_matrix", "_texelOffset",
	"_texCoordMin", "_texCoordMax", "_weights[0]"};

enum class VertexAttribute { Position, Color0, TexCoord0, Count };
static const char* const vertex_attribute_names[(size_t)VertexAttribute::Count] = {"inPosition", "inColor0", "inTexCoord0"};

struct VertShaderDefinition {
	VertShaderId id;
	const char* name_str;
	const char* code_str;
};
struct FragShaderDefinition {
	FragShaderId id;
	const char* name_str;
	const char* code_str;
};
struct ProgramDefinition {
	ProgramId id;
	const char* name_str;
	VertShaderId vert_shader;
	FragShaderId frag_shader;
};

// clang-format off
static const VertShaderDefinition vert_shader_definitions[] = {
	{VertShaderId::Main,        "main",         shader_vert_main},
	{VertShaderId::Passthrough, "passthrough",  shader_vert_passthrough},
	{VertShaderId::Blur,        "blur",         shader_vert_blur},
};
static const FragShaderDefinition frag_shader_definitions[] = {
	{FragShaderId::Color,       "color",        shader_frag_color},
	{FragShaderId::Texture,     "texture",      shader_frag_texture},
	{FragShaderId::Passthrough, "passthrough",  shader_frag_passthrough},
	{FragShaderId::ColorMatrix, "color_matrix", shader_frag_color_matrix},
	{FragShaderId::Blur,        "blur",         shader_frag_blur},
};
static const ProgramDefinition program_definitions[] = {
	{ProgramId::Color,       "color",        VertShaderId::Main,        FragShaderId::Color},
	{ProgramId::Texture,     "texture",      VertShaderId::Main,        FragShaderId::Texture},
	{ProgramId::Passthrough, "passthrough",  VertShaderId::Passthrough, FragShaderId::Passthrough},
	{ProgramId::ColorMatrix, "color_matrix", VertShaderId::Passthrough, FragShaderId::ColorMatrix},
	{ProgramId::Blur,        "blur",         VertShaderId::Blur,        FragShaderId::Blur},
};
// clang-format on

template <typename T, typename Enum>
class EnumArray {
public:
	const T& operator[](Enum id) const
	{
		RMLUI_ASSERT((size_t)id < (size_t)Enum::Count);
		return ids[size_t(id)];
	}
	T& operator[](Enum id)
	{
		RMLUI_ASSERT((size_t)id < (size_t)Enum::Count);
		return ids[size_t(id)];
	}
	auto begin() const { return ids.begin(); }
	auto end() const { return ids.end(); }

private:
	Rml::Array<T, (size_t)Enum::Count> ids = {};
};

using Programs = EnumArray<GLuint, ProgramId>;
using VertShaders = EnumArray<GLuint, VertShaderId>;
using FragShaders = EnumArray<GLuint, FragShaderId>;

class Uniforms {
public:
	GLint Get(ProgramId id, UniformId uniform) const
	{
		auto it = map.find(ToKey(id, uniform));
		if (it != map.end())
			return it->second;
		return -1;
	}
	void Insert(ProgramId id, UniformId uniform, GLint location) { map[ToKey(id, uniform)] = location; }

private:
	using Key = std::uint64_t;
	Key ToKey(ProgramId id, UniformId uniform) const { return (static_cast<Key>(id) << 32) | static_cast<Key>(uniform); }
	Rml::UnorderedMap<Key, GLint> map;
};

struct ProgramData {
	Programs programs;
	VertShaders vert_shaders;
	FragShaders frag_shaders;
	Uniforms uniforms;
};

struct CompiledGeometryData {
	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	GLsizei draw_count;
};

struct FramebufferData {
	int width, height;
	GLuint framebuffer;
	GLuint color_tex_buffer;
	GLuint color_render_buffer;
	GLuint depth_stencil_buffer;
	bool owns_depth_stencil_buffer;
};

enum class FramebufferAttachment { None, Depth, DepthStencil };

static void CheckGLError(const char* operation_name)
{
#ifdef RMLUI_DEBUG
	GLenum error_code = glGetError();
	if (error_code != GL_NO_ERROR)
	{
		static const Rml::Pair<GLenum, const char*> error_names[] = {{GL_INVALID_ENUM, "GL_INVALID_ENUM"}, {GL_INVALID_VALUE, "GL_INVALID_VALUE"},
			{GL_INVALID_OPERATION, "GL_INVALID_OPERATION"}, {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"}};
		const char* error_str = "''";
		for (auto& err : error_names)
		{
			if (err.first == error_code)
			{
				error_str = err.second;
				break;
			}
		}
		Rml::Log::Message(Rml::Log::LT_ERROR, "OpenGL error during %s. Error code 0x%x (%s).", operation_name, error_code, error_str);
	}
#endif
	(void)operation_name;
}

// Create the shader, 'shader_type' is either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
static bool CreateShader(GLuint& out_shader_id, GLenum shader_type, const char* code_string)
{
	RMLUI_ASSERT(shader_type == GL_VERTEX_SHADER || shader_type == GL_FRAGMENT_SHADER);

	GLuint id = glCreateShader(shader_type);
	glShaderSource(id, 1, (const GLchar**)&code_string, NULL);
	glCompileShader(id);

	GLint status = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint info_log_length = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
		char* info_log_string = new char[info_log_length + 1];
		glGetShaderInfoLog(id, info_log_length, NULL, info_log_string);

		Rml::Log::Message(Rml::Log::LT_ERROR, "Compile failure in OpenGL shader: %s", info_log_string);
		delete[] info_log_string;
		glDeleteShader(id);
		return false;
	}

	CheckGLError("CreateShader");

	out_shader_id = id;
	return true;
}

static bool CreateProgram(GLuint& out_program, Uniforms& inout_uniform_map, ProgramId program_id, GLuint vertex_shader, GLuint fragment_shader)
{
	GLuint id = glCreateProgram();
	RMLUI_ASSERT(id);

	for (GLuint i = 0; i < (GLuint)VertexAttribute::Count; i++)
		glBindAttribLocation(id, i, vertex_attribute_names[i]);

	CheckGLError("BindAttribLocations");

	glAttachShader(id, vertex_shader);
	glAttachShader(id, fragment_shader);

	glLinkProgram(id);

	glDetachShader(id, vertex_shader);
	glDetachShader(id, fragment_shader);

	GLint status = 0;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint info_log_length = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
		char* info_log_string = new char[info_log_length + 1];
		glGetProgramInfoLog(id, info_log_length, NULL, info_log_string);

		Rml::Log::Message(Rml::Log::LT_ERROR, "OpenGL program linking failure: %s", info_log_string);
		delete[] info_log_string;
		glDeleteProgram(id);
		return false;
	}

	out_program = id;

	// Make a lookup table for the uniform locations.
	GLint num_active_uniforms = 0;
	glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &num_active_uniforms);

	constexpr size_t name_size = 64;
	GLchar name_buf[name_size] = "";
	for (int unif = 0; unif < num_active_uniforms; ++unif)
	{
		GLint array_size = 0;
		GLenum type = 0;
		GLsizei actual_length = 0;
		glGetActiveUniform(id, unif, name_size, &actual_length, &array_size, &type, name_buf);
		GLint location = glGetUniformLocation(id, name_buf);

		// See if we have the name in our pre-defined name list.
		UniformId program_uniform = UniformId::Count;
		for (int i = 0; i < (int)UniformId::Count; i++)
		{
			const char* uniform_name = program_uniform_names[i];
			if (strcmp(name_buf, uniform_name) == 0)
			{
				program_uniform = (UniformId)i;
				break;
			}
		}

		if ((size_t)program_uniform < (size_t)UniformId::Count)
		{
			inout_uniform_map.Insert(program_id, program_uniform, location);
		}
		else
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "OpenGL program uses unknown uniform '%s'.", name_buf);
			return false;
		}
	}

	CheckGLError("CreateProgram");

	return true;
}

static bool CreateFramebuffer(FramebufferData& out_fb, int width, int height, int samples, FramebufferAttachment attachment,
	GLuint shared_depth_stencil_buffer)
{
#ifdef RMLUI_PLATFORM_EMSCRIPTEN
	constexpr GLint wrap_mode = GL_CLAMP_TO_EDGE;
#else
	constexpr GLint wrap_mode = GL_CLAMP_TO_BORDER; // GL_REPEAT GL_MIRRORED_REPEAT GL_CLAMP_TO_EDGE
#endif

	constexpr GLenum color_format = GL_RGBA8;   // GL_RGBA8 GL_SRGB8_ALPHA8 GL_RGBA16F
	constexpr GLint min_mag_filter = GL_LINEAR; // GL_NEAREST
	const Rml::Colourf border_color(0.f, 0.f);

	GLuint framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	GLuint color_tex_buffer = 0;
	GLuint color_render_buffer = 0;
	if (samples > 0)
	{
		glGenRenderbuffers(1, &color_render_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, color_render_buffer);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, color_format, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_render_buffer);
	}
	else
	{
		glGenTextures(1, &color_tex_buffer);
		glBindTexture(GL_TEXTURE_2D, color_tex_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, color_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, min_mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);
#ifndef RMLUI_PLATFORM_EMSCRIPTEN
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);
#endif

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex_buffer, 0);
	}

	// Create depth/stencil buffer storage attachment.
	GLuint depth_stencil_buffer = 0;
	if (attachment != FramebufferAttachment::None)
	{
		if (shared_depth_stencil_buffer)
		{
			// Share depth/stencil buffer
			depth_stencil_buffer = shared_depth_stencil_buffer;
		}
		else
		{
			// Create new depth/stencil buffer
			glGenRenderbuffers(1, &depth_stencil_buffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_buffer);

			const GLenum internal_format = (attachment == FramebufferAttachment::DepthStencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internal_format, width, height);
		}

		const GLenum attachment_type = (attachment == FramebufferAttachment::DepthStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment_type, GL_RENDERBUFFER, depth_stencil_buffer);
	}

	const GLuint framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "OpenGL framebuffer could not be generated. Error code %x.", framebuffer_status);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	CheckGLError("CreateFramebuffer");

	out_fb = {};
	out_fb.width = width;
	out_fb.height = height;
	out_fb.framebuffer = framebuffer;
	out_fb.color_tex_buffer = color_tex_buffer;
	out_fb.color_render_buffer = color_render_buffer;
	out_fb.depth_stencil_buffer = depth_stencil_buffer;
	out_fb.owns_depth_stencil_buffer = !shared_depth_stencil_buffer;

	return true;
}

static void DestroyFramebuffer(FramebufferData& fb)
{
	if (fb.framebuffer)
		glDeleteFramebuffers(1, &fb.framebuffer);
	if (fb.color_tex_buffer)
		glDeleteTextures(1, &fb.color_tex_buffer);
	if (fb.color_render_buffer)
		glDeleteRenderbuffers(1, &fb.color_render_buffer);
	if (fb.owns_depth_stencil_buffer && fb.depth_stencil_buffer)
		glDeleteRenderbuffers(1, &fb.depth_stencil_buffer);
	fb = {};
}

static void BindTexture(const FramebufferData& fb)
{
	if (!fb.color_tex_buffer)
	{
		RMLUI_ERRORMSG("Only framebuffers with color textures can be bound as textures. This framebuffer probably uses multisampling which needs a "
					   "blit step first.");
	}

	glBindTexture(GL_TEXTURE_2D, fb.color_tex_buffer);
}

static bool CreateShaders(ProgramData& data)
{
	RMLUI_ASSERT(std::all_of(data.vert_shaders.begin(), data.vert_shaders.end(), [](auto&& value) { return value == 0; }));
	RMLUI_ASSERT(std::all_of(data.frag_shaders.begin(), data.frag_shaders.end(), [](auto&& value) { return value == 0; }));
	RMLUI_ASSERT(std::all_of(data.programs.begin(), data.programs.end(), [](auto&& value) { return value == 0; }));
	auto ReportError = [](const char* type, const char* name) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not create OpenGL %s: '%s'.", type, name);
		return false;
	};

	for (const VertShaderDefinition& def : vert_shader_definitions)
	{
		if (!CreateShader(data.vert_shaders[def.id], GL_VERTEX_SHADER, def.code_str))
			return ReportError("vertex shader", def.name_str);
	}

	for (const FragShaderDefinition& def : frag_shader_definitions)
	{
		if (!CreateShader(data.frag_shaders[def.id], GL_FRAGMENT_SHADER, def.code_str))
			return ReportError("fragment shader", def.name_str);
	}

	for (const ProgramDefinition& def : program_definitions)
	{
		if (!CreateProgram(data.programs[def.id], data.uniforms, def.id, data.vert_shaders[def.vert_shader], data.frag_shaders[def.frag_shader]))
			return ReportError("program", def.name_str);
	}

	glUseProgram(0);

	return true;
}

static void DestroyShaders(const ProgramData& data)
{
	for (GLuint id : data.programs)
		glDeleteProgram(id);

	for (GLuint id : data.vert_shaders)
		glDeleteShader(id);

	for (GLuint id : data.frag_shaders)
		glDeleteShader(id);
}

} // namespace Gfx

RenderInterface_GL3::RenderInterface_GL3()
{
	auto mut_program_data = Rml::MakeUnique<Gfx::ProgramData>();
	if (Gfx::CreateShaders(*mut_program_data))
	{
		program_data = std::move(mut_program_data);

		Rml::Vertex vertices[4];
		int indices[6];
		Rml::GeometryUtilities::GenerateQuad(vertices, indices, Rml::Vector2f(-1), Rml::Vector2f(2), {});
		fullscreen_quad_geometry = RenderInterface_GL3::CompileGeometry(vertices, 4, indices, 6);
	}
}

RenderInterface_GL3::~RenderInterface_GL3()
{
	if (fullscreen_quad_geometry)
	{
		RenderInterface_GL3::ReleaseCompiledGeometry(fullscreen_quad_geometry);
		fullscreen_quad_geometry = {};
	}

	if (program_data)
	{
		Gfx::DestroyShaders(*program_data);
		program_data.reset();
	}
}

void RenderInterface_GL3::SetViewport(int width, int height)
{
	viewport_width = width;
	viewport_height = height;
	projection = Rml::Matrix4f::ProjectOrtho(0, (float)viewport_width, (float)viewport_height, 0, -10000, 10000);
}

void RenderInterface_GL3::BeginFrame()
{
	RMLUI_ASSERT(viewport_width >= 0 && viewport_height >= 0);

	// Backup GL state.
	glstate_backup.enable_cull_face = glIsEnabled(GL_CULL_FACE);
	glstate_backup.enable_blend = glIsEnabled(GL_BLEND);
	glstate_backup.enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
	glstate_backup.enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

	glGetIntegerv(GL_VIEWPORT, glstate_backup.viewport);
	glGetIntegerv(GL_SCISSOR_BOX, glstate_backup.scissor);

	glGetIntegerv(GL_ACTIVE_TEXTURE, &glstate_backup.active_texture);

	glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &glstate_backup.stencil_clear_value);
	glGetFloatv(GL_COLOR_CLEAR_VALUE, glstate_backup.color_clear_value);

	glGetIntegerv(GL_BLEND_EQUATION_RGB, &glstate_backup.blend_equation_rgb);
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &glstate_backup.blend_equation_alpha);
	glGetIntegerv(GL_BLEND_SRC_RGB, &glstate_backup.blend_src_rgb);
	glGetIntegerv(GL_BLEND_DST_RGB, &glstate_backup.blend_dst_rgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &glstate_backup.blend_src_alpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &glstate_backup.blend_dst_alpha);

	glGetIntegerv(GL_STENCIL_FUNC, &glstate_backup.stencil_front.func);
	glGetIntegerv(GL_STENCIL_REF, &glstate_backup.stencil_front.ref);
	glGetIntegerv(GL_STENCIL_VALUE_MASK, &glstate_backup.stencil_front.value_mask);
	glGetIntegerv(GL_STENCIL_WRITEMASK, &glstate_backup.stencil_front.writemask);
	glGetIntegerv(GL_STENCIL_FAIL, &glstate_backup.stencil_front.fail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &glstate_backup.stencil_front.pass_depth_fail);
	glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &glstate_backup.stencil_front.pass_depth_pass);

	glGetIntegerv(GL_STENCIL_BACK_FUNC, &glstate_backup.stencil_back.func);
	glGetIntegerv(GL_STENCIL_BACK_REF, &glstate_backup.stencil_back.ref);
	glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &glstate_backup.stencil_back.value_mask);
	glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &glstate_backup.stencil_back.writemask);
	glGetIntegerv(GL_STENCIL_BACK_FAIL, &glstate_backup.stencil_back.fail);
	glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &glstate_backup.stencil_back.pass_depth_fail);
	glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &glstate_backup.stencil_back.pass_depth_pass);

	// Setup expected GL state.
	glViewport(0, 0, viewport_width, viewport_height);

	glClearStencil(0);
	glClearColor(0, 0, 0, 0);

	glActiveTexture(GL_TEXTURE0);

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
#if RMLUI_PREMULTIPLIED_ALPHA
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#else
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

#ifndef RMLUI_PLATFORM_EMSCRIPTEN
	// We do blending in nonlinear sRGB space because that is the common practice and gives results that we are used to.
	glDisable(GL_FRAMEBUFFER_SRGB);
#endif

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, GLuint(-1));
	glStencilMask(GLuint(-1));
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	SetTransform(nullptr);

	render_layers.BeginFrame(viewport_width, viewport_height);
	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
	glClear(GL_COLOR_BUFFER_BIT);

	UseProgram(ProgramId::None);
	program_transform_dirty.set();
	scissor_state = Rml::Rectanglei::MakeInvalid();

	Gfx::CheckGLError("BeginFrame");
}

void RenderInterface_GL3::EndFrame()
{
	const Gfx::FramebufferData& fb_active = render_layers.GetTopLayer();
	const Gfx::FramebufferData& fb_postprocess = render_layers.GetPostprocessPrimary();

	// Resolve MSAA to postprocess framebuffer.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_active.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_postprocess.framebuffer);

	glBlitFramebuffer(0, 0, fb_active.width, fb_active.height, 0, 0, fb_postprocess.width, fb_postprocess.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Draw to backbuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Assuming we have an opaque background, we can just write to it with the premultiplied alpha blend mode and we'll get the correct result.
	// Instead, if we had a transparent destination that didn't use pre-multiplied alpha, we would need to perform a manual un-premultiplication step.
	glActiveTexture(GL_TEXTURE0);
	Gfx::BindTexture(fb_postprocess);
	UseProgram(ProgramId::Passthrough);
	DrawFullscreenQuad();

	render_layers.EndFrame();

	// Restore GL state.
	if (glstate_backup.enable_cull_face)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	if (glstate_backup.enable_blend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	if (glstate_backup.enable_stencil_test)
		glEnable(GL_STENCIL_TEST);
	else
		glDisable(GL_STENCIL_TEST);

	if (glstate_backup.enable_scissor_test)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);

	glViewport(glstate_backup.viewport[0], glstate_backup.viewport[1], glstate_backup.viewport[2], glstate_backup.viewport[3]);
	glScissor(glstate_backup.scissor[0], glstate_backup.scissor[1], glstate_backup.scissor[2], glstate_backup.scissor[3]);

	glActiveTexture(glstate_backup.active_texture);

	glClearStencil(glstate_backup.stencil_clear_value);
	glClearColor(glstate_backup.color_clear_value[0], glstate_backup.color_clear_value[1], glstate_backup.color_clear_value[2],
		glstate_backup.color_clear_value[3]);

	glBlendEquationSeparate(glstate_backup.blend_equation_rgb, glstate_backup.blend_equation_alpha);
	glBlendFuncSeparate(glstate_backup.blend_src_rgb, glstate_backup.blend_dst_rgb, glstate_backup.blend_src_alpha, glstate_backup.blend_dst_alpha);

	glStencilFuncSeparate(GL_FRONT, glstate_backup.stencil_front.func, glstate_backup.stencil_front.ref, glstate_backup.stencil_front.value_mask);
	glStencilMaskSeparate(GL_FRONT, glstate_backup.stencil_front.writemask);
	glStencilOpSeparate(GL_FRONT, glstate_backup.stencil_front.fail, glstate_backup.stencil_front.pass_depth_fail,
		glstate_backup.stencil_front.pass_depth_pass);

	glStencilFuncSeparate(GL_BACK, glstate_backup.stencil_back.func, glstate_backup.stencil_back.ref, glstate_backup.stencil_back.value_mask);
	glStencilMaskSeparate(GL_BACK, glstate_backup.stencil_back.writemask);
	glStencilOpSeparate(GL_BACK, glstate_backup.stencil_back.fail, glstate_backup.stencil_back.pass_depth_fail,
		glstate_backup.stencil_back.pass_depth_pass);

	Gfx::CheckGLError("EndFrame");
}

void RenderInterface_GL3::Clear()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RenderInterface_GL3::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture,
	const Rml::Vector2f& translation)
{
	Rml::CompiledGeometryHandle geometry = CompileGeometry(vertices, num_vertices, indices, num_indices);

	if (geometry)
	{
		RenderCompiledGeometry(geometry, translation, texture);
		ReleaseCompiledGeometry(geometry);
	}
}

Rml::CompiledGeometryHandle RenderInterface_GL3::CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices)
{
	constexpr GLenum draw_usage = GL_STATIC_DRAW;

	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ibo = 0;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Rml::Vertex) * num_vertices, (const void*)vertices, draw_usage);

	glEnableVertexAttribArray((GLuint)Gfx::VertexAttribute::Position);
	glVertexAttribPointer((GLuint)Gfx::VertexAttribute::Position, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
		(const GLvoid*)(offsetof(Rml::Vertex, position)));

	glEnableVertexAttribArray((GLuint)Gfx::VertexAttribute::Color0);
	glVertexAttribPointer((GLuint)Gfx::VertexAttribute::Color0, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex),
		(const GLvoid*)(offsetof(Rml::Vertex, colour)));

	glEnableVertexAttribArray((GLuint)Gfx::VertexAttribute::TexCoord0);
	glVertexAttribPointer((GLuint)Gfx::VertexAttribute::TexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
		(const GLvoid*)(offsetof(Rml::Vertex, tex_coord)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * num_indices, (const void*)indices, draw_usage);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Gfx::CheckGLError("CompileGeometry");

	Gfx::CompiledGeometryData* geometry = new Gfx::CompiledGeometryData;
	geometry->vao = vao;
	geometry->vbo = vbo;
	geometry->ibo = ibo;
	geometry->draw_count = num_indices;

	return (Rml::CompiledGeometryHandle)geometry;
}

void RenderInterface_GL3::RenderCompiledGeometry(Rml::CompiledGeometryHandle handle, const Rml::Vector2f& translation, Rml::TextureHandle texture)
{
	Gfx::CompiledGeometryData* geometry = (Gfx::CompiledGeometryData*)handle;

	if (texture == TexturePostprocess)
	{
		// Do nothing.
	}
	else if (texture)
	{
		UseProgram(ProgramId::Texture);
		SubmitTransformUniform(translation);
		if (texture != TextureEnableWithoutBinding)
			glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
	}
	else
	{
		UseProgram(ProgramId::Color);
		glBindTexture(GL_TEXTURE_2D, 0);
		SubmitTransformUniform(translation);
	}

	glBindVertexArray(geometry->vao);
	glDrawElements(GL_TRIANGLES, geometry->draw_count, GL_UNSIGNED_INT, (const GLvoid*)0);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	Gfx::CheckGLError("RenderCompiledGeometry");
}

void RenderInterface_GL3::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle handle)
{
	Gfx::CompiledGeometryData* geometry = (Gfx::CompiledGeometryData*)handle;

	glDeleteVertexArrays(1, &geometry->vao);
	glDeleteBuffers(1, &geometry->vbo);
	glDeleteBuffers(1, &geometry->ibo);

	delete geometry;
}

/// Flip vertical axis of the rectangle, and move its origin to the vertically opposite side of the viewport.
/// @note Changes coordinate system from RmlUi to OpenGL, or equivalently in reverse.
/// @note The Rectangle::Top and Rectangle::Bottom members will have reverse meaning in the returned rectangle.
static Rml::Rectanglei VerticallyFlipped(Rml::Rectanglei rect, int viewport_height)
{
	RMLUI_ASSERT(rect.Valid());
	Rml::Rectanglei flipped_rect = rect;
	flipped_rect.p0.y = viewport_height - rect.p1.y;
	flipped_rect.p1.y = viewport_height - rect.p0.y;
	return flipped_rect;
}

void RenderInterface_GL3::SetScissor(Rml::Rectanglei region, bool vertically_flip)
{
	if (region.Valid() != scissor_state.Valid())
	{
		if (region.Valid())
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}

	if (region.Valid() && vertically_flip)
		region = VerticallyFlipped(region, viewport_height);

	if (region.Valid() && region != scissor_state)
		glScissor(region.Left(), viewport_height - region.Bottom(), region.Width(), region.Height());

	Gfx::CheckGLError("SetScissorRegion");
	scissor_state = region;
}

void RenderInterface_GL3::EnableScissorRegion(bool enable)
{
	// Assume enable is immediately followed by a SetScissorRegion() call, and ignore it here.
	if (!enable)
		SetScissor(Rml::Rectanglei::MakeInvalid(), false);
}

void RenderInterface_GL3::SetScissorRegion(int x, int y, int width, int height)
{
	SetScissor(Rml::Rectanglei::FromPositionSize({x, y}, {width, height}));
}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1)
struct TGAHeader {
	char idLength;
	char colourMapType;
	char dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char bitsPerPixel;
	char imageDescriptor;
};
// Restore packing
#pragma pack()

bool RenderInterface_GL3::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
	{
		return false;
	}

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	if (buffer_size <= sizeof(TGAHeader))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Texture file size is smaller than TGAHeader, file is not a valid TGA image.");
		file_interface->Close(file_handle);
		return false;
	}

	using Rml::byte;
	byte* buffer = new byte[buffer_size];
	file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer, sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	int image_size = header.width * header.height * 4; // We always make 32bit textures

	if (header.dataType != 2)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		delete[] buffer;
		return false;
	}

	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported.");
		delete[] buffer;
		return false;
	}

	const byte* image_src = buffer + sizeof(TGAHeader);
	byte* image_dest = new byte[image_size];

	// Targa is BGR, swap to RGB and flip Y axis
	for (long y = 0; y < header.height; y++)
	{
		long read_index = y * header.width * color_mode;
		long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : (header.height - y - 1) * header.width * 4;
		for (long x = 0; x < header.width; x++)
		{
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4)
				image_dest[write_index + 3] = image_src[read_index + 3];
			else
				image_dest[write_index + 3] = 255;

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;

	bool success = GenerateTexture(texture_handle, image_dest, texture_dimensions);

	delete[] image_dest;
	delete[] buffer;

	return success;
}

bool RenderInterface_GL3::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	if (texture_id == 0)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to generate texture.");
		return false;
	}

#if RMLUI_PREMULTIPLIED_ALPHA
	using Rml::byte;
	Rml::UniquePtr<byte[]> source_premultiplied;
	if (source)
	{
		const size_t num_bytes = source_dimensions.x * source_dimensions.y * 4;
		source_premultiplied = Rml::UniquePtr<byte[]>(new byte[num_bytes]);

		for (size_t i = 0; i < num_bytes; i += 4)
		{
			const byte alpha = source[i + 3];
			for (size_t j = 0; j < 3; j++)
				source_premultiplied[i + j] = byte((int(source[i + j]) * int(alpha)) / 255);
			source_premultiplied[i + 3] = alpha;
		}

		source = source_premultiplied.get();
	}
#endif

	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	texture_handle = (Rml::TextureHandle)texture_id;

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void RenderInterface_GL3::DrawFullscreenQuad()
{
	RenderCompiledGeometry(fullscreen_quad_geometry, {}, RenderInterface_GL3::TexturePostprocess);
}

void RenderInterface_GL3::DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling)
{
	Rml::Vertex vertices[4];
	int indices[6];
	Rml::GeometryUtilities::GenerateQuad(vertices, indices, Rml::Vector2f(-1), Rml::Vector2f(2), {});
	if (uv_offset != Rml::Vector2f() || uv_scaling != Rml::Vector2f(1.f))
	{
		for (Rml::Vertex& vertex : vertices)
			vertex.tex_coord = (vertex.tex_coord * uv_scaling) + uv_offset;
	}
	RenderGeometry(vertices, 4, indices, 6, RenderInterface_GL3::TexturePostprocess, {});
}

static void SigmaToParameters(const float desired_sigma, int& out_pass_level, float& out_sigma)
{
	constexpr int max_num_passes = 10;
	static_assert(max_num_passes < 31, "");
	constexpr float max_single_pass_sigma = 3.0f;
	out_pass_level = Rml::Math::Clamp(Rml::Math::Log2(int(desired_sigma * (2.f / max_single_pass_sigma))), 0, max_num_passes);
	out_sigma = Rml::Math::Clamp(desired_sigma / float(1 << out_pass_level), 0.0f, max_single_pass_sigma);
}

static void SetTexCoordLimits(GLint tex_coord_min_location, GLint tex_coord_max_location, Rml::Rectanglei rectangle_flipped,
	Rml::Vector2i framebuffer_size)
{
	// Offset by half-texel values so that texture lookups are clamped to fragment centers, thereby avoiding color
	// bleeding from neighboring texels due to bilinear interpolation.
	const Rml::Vector2f min = (Rml::Vector2f(rectangle_flipped.p0) + Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);
	const Rml::Vector2f max = (Rml::Vector2f(rectangle_flipped.p1) - Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);

	glUniform2f(tex_coord_min_location, min.x, min.y);
	glUniform2f(tex_coord_max_location, max.x, max.y);
}

static void SetBlurWeights(GLint weights_location, float sigma)
{
	constexpr int num_weights = BLUR_NUM_WEIGHTS;
	float weights[num_weights];
	float normalization = 0.0f;
	for (int i = 0; i < num_weights; i++)
	{
		if (Rml::Math::Absolute(sigma) < 0.1f)
			weights[i] = float(i == 0);
		else
			weights[i] = Rml::Math::Exp(-float(i * i) / (2.0f * sigma * sigma)) / (Rml::Math::SquareRoot(2.f * Rml::Math::RMLUI_PI) * sigma);

		normalization += (i == 0 ? 1.f : 2.0f) * weights[i];
	}
	for (int i = 0; i < num_weights; i++)
		weights[i] /= normalization;

	glUniform1fv(weights_location, (GLsizei)num_weights, &weights[0]);
}

void RenderInterface_GL3::RenderBlur(float sigma, const Gfx::FramebufferData& source_destination, const Gfx::FramebufferData& temp,
	const Rml::Rectanglei window_flipped)
{
	RMLUI_ASSERT(&source_destination != &temp && source_destination.width == temp.width && source_destination.height == temp.height);
	RMLUI_ASSERT(window_flipped.Valid());

	int pass_level = 0;
	SigmaToParameters(sigma, pass_level, sigma);

	const Rml::Rectanglei original_scissor = scissor_state;

	// Begin by downscaling so that the blur pass can be done at a reduced resolution for large sigma.
	Rml::Rectanglei scissor = window_flipped;

	UseProgram(ProgramId::Passthrough);
	SetScissor(scissor, true);

	// Downscale by iterative half-scaling with bilinear filtering, to reduce aliasing.
	glViewport(0, 0, source_destination.width / 2, source_destination.height / 2);

	// Scale UVs if we have even dimensions, such that texture fetches align perfectly between texels, thereby producing a 50% blend of
	// neighboring texels.
	const Rml::Vector2f uv_scaling = {(source_destination.width % 2 == 1) ? (1.f - 1.f / float(source_destination.width)) : 1.f,
		(source_destination.height % 2 == 1) ? (1.f - 1.f / float(source_destination.height)) : 1.f};

	for (int i = 0; i < pass_level; i++)
	{
		scissor.p0 = (scissor.p0 + Rml::Vector2i(1)) / 2;
		scissor.p1 = Rml::Math::Max(scissor.p1 / 2, scissor.p0);
		const bool from_source = (i % 2 == 0);
		Gfx::BindTexture(from_source ? source_destination : temp);
		glBindFramebuffer(GL_FRAMEBUFFER, (from_source ? temp : source_destination).framebuffer);
		SetScissor(scissor, true);

		DrawFullscreenQuad({}, uv_scaling);
	}

	glViewport(0, 0, source_destination.width, source_destination.height);

	// Ensure texture data end up in the temp buffer. Depending on the last downscaling, we might need to move it from the source_destination buffer.
	const bool transfer_to_temp_buffer = (pass_level % 2 == 0);
	if (transfer_to_temp_buffer)
	{
		Gfx::BindTexture(source_destination);
		glBindFramebuffer(GL_FRAMEBUFFER, temp.framebuffer);
		DrawFullscreenQuad();
	}

	// Set up uniforms.
	UseProgram(ProgramId::Blur);
	SetBlurWeights(GetUniformLocation(UniformId::Weights), sigma);
	SetTexCoordLimits(GetUniformLocation(UniformId::TexCoordMin), GetUniformLocation(UniformId::TexCoordMax), scissor,
		{source_destination.width, source_destination.height});

	const GLint texel_offset_location = GetUniformLocation(UniformId::TexelOffset);
	auto SetTexelOffset = [texel_offset_location](Rml::Vector2f blur_direction, int texture_dimension) {
		const Rml::Vector2f texel_offset = blur_direction * (1.0f / float(texture_dimension));
		glUniform2f(texel_offset_location, texel_offset.x, texel_offset.y);
	};

	// Blur render pass - vertical.
	Gfx::BindTexture(temp);
	glBindFramebuffer(GL_FRAMEBUFFER, source_destination.framebuffer);

	SetTexelOffset({0.f, 1.f}, temp.height);
	DrawFullscreenQuad();

	// Blur render pass - horizontal.
	Gfx::BindTexture(source_destination);
	glBindFramebuffer(GL_FRAMEBUFFER, temp.framebuffer);

	SetTexelOffset({1.f, 0.f}, source_destination.width);
	DrawFullscreenQuad();

	// Blit the blurred image to the scissor region with upscaling.
	SetScissor(window_flipped, true);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, temp.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, source_destination.framebuffer);

	const Rml::Vector2i src_min = scissor.p0;
	const Rml::Vector2i src_max = scissor.p1;
	const Rml::Vector2i dst_min = window_flipped.p0;
	const Rml::Vector2i dst_max = window_flipped.p1;
	glBlitFramebuffer(src_min.x, src_min.y, src_max.x, src_max.y, dst_min.x, dst_min.y, dst_max.x, dst_max.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// The above upscale blit might be jittery at low resolutions (large pass levels). This is especially noticable when moving an element with
	// backdrop blur around or when trying to click/hover an element within a blurred region since it may be rendered at an offset. For more stable
	// and accurate rendering we next upscale the blur image by an exact power-of-two. However, this may not fill the edges completely so we need to
	// do the above first. Note that this strategy may sometimes result in visible seams. Alternatively, we could try to enlargen the window to the
	// next power-of-two size and then downsample and blur that.
	const Rml::Vector2i target_min = src_min * (1 << pass_level);
	const Rml::Vector2i target_max = src_max * (1 << pass_level);
	if (target_min != dst_min || target_max != dst_max)
	{
		glBlitFramebuffer(src_min.x, src_min.y, src_max.x, src_max.y, target_min.x, target_min.y, target_max.x, target_max.y, GL_COLOR_BUFFER_BIT,
			GL_LINEAR);
	}

	// Restore render state.
	SetScissor(original_scissor);

	Gfx::CheckGLError("Blur");
}

void RenderInterface_GL3::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	glDeleteTextures(1, (GLuint*)&texture_handle);
}

void RenderInterface_GL3::SetTransform(const Rml::Matrix4f* new_transform)
{
	transform = projection * (new_transform ? *new_transform : Rml::Matrix4f::Identity());
	program_transform_dirty.set();
}

enum class FilterType { Invalid = 0, Passthrough, Blur, ColorMatrix };
struct CompiledFilter {
	FilterType type;

	// Passthrough
	float blend_factor;

	// Blur
	float sigma;

	// ColorMatrix
	Rml::Matrix4f color_matrix;
};

Rml::CompiledFilterHandle RenderInterface_GL3::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters)
{
	CompiledFilter filter = {};

	if (name == "opacity")
	{
		filter.type = FilterType::Passthrough;
		filter.blend_factor = Rml::Get(parameters, "value", 1.0f);
	}
	else if (name == "blur")
	{
		filter.type = FilterType::Blur;
		filter.sigma = 0.5f * Rml::Get(parameters, "radius", 1.0f);
	}
	else if (name == "brightness")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		filter.color_matrix = Rml::Matrix4f::Diag(value, value, value, 1.f);
	}
	else if (name == "contrast")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float grayness = 0.5f - 0.5f * value;
		filter.color_matrix = Rml::Matrix4f::Diag(value, value, value, 1.f);
		filter.color_matrix.SetColumn(3, Rml::Vector4f(grayness, grayness, grayness, 1.f));
	}
	else if (name == "invert")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Math::Clamp(Rml::Get(parameters, "value", 1.0f), 0.f, 1.f);
		const float inverted = 1.f - 2.f * value;
		filter.color_matrix = Rml::Matrix4f::Diag(inverted, inverted, inverted, 1.f);
		filter.color_matrix.SetColumn(3, Rml::Vector4f(value, value, value, 1.f));
	}
	else if (name == "grayscale")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Rml::Vector3f gray = value * Rml::Vector3f(0.2126f, 0.7152f, 0.0722f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{gray.x + rev_value, gray.y,             gray.z,             0.f},
			{gray.x,             gray.y + rev_value, gray.z,             0.f},
			{gray.x,             gray.y,             gray.z + rev_value, 0.f},
			{0.f,                0.f,                0.f,                1.f}
		);
		// clang-format on
	}
	else if (name == "sepia")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float rev_value = 1.f - value;
		const Rml::Vector3f r_mix = value * Rml::Vector3f(0.393f, 0.769f, 0.189f);
		const Rml::Vector3f g_mix = value * Rml::Vector3f(0.349f, 0.686f, 0.168f);
		const Rml::Vector3f b_mix = value * Rml::Vector3f(0.272f, 0.534f, 0.131f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{r_mix.x + rev_value, r_mix.y,             r_mix.z,             0.f},
			{g_mix.x,             g_mix.y + rev_value, g_mix.z,             0.f},
			{b_mix.x,             b_mix.y,             b_mix.z + rev_value, 0.f},
			{0.f,                 0.f,                 0.f,                 1.f}
		);
		// clang-format on
	}
	else if (name == "hue-rotate")
	{
		// Hue-rotation and saturation values based on: https://www.w3.org/TR/filter-effects-1/#attr-valuedef-type-huerotate
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		const float s = Rml::Math::Sin(value);
		const float c = Rml::Math::Cos(value);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{0.213f + 0.787f * c - 0.213f * s,  0.715f - 0.715f * c - 0.715f * s,  0.072f - 0.072f * c + 0.928f * s,  0.f},
			{0.213f - 0.213f * c + 0.143f * s,  0.715f + 0.285f * c + 0.140f * s,  0.072f - 0.072f * c - 0.283f * s,  0.f},
			{0.213f - 0.213f * c - 0.787f * s,  0.715f - 0.715f * c + 0.715f * s,  0.072f + 0.928f * c + 0.072f * s,  0.f},
			{0.f,                               0.f,                               0.f,                               1.f}
		);
		// clang-format on
	}
	else if (name == "saturate")
	{
		filter.type = FilterType::ColorMatrix;
		const float value = Rml::Get(parameters, "value", 1.0f);
		// clang-format off
		filter.color_matrix = Rml::Matrix4f::FromRows(
			{0.213f + 0.787f * value,  0.715f - 0.715f * value,  0.072f - 0.072f * value,  0.f},
			{0.213f - 0.213f * value,  0.715f + 0.285f * value,  0.072f - 0.072f * value,  0.f},
			{0.213f - 0.213f * value,  0.715f - 0.715f * value,  0.072f + 0.928f * value,  0.f},
			{0.f,                      0.f,                      0.f,                      1.f}
		);
		// clang-format on
	}

	if (filter.type != FilterType::Invalid)
		return reinterpret_cast<Rml::CompiledFilterHandle>(new CompiledFilter(std::move(filter)));

	Rml::Log::Message(Rml::Log::LT_WARNING, "Unsupported filter type '%s'.", name.c_str());
	return {};
}

void RenderInterface_GL3::ReleaseCompiledFilter(Rml::CompiledFilterHandle filter)
{
	delete reinterpret_cast<CompiledFilter*>(filter);
}

void RenderInterface_GL3::BlitTopLayerToPostprocessPrimary()
{
	const Gfx::FramebufferData& source = render_layers.GetTopLayer();
	const Gfx::FramebufferData& destination = render_layers.GetPostprocessPrimary();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, source.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.framebuffer);

	// Blit and resolve MSAA. Any active scissor state will restrict the size of the blit region.
	glBlitFramebuffer(0, 0, source.width, source.height, 0, 0, destination.width, destination.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void RenderInterface_GL3::RenderFilters(const Rml::FilterHandleList& filter_handles)
{
	for (const Rml::CompiledFilterHandle filter_handle : filter_handles)
	{
		const CompiledFilter& filter = *reinterpret_cast<const CompiledFilter*>(filter_handle);
		const FilterType type = filter.type;

		switch (type)
		{
		case FilterType::Passthrough:
		{
			UseProgram(ProgramId::Passthrough);
			glBlendFunc(GL_CONSTANT_ALPHA, GL_ZERO);
			glBlendColor(0.0f, 0.0f, 0.0f, filter.blend_factor);

			const Gfx::FramebufferData& source = render_layers.GetPostprocessPrimary();
			const Gfx::FramebufferData& destination = render_layers.GetPostprocessSecondary();
			Gfx::BindTexture(source);
			glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);

			DrawFullscreenQuad();

			render_layers.SwapPostprocessPrimarySecondary();
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;
		case FilterType::Blur:
		{
			glDisable(GL_BLEND);

			const Gfx::FramebufferData& source_destination = render_layers.GetPostprocessPrimary();
			const Gfx::FramebufferData& temp = render_layers.GetPostprocessSecondary();

			const Rml::Rectanglei window_flipped = VerticallyFlipped(scissor_state, viewport_height);
			RenderBlur(filter.sigma, source_destination, temp, window_flipped);

			glEnable(GL_BLEND);
		}
		break;
		case FilterType::ColorMatrix:
		{
			UseProgram(ProgramId::ColorMatrix);
			glDisable(GL_BLEND);

			const GLint uniform_location = program_data->uniforms.Get(ProgramId::ColorMatrix, UniformId::ColorMatrix);
			constexpr bool transpose = std::is_same<decltype(filter.color_matrix), Rml::RowMajorMatrix4f>::value;
			glUniformMatrix4fv(uniform_location, 1, transpose, filter.color_matrix.data());

			const Gfx::FramebufferData& source = render_layers.GetPostprocessPrimary();
			const Gfx::FramebufferData& destination = render_layers.GetPostprocessSecondary();
			Gfx::BindTexture(source);
			glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);

			DrawFullscreenQuad();

			render_layers.SwapPostprocessPrimarySecondary();
			glEnable(GL_BLEND);
		}
		break;
		case FilterType::Invalid:
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Unhandled render filter %d.", (int)type);
		}
		break;
		}
	}

	Gfx::CheckGLError("RenderFilter");
}

void RenderInterface_GL3::PushLayer(Rml::LayerFill layer_fill)
{
	if (layer_fill == Rml::LayerFill::Clone)
		render_layers.PushLayerClone();
	else
		render_layers.PushLayer();

	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
	if (layer_fill == Rml::LayerFill::Clear)
		glClear(GL_COLOR_BUFFER_BIT);
}

void RenderInterface_GL3::PopLayer(Rml::BlendMode blend_mode, const Rml::FilterHandleList& filters)
{
	using Rml::BlendMode;

	if (blend_mode == BlendMode::Discard)
	{
		RMLUI_ASSERT(filters.empty());
		render_layers.PopLayer();
		glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
		return;
	}

	// Blit stack to filter rendering buffer. Do this regardless of whether we actually have any filters to be applied,
	// because we need to resolve the multi-sampled framebuffer in any case.
	// @performance If we have BlendMode::Replace and no filters or mask then we can just blit directly to the destination.
	BlitTopLayerToPostprocessPrimary();

	// Render the filters, the PostprocessPrimary framebuffer is used for both input and output.
	RenderFilters(filters);

	// Pop the active layer, thereby activating the beneath layer.
	render_layers.PopLayer();

	// Render to the activated layer. Apply any mask if active.
	glBindFramebuffer(GL_FRAMEBUFFER, render_layers.GetTopLayer().framebuffer);
	Gfx::BindTexture(render_layers.GetPostprocessPrimary());

	UseProgram(ProgramId::Passthrough);

	if (blend_mode == BlendMode::Replace)
		glDisable(GL_BLEND);

	DrawFullscreenQuad();

	if (blend_mode == BlendMode::Replace)
		glEnable(GL_BLEND);

	Gfx::CheckGLError("PopLayer");
}

void RenderInterface_GL3::UseProgram(ProgramId program_id)
{
	RMLUI_ASSERT(program_data);
	if (active_program != program_id)
	{
		if (program_id != ProgramId::None)
			glUseProgram(program_data->programs[program_id]);
		active_program = program_id;
	}
}

int RenderInterface_GL3::GetUniformLocation(UniformId uniform_id) const
{
	return program_data->uniforms.Get(active_program, uniform_id);
}

void RenderInterface_GL3::SubmitTransformUniform(Rml::Vector2f translation)
{
	static_assert((size_t)ProgramId::Count < MaxNumPrograms, "Maximum number of programs exceeded.");
	const size_t program_index = (size_t)active_program;

	if (program_transform_dirty.test(program_index))
	{
		glUniformMatrix4fv(GetUniformLocation(UniformId::Transform), 1, false, transform.data());
		program_transform_dirty.set(program_index, false);
	}

	glUniform2fv(GetUniformLocation(UniformId::Translate), 1, &translation.x);

	Gfx::CheckGLError("SubmitTransformUniform");
}

RenderInterface_GL3::RenderLayerStack::RenderLayerStack()
{
	fb_postprocess.resize(3);
}

RenderInterface_GL3::RenderLayerStack::~RenderLayerStack()
{
	DestroyFramebuffers();
}

void RenderInterface_GL3::RenderLayerStack::PushLayer()
{
	RMLUI_ASSERT(layers_size <= (int)fb_layers.size());

	if (layers_size == (int)fb_layers.size())
	{
		// All framebuffers should share a single stencil buffer.
		GLuint shared_depth_stencil = (fb_layers.empty() ? 0 : fb_layers.front().depth_stencil_buffer);

		fb_layers.push_back(Gfx::FramebufferData{});
		Gfx::CreateFramebuffer(fb_layers.back(), width, height, NUM_MSAA_SAMPLES, Gfx::FramebufferAttachment::DepthStencil, shared_depth_stencil);
	}

	layers_size += 1;
}

void RenderInterface_GL3::RenderLayerStack::PushLayerClone()
{
	RMLUI_ASSERT(layers_size > 0);
	fb_layers.insert(fb_layers.begin() + layers_size, Gfx::FramebufferData{fb_layers[layers_size - 1]});
	layers_size += 1;
}

void RenderInterface_GL3::RenderLayerStack::PopLayer()
{
	RMLUI_ASSERT(layers_size > 0);
	layers_size -= 1;

	// Only cloned framebuffers are removed. Other framebuffers remain for later re-use.
	if (IsCloneOfBelow(layers_size))
		fb_layers.erase(fb_layers.begin() + layers_size);
}

const Gfx::FramebufferData& RenderInterface_GL3::RenderLayerStack::GetTopLayer() const
{
	RMLUI_ASSERT(layers_size > 0);
	return fb_layers[layers_size - 1];
}

void RenderInterface_GL3::RenderLayerStack::SwapPostprocessPrimarySecondary()
{
	std::swap(fb_postprocess[0], fb_postprocess[1]);
}

void RenderInterface_GL3::RenderLayerStack::BeginFrame(int new_width, int new_height)
{
	RMLUI_ASSERT(layers_size == 0);

	if (new_width != width || new_height != height)
	{
		width = new_width;
		height = new_height;

		DestroyFramebuffers();
	}

	PushLayer();
}

void RenderInterface_GL3::RenderLayerStack::EndFrame()
{
	RMLUI_ASSERT(layers_size == 1);
	PopLayer();
}

void RenderInterface_GL3::RenderLayerStack::DestroyFramebuffers()
{
	RMLUI_ASSERTMSG(layers_size == 0, "Do not call this during frame rendering, that is, between BeginFrame() and EndFrame().");

	for (Gfx::FramebufferData& fb : fb_layers)
		Gfx::DestroyFramebuffer(fb);

	fb_layers.clear();

	for (Gfx::FramebufferData& fb : fb_postprocess)
		Gfx::DestroyFramebuffer(fb);
}

bool RenderInterface_GL3::RenderLayerStack::IsCloneOfBelow(int layer_index) const
{
	const bool result =
		(layer_index >= 1 && layer_index < (int)fb_layers.size() && fb_layers[layer_index].framebuffer == fb_layers[layer_index - 1].framebuffer);
	return result;
}

const Gfx::FramebufferData& RenderInterface_GL3::RenderLayerStack::EnsureFramebufferPostprocess(int index)
{
	RMLUI_ASSERT(index < (int)fb_postprocess.size())
	Gfx::FramebufferData& fb = fb_postprocess[index];
	if (!fb.framebuffer)
		Gfx::CreateFramebuffer(fb, width, height, 0, Gfx::FramebufferAttachment::None, 0);
	return fb;
}

bool RmlGL3::Initialize(Rml::String* out_message)
{
#if defined RMLUI_PLATFORM_EMSCRIPTEN
	if (out_message)
		*out_message = "Started Emscripten WebGL renderer.";
#elif !defined RMLUI_GL3_CUSTOM_LOADER
	const int gl_version = gladLoaderLoadGL();
	if (gl_version == 0)
	{
		if (out_message)
			*out_message = "Failed to initialize OpenGL context.";
		return false;
	}

	if (out_message)
		*out_message = Rml::CreateString(128, "Loaded OpenGL %d.%d.", GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));
#endif

	return true;
}

void RmlGL3::Shutdown()
{
#if !defined RMLUI_PLATFORM_EMSCRIPTEN && !defined RMLUI_GL3_CUSTOM_LOADER
	gladLoaderUnloadGL();
#endif
}
