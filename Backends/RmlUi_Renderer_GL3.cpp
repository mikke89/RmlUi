/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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
	#define RMLUI_SHADER_HEADER "#version 300 es\nprecision highp float;\n"
	#include <GLES3/gl3.h>
#elif defined RMLUI_GL3_CUSTOM_LOADER
	#define RMLUI_SHADER_HEADER "#version 330\n"
	#include RMLUI_GL3_CUSTOM_LOADER
#else
	#define RMLUI_SHADER_HEADER "#version 330\n"
	#define GLAD_GL_IMPLEMENTATION
	#include "RmlUi_Include_GL3.h"
#endif

static const char* shader_main_vertex = RMLUI_SHADER_HEADER R"(
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

	vec2 translatedPos = inPosition + _translate.xy;
	vec4 outPos = _transform * vec4(translatedPos, 0, 1);

    gl_Position = outPos;
}
)";

static const char* shader_main_fragment_texture = RMLUI_SHADER_HEADER R"(
uniform sampler2D _tex;
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	vec4 texColor = texture(_tex, fragTexCoord);
	finalColor = fragColor * texColor;
}
)";
static const char* shader_main_fragment_color = RMLUI_SHADER_HEADER R"(
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main() {
	finalColor = fragColor;
}
)";

namespace Gfx {

enum class ProgramUniform { Translate, Transform, Tex, Count };
static const char* const program_uniform_names[(size_t)ProgramUniform::Count] = {"_translate", "_transform", "_tex"};

enum class VertexAttribute { Position, Color0, TexCoord0, Count };
static const char* const vertex_attribute_names[(size_t)VertexAttribute::Count] = {"inPosition", "inColor0", "inTexCoord0"};

struct CompiledGeometryData {
	Rml::TextureHandle texture;
	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	GLsizei draw_count;
};

struct ProgramData {
	GLuint id;
	GLint uniform_locations[(size_t)ProgramUniform::Count];
};

struct ShadersData {
	ProgramData program_color;
	ProgramData program_texture;
	GLuint shader_main_vertex;
	GLuint shader_main_fragment_color;
	GLuint shader_main_fragment_texture;
};

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
static GLuint CreateShader(GLenum shader_type, const char* code_string)
{
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
		return 0;
	}

	CheckGLError("CreateShader");

	return id;
}

static void BindAttribLocations(GLuint program)
{
	for (GLuint i = 0; i < (GLuint)VertexAttribute::Count; i++)
	{
		glBindAttribLocation(program, i, vertex_attribute_names[i]);
	}
	CheckGLError("BindAttribLocations");
}

static bool CreateProgram(GLuint vertex_shader, GLuint fragment_shader, ProgramData& out_program)
{
	GLuint id = glCreateProgram();
	RMLUI_ASSERT(id);

	BindAttribLocations(id);

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

	out_program = {};
	out_program.id = id;

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
		ProgramUniform program_uniform = ProgramUniform::Count;
		for (int i = 0; i < (int)ProgramUniform::Count; i++)
		{
			const char* uniform_name = program_uniform_names[i];
			if (strcmp(name_buf, uniform_name) == 0)
			{
				program_uniform = (ProgramUniform)i;
				break;
			}
		}

		if ((size_t)program_uniform < (size_t)ProgramUniform::Count)
		{
			out_program.uniform_locations[(size_t)program_uniform] = location;
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

static bool CreateShaders(ShadersData& out_shaders)
{
	out_shaders = {};
	GLuint& main_vertex = out_shaders.shader_main_vertex;
	GLuint& main_fragment_color = out_shaders.shader_main_fragment_color;
	GLuint& main_fragment_texture = out_shaders.shader_main_fragment_texture;

	main_vertex = CreateShader(GL_VERTEX_SHADER, shader_main_vertex);
	if (!main_vertex)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not create OpenGL shader: 'shader_main_vertex'.");
		return false;
	}
	main_fragment_color = CreateShader(GL_FRAGMENT_SHADER, shader_main_fragment_color);
	if (!main_fragment_color)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not create OpenGL shader: 'shader_main_fragment_color'.");
		return false;
	}
	main_fragment_texture = CreateShader(GL_FRAGMENT_SHADER, shader_main_fragment_texture);
	if (!main_fragment_texture)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not create OpenGL shader: 'shader_main_fragment_texture'.");
		return false;
	}

	if (!CreateProgram(main_vertex, main_fragment_color, out_shaders.program_color))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not create OpenGL program: 'program_color'.");
		return false;
	}
	if (!CreateProgram(main_vertex, main_fragment_texture, out_shaders.program_texture))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not create OpenGL program: 'program_texture'.");
		return false;
	}

	return true;
}

static void DestroyShaders(ShadersData& shaders)
{
	glDeleteProgram(shaders.program_color.id);
	glDeleteProgram(shaders.program_texture.id);

	glDeleteShader(shaders.shader_main_vertex);
	glDeleteShader(shaders.shader_main_fragment_color);
	glDeleteShader(shaders.shader_main_fragment_texture);

	shaders = {};
}

} // namespace Gfx

RenderInterface_GL3::RenderInterface_GL3()
{
	shaders = Rml::MakeUnique<Gfx::ShadersData>();

	if (!Gfx::CreateShaders(*shaders))
		shaders.reset();
}

RenderInterface_GL3::~RenderInterface_GL3()
{
	if (shaders)
		Gfx::DestroyShaders(*shaders);
}

void RenderInterface_GL3::SetViewport(int width, int height)
{
	viewport_width = width;
	viewport_height = height;
}

void RenderInterface_GL3::BeginFrame()
{
	RMLUI_ASSERT(viewport_width >= 0 && viewport_height >= 0);
	glViewport(0, 0, viewport_width, viewport_height);

	glClearStencil(0);
	glClearColor(0, 0, 0, 1);

	glDisable(GL_CULL_FACE);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, GLuint(-1));
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projection = Rml::Matrix4f::ProjectOrtho(0, (float)viewport_width, (float)viewport_height, 0, -10000, 10000);

	SetTransform(nullptr);
}

void RenderInterface_GL3::EndFrame() {}

void RenderInterface_GL3::Clear()
{
	glClearStencil(0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RenderInterface_GL3::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture,
	const Rml::Vector2f& translation)
{
	Rml::CompiledGeometryHandle geometry = CompileGeometry(vertices, num_vertices, indices, num_indices, texture);

	if (geometry)
	{
		RenderCompiledGeometry(geometry, translation);
		ReleaseCompiledGeometry(geometry);
	}
}

Rml::CompiledGeometryHandle RenderInterface_GL3::CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices,
	Rml::TextureHandle texture)
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

	Gfx::CheckGLError("CompileGeometry");

	Gfx::CompiledGeometryData* geometry = new Gfx::CompiledGeometryData;
	geometry->texture = texture;
	geometry->vao = vao;
	geometry->vbo = vbo;
	geometry->ibo = ibo;
	geometry->draw_count = num_indices;

	return (Rml::CompiledGeometryHandle)geometry;
}

void RenderInterface_GL3::RenderCompiledGeometry(Rml::CompiledGeometryHandle handle, const Rml::Vector2f& translation)
{
	Gfx::CompiledGeometryData* geometry = (Gfx::CompiledGeometryData*)handle;

	if (geometry->texture)
	{
		glUseProgram(shaders->program_texture.id);
		if (geometry->texture != TextureEnableWithoutBinding)
			glBindTexture(GL_TEXTURE_2D, (GLuint)geometry->texture);
		SubmitTransformUniform(ProgramId::Texture, shaders->program_texture.uniform_locations[(size_t)Gfx::ProgramUniform::Transform]);
		glUniform2fv(shaders->program_texture.uniform_locations[(size_t)Gfx::ProgramUniform::Translate], 1, &translation.x);
	}
	else
	{
		glUseProgram(shaders->program_color.id);
		glBindTexture(GL_TEXTURE_2D, 0);
		SubmitTransformUniform(ProgramId::Color, shaders->program_color.uniform_locations[(size_t)Gfx::ProgramUniform::Transform]);
		glUniform2fv(shaders->program_color.uniform_locations[(size_t)Gfx::ProgramUniform::Translate], 1, &translation.x);
	}

	glBindVertexArray(geometry->vao);
	glDrawElements(GL_TRIANGLES, geometry->draw_count, GL_UNSIGNED_INT, (const GLvoid*)0);
	glBindVertexArray(0);

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

void RenderInterface_GL3::EnableScissorRegion(bool enable)
{
	ScissoringState new_state = ScissoringState::Disable;

	if (enable)
		new_state = (transform_active ? ScissoringState::Stencil : ScissoringState::Scissor);

	if (new_state != scissoring_state)
	{
		// Disable old
		if (scissoring_state == ScissoringState::Scissor)
			glDisable(GL_SCISSOR_TEST);
		else if (scissoring_state == ScissoringState::Stencil)
			glStencilFunc(GL_ALWAYS, 1, GLuint(-1));

		// Enable new
		if (new_state == ScissoringState::Scissor)
			glEnable(GL_SCISSOR_TEST);
		else if (new_state == ScissoringState::Stencil)
			glStencilFunc(GL_EQUAL, 1, GLuint(-1));

		scissoring_state = new_state;
	}
}

void RenderInterface_GL3::SetScissorRegion(int x, int y, int width, int height)
{
	if (transform_active)
	{
		const float left = float(x);
		const float right = float(x + width);
		const float top = float(y);
		const float bottom = float(y + height);

		Rml::Vertex vertices[4];
		vertices[0].position = {left, top};
		vertices[1].position = {right, top};
		vertices[2].position = {right, bottom};
		vertices[3].position = {left, bottom};

		int indices[6] = {0, 2, 1, 0, 3, 2};

		glClear(GL_STENCIL_BUFFER_BIT);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glStencilFunc(GL_ALWAYS, 1, GLuint(-1));
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		RenderGeometry(vertices, 4, indices, 6, 0, Rml::Vector2f(0, 0));

		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 1, GLuint(-1));
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
	else
	{
		glScissor(x, viewport_height - (y + height), width, height);
	}
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
			{
				const int alpha = image_src[read_index + 3];
#ifdef RMLUI_SRGB_PREMULTIPLIED_ALPHA
				image_dest[write_index + 0] = (image_dest[write_index + 0] * alpha) / 255;
				image_dest[write_index + 1] = (image_dest[write_index + 1] * alpha) / 255;
				image_dest[write_index + 2] = (image_dest[write_index + 2] * alpha) / 255;
#endif
				image_dest[write_index + 3] = (byte)alpha;
			}
			else
			{
				image_dest[write_index + 3] = 255;
			}

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

	glBindTexture(GL_TEXTURE_2D, texture_id);

	GLint internal_format = GL_RGBA8;
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	texture_handle = (Rml::TextureHandle)texture_id;

	return true;
}

void RenderInterface_GL3::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	glDeleteTextures(1, (GLuint*)&texture_handle);
}

void RenderInterface_GL3::SetTransform(const Rml::Matrix4f* new_transform)
{
	transform_active = (new_transform != nullptr);
	transform = projection * (new_transform ? *new_transform : Rml::Matrix4f::Identity());
	transform_dirty_state = ProgramId::All;
}

void RenderInterface_GL3::SubmitTransformUniform(ProgramId program_id, int uniform_location)
{
	if ((int)program_id & (int)transform_dirty_state)
	{
		glUniformMatrix4fv(uniform_location, 1, false, transform.data());
		transform_dirty_state = ProgramId((int)transform_dirty_state & ~(int)program_id);
	}
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
