/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2025 The RmlUi Team, and contributors
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

#include "RmlUi_Renderer_DX12.h"
#include "RmlUi_Backend.h"
// #include "RmlUi_Vulkan/ShadersCompiledSPV.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Profiling.h>
#include <RmlUi/Core/Stream.h>
#include <algorithm>
#include <string.h>

#ifndef RMLUI_PLATFORM_WIN32
	#error unable to compile platform specific renderer required Windows OS that support DirectX-12
#endif

#ifdef RMLUI_PLATFORM_WIN32
	#include <RmlUi_Platform_Win32.h>
	#ifdef RMLUI_DX_DEBUG
		#include <dxgidebug.h>
	#endif

/// @brief in bytes see pShaderSourceText_Vertex - > cbuffer ConstantBuffer : register(b0)
constexpr uint32_t kAllocationSize_ConstantBuffer_Vertex_Main = 72;

/// @brief in bytes see pShaderSourceText_Vertex_Blur - > cbuffer SharedConstantBuffer : register(b0)
constexpr uint32_t kAllocationSize_ConstantBufer_Vertex_Blur = 96;

constexpr uint32_t kAllocationSize_ConstantBuffer_Pixel_Gradient = 416;

/// @brief generally saying it is not universal approach but for keeping things not so much complex better to specify max amount of constantbuffer
/// that's enough to satisfy all shaders otherwise better to use only those which size is required for allocation
constexpr uint32_t kAllocationSizeMax_ConstantBuffer =
	std::max({kAllocationSize_ConstantBuffer_Vertex_Main, kAllocationSize_ConstantBufer_Vertex_Blur, kAllocationSize_ConstantBuffer_Pixel_Gradient});

	#define MAX_NUM_STOPS 16
	#define BLUR_SIZE 7
	#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

	#define RMLUI_STRINGIFY_IMPL(x) #x
	#define RMLUI_STRINGIFY(x) RMLUI_STRINGIFY_IMPL(x)

constexpr const char pShaderSourceText_Color[] = R"(
struct VS_INPUT
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

float4 main(const VS_INPUT IN) : SV_TARGET 
{ 
	return IN.color; 
}
)";
constexpr const char pShaderSourceText_Vertex[] = R"(
struct VS_INPUT 
{
	float2 position : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PS_OUTPUT
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 m_transform;
	float2 m_translate;
};

PS_OUTPUT main(const VS_INPUT IN)
{
	PS_OUTPUT OUT;

	float2 translatedPos = IN.position + m_translate;
	float4 resPos = mul(m_transform, float4(translatedPos.x, translatedPos.y, 0.0, 1.0));

	OUT.position = resPos;
	OUT.color = IN.color;
	OUT.uv = IN.uv;

	return OUT;
};
)";

constexpr const char pShaderSourceText_Texture[] = R"(
struct VS_INPUT
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D g_InputTexture : register(t0);

SamplerState g_SamplerLinear : register(s0);


float4 main(const VS_INPUT IN) : SV_TARGET 
{ 
	return IN.color * g_InputTexture.Sample(g_SamplerLinear, IN.uv); 
}
)";

constexpr const char pShaderSourceText_Vertex_PassThrough[] = R"(
struct VS_INPUT 
{
	float2 position : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PS_OUTPUT
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

PS_OUTPUT main(const VS_INPUT IN)
{
	PS_OUTPUT OUT;
	OUT.position = float4(IN.position.x, IN.position.y, 0.0f, 1.0f);
	OUT.color = IN.color;
	// need to flip here since Rml::MeshUtilities::GenerateQuad makes valid data for GL APIs but on DirectX 
	// it is not valid UV (otherwise image will be flipped)
	OUT.uv = float2(IN.uv.x, 1.0f - IN.uv.y); 

	return OUT;
}
)";

constexpr const char pShaderSourceText_Pixel_Passthrough[] = R"(
struct PS_INPUT
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D g_InputTexture : register(t0);

SamplerState g_SamplerLinear : register(s0);


float4 main(const PS_INPUT inputArgs) : SV_TARGET 
{ 
	return g_InputTexture.Sample(g_SamplerLinear, inputArgs.uv); 
}
)";

#define RMLUI_SHADER_HEADER "#define MAX_NUM_STOPS " RMLUI_STRINGIFY(MAX_NUM_STOPS) "\n#line " RMLUI_STRINGIFY(__LINE__) "\n"

	#define RMLUI_SHADER_BLUR_HEADER \
		RMLUI_SHADER_HEADER "\n#define BLUR_SIZE " RMLUI_STRINGIFY(BLUR_SIZE) "\n#define BLUR_NUM_WEIGHTS " RMLUI_STRINGIFY(BLUR_NUM_WEIGHTS)

constexpr const char pShaderSourceText_Vertex_Blur[] = RMLUI_SHADER_BLUR_HEADER R"(
struct VS_INPUT
{
    float2 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_Position;
    float2 uv[BLUR_SIZE] : TEXCOORD;
};

cbuffer SharedConstantBuffer : register(b0)
{
    float4x4 m_transform;
    float2 m_translate;
    float2 m_texelOffset;
    float4 m_weights;
    float2 m_texCoordMin;
    float2 m_texCoordMax;
};

PS_INPUT VSMain(const VS_INPUT IN)
{
    PS_INPUT result = (PS_INPUT)0;

    for (int i = 0; i < BLUR_SIZE; i++) {
        result.uv[i] = IN.uv - float(i - BLUR_NUM_WEIGHTS + 1) * m_texelOffset;
    }
    result.position = float4(IN.position.xy, 1.0, 1.0);

    return result;
};
)";

constexpr const char pShaderSourceText_Pixel_Blur[] = RMLUI_SHADER_BLUR_HEADER R"(
Texture2D g_InputTexture : register(t0);
SamplerState g_SamplerLinear : register(s0);

cbuffer SharedConstantBuffer : register(b0)
{
    float4x4 m_transform;
    float2 m_translate;
    float2 m_texelOffset;
    float4 m_weights;
    float2 m_texCoordMin;
    float2 m_texCoordMax;
};

struct PS_INPUT
{
    float4 position : SV_Position;
    float2 uv[BLUR_SIZE] : TEXCOORD;
};

float4 PSMain(const PS_INPUT IN) : SV_TARGET
{
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    for(int i = 0; i < BLUR_SIZE; i++)
    {
        float2 in_region = step(m_texCoordMin, IN.uv[i]) * step(IN.uv[i], m_texCoordMax);
        color += g_InputTexture.Sample(g_SamplerLinear, float2(IN.uv[i].x, 1.0f - IN.uv[i].y)) * in_region.x * in_region.y * m_weights[abs(i - BLUR_NUM_WEIGHTS + 1)];
    }
    return color;
};
)";

// AlignUp(314, 256) = 512
template <typename T>
static T AlignUp(T val, T alignment)
{
	RMLUI_ZoneScopedN("DirectX 12 - AlignUp");
	return (val + alignment - (T)1) & ~(alignment - (T)1);
}

static Rml::Colourf ConvertToColorf(Rml::ColourbPremultiplied c0)
{
	RMLUI_ZoneScopedN("DirectX 12 - ConvertToColorf");
	Rml::Colourf result;
	for (int i = 0; i < 4; i++)
		result[i] = (1.f / 255.f) * float(c0[i]);
	return result;
}

static Rml::Colourf ToPremultipliedAlpha(Rml::Colourb c0)
{
	RMLUI_ZoneScopedN("DirectX 12 - ToPremultipliedAlpha");
	Rml::Colourf result;
	result.alpha = (1.f / 255.f) * float(c0.alpha);
	result.red = (1.f / 255.f) * float(c0.red) * result.alpha;
	result.green = (1.f / 255.f) * float(c0.green) * result.alpha;
	result.blue = (1.f / 255.f) * float(c0.blue) * result.alpha;
	return result;
}

/// Flip vertical axis of the rectangle, and move its origin to the vertically opposite side of the viewport.
/// @note Changes coordinate system from RmlUi to OpenGL, or equivalently in reverse.
/// @note The Rectangle::Top and Rectangle::Bottom members will have reverse meaning in the returned rectangle.
static Rml::Rectanglei VerticallyFlipped(Rml::Rectanglei rect, int viewport_height)
{
	RMLUI_ZoneScopedN("DirectX 12 - VerticallyFlipped");
	RMLUI_ASSERT(rect.Valid());
	Rml::Rectanglei flipped_rect = rect;
	flipped_rect.p0.y = viewport_height - rect.p1.y;
	flipped_rect.p1.y = viewport_height - rect.p0.y;
	return flipped_rect;
}

static void SetBlurWeights(Rml::Vector4f& p_weights, float sigma)
{
	float weights[BLUR_NUM_WEIGHTS];
	float normalization = 0.0f;
	for (int i = 0; i < BLUR_NUM_WEIGHTS; i++)
	{
		if (Rml::Math::Absolute(sigma) < 0.1f)
			weights[i] = float(i == 0);
		else
			weights[i] = Rml::Math::Exp(-float(i * i) / (2.0f * sigma * sigma)) / (Rml::Math::SquareRoot(2.f * Rml::Math::RMLUI_PI) * sigma);

		normalization += (i == 0 ? 1.f : 2.0f) * weights[i];
	}
	for (int i = 0; i < BLUR_NUM_WEIGHTS; i++)
		weights[i] /= normalization;

	p_weights.x = weights[0];
	p_weights.y = weights[1];
	p_weights.z = weights[2];
	p_weights.w = weights[3];
}

static void SetTexCoordLimits(Rml::Vector2f& p_tex_coord_min, Rml::Vector2f& p_tex_coord_max, Rml::Rectanglei rectangle_flipped,
	Rml::Vector2i framebuffer_size)
{
	// Offset by half-texel values so that texture lookups are clamped to fragment centers, thereby avoiding color
	// bleeding from neighboring texels due to bilinear interpolation.
	const Rml::Vector2f min = (Rml::Vector2f(rectangle_flipped.p0) + Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);
	const Rml::Vector2f max = (Rml::Vector2f(rectangle_flipped.p1) - Rml::Vector2f(0.5f)) / Rml::Vector2f(framebuffer_size);

	p_tex_coord_min.x = min.x;
	p_tex_coord_min.y = min.y;
	p_tex_coord_max.x = max.x;
	p_tex_coord_max.y = max.y;
}


enum class ShaderGradientFunction { Linear, Radial, Conic, RepeatingLinear, RepeatingRadial, RepeatingConic }; // Must match shader definitions below.

enum class FilterType { Invalid = 0, Passthrough, Blur, DropShadow, ColorMatrix, MaskImage };
struct CompiledFilter {
	FilterType type;

	// Passthrough
	float blend_factor;

	// Blur
	float sigma;

	// Drop shadow
	Rml::Vector2f offset;
	Rml::Colourb color;

	// ColorMatrix
	Rml::Matrix4f color_matrix;
};

enum class CompiledShaderType { Invalid = 0, Gradient, Creation };
struct CompiledShader {
	CompiledShaderType type;

	// Gradient
	ShaderGradientFunction gradient_function;
	Rml::Vector2f p;
	Rml::Vector2f v;
	Rml::Vector<float> stop_positions;
	Rml::Vector<Rml::Colourf> stop_colors;

	// Shader
	Rml::Vector2f dimensions;
};

// those programs that have postfix as _MSAA it means that it accepts render target with sample count >= 2 (thus it is called MSAA)
enum class ProgramId : int {
	None,
	Color_Stencil_Always,
	Color_Stencil_Equal,
	Color_Stencil_Set,
	Color_Stencil_SetInverse,
	Color_Stencil_Intersect,
	Color_Stencil_Disabled,
	Texture_Stencil_Always,
	Texture_Stencil_Equal,
	Texture_Stencil_Disabled,
	Gradient,
	Creation,
	// this is for presenting our msaa render target texture for NO MSAA RT
	// if you do not correctly stuff DX12 validation will say about different
	// sample count like it is expected 1 (because no MSAA) but your RT target texture was created with
	// sample count = 2, so it is not correct way of using it
	Passthrough,
	Passthrough_MSAA,
	// glBlendFunc(GL_CONSTANT_ALPHA, GL_ZERO);
	Passthrough_ColorMask,
	Passthrough_NoBlend,
	ColorMatrix,
	BlendMask,
	Blur,
	DropShadow,
	Count
};

// Determines the anti-aliasing quality when creating layers. Enables better-looking visuals, especially when transforms are applied.
static constexpr int NUM_MSAA_SAMPLES = 2;

	#define RMLUI_PREMULTIPLIED_ALPHA 0

	#define MAX_NUM_STOPS 16
	#define BLUR_SIZE 7
	#define BLUR_NUM_WEIGHTS ((BLUR_SIZE + 1) / 2)

	#define RMLUI_STRINGIFY_IMPL(x) #x
	#define RMLUI_STRINGIFY(x) RMLUI_STRINGIFY_IMPL(x)

	#pragma comment(lib, "d3dcompiler.lib")
	#pragma comment(lib, "d3d12.lib")
	#pragma comment(lib, "dxgi.lib")

	#ifdef RMLUI_DX_DEBUG
		#pragma comment(lib, "dxguid.lib")
	#endif

	#ifdef RMLUI_DX_DEBUG
		// first argument specifies that input string is char if it is needed to use other languages than English then use u8 prefix
	    // example: RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "::RenderGeometry");
	    // example UTF8: RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, u8"::RenderGeometry");
		#define RMLUI_DX_MARKER_BEGIN(list, name) \
			if (list)                             \
				list->BeginEvent(1, reinterpret_cast<const char*>(name), sizeof(name));
		#define RMLUI_DX_MARKER_END(list) \
			if (list)                     \
				list->EndEvent();
	#else
		#define RMLUI_DX_MARKER_BEGIN(list, name)
		#define RMLUI_DX_MARKER_END(list)
	#endif

	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
namespace Gfx {

enum RenderState { Valid, Invalid_Scissor, Invalid_Unknown };

inline const char* convert_render_state_to_str(unsigned char state) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - convert_render_state_to_str");
	switch (static_cast<RenderState>(state))
	{
	case RenderState::Valid:
	{
		return "Valid";
	}
	case RenderState::Invalid_Scissor:
	{
		return "Invalid scissor";
	}
	case RenderState::Invalid_Unknown:
	{
		return "Unknown reason";
	}
	default:
	{
		return "Undefined state (report to developers!)";
	}
	}
}
} // namespace Gfx
	#endif

RenderInterface_DX12::RenderInterface_DX12(void* p_window_handle, ID3D12Device* p_user_device, IDXGISwapChain* p_user_swapchain, bool use_vsync,
	unsigned char msaa_sample_count) :
	m_is_full_initialization{false}, m_is_shutdown_called{}, m_is_use_vsync{use_vsync}, m_is_use_tearing{}, m_is_scissor_was_set{},
	m_is_stencil_enabled{}, m_is_stencil_equal{}, m_is_use_msaa{true}, m_msaa_sample_count{msaa_sample_count},
	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
	m_render_state{static_cast<unsigned char>(Gfx::RenderState::Valid)},
	#endif
	m_width{}, m_height{}, m_current_clip_operation{-1}, m_active_program_id{}, m_size_descriptor_heap_render_target_view{},
	m_size_descriptor_heap_shaders{}, m_p_descriptor_heap_shaders{}, m_current_back_buffer_index{}, m_stencil_ref_value{}
{
	RMLUI_ZoneScopedN("DirectX 12 - Constructor (user)");
	RMLUI_ASSERT(p_window_handle && "you can't pass an empty window handle! (also it must be castable to HWND)");
	RMLUI_ASSERT(p_user_device && "you can't pass an empty device!");
	RMLUI_ASSERT(p_user_swapchain && "you can't pass an empty swapchain!");
	RMLUI_ASSERT((sizeof(this->m_pipelines) / sizeof(this->m_pipelines[0])) == static_cast<int>(ProgramId::Count) &&
		"you didn't update size of your variable");
	RMLUI_ASSERT((sizeof(this->m_root_signatures) / sizeof(this->m_root_signatures[0])) == static_cast<int>(ProgramId::Count) &&
		"you didn't update size of your variable");
	this->m_p_window_handle = static_cast<HWND>(p_window_handle);

	std::memset(this->m_pipelines, 0, sizeof(this->m_pipelines));
	std::memset(this->m_root_signatures, 0, sizeof(this->m_root_signatures));
	std::memset(this->m_constant_buffer_count_per_frame.data(), 0, sizeof(this->m_constant_buffer_count_per_frame));

	#ifdef RMLUI_DX_DEBUG
	this->m_default_shader_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
	this->m_default_shader_flags = 0;
	#endif
}

namespace Gfx {
struct FramebufferData {
public:
	FramebufferData() :
	#ifdef RMLUI_DEBUG
		m_is_allocated_on_stack{true},
	#endif
		m_is_render_target{true}, m_width{}, m_height{}, m_id{-1}, m_p_texture{}, m_p_texture_depth_stencil{}, m_texture_descriptor_resource_view{},
		m_allocation_descriptor_offset{}
	{}

	// overload because we don't need to copy information about how our object was allocated (from storage or on stack, see m_is_allocated_on_stack)
	FramebufferData(const FramebufferData& data) :
		m_is_render_target{data.m_is_render_target}, m_width{data.m_width}, m_height{data.m_height}, m_id{data.m_id}, m_p_texture{data.m_p_texture},
		m_p_texture_depth_stencil{data.m_p_texture_depth_stencil}, m_texture_descriptor_resource_view{data.m_texture_descriptor_resource_view},
		m_allocation_descriptor_offset{data.m_allocation_descriptor_offset}
	{}

	~FramebufferData()
	{
		m_id = -1;

	#ifdef RMLUI_DEBUG
		if (m_is_allocated_on_stack == false)
			RMLUI_ASSERT(this->m_p_texture == nullptr && "you must manually deallocate texture and set this field as nullptr!");
	#endif
	}

	int Get_Width() const { return this->m_width; }
	void Set_Width(int value) { this->m_width = value; }

	int Get_Height(void) const { return this->m_height; }
	void Set_Height(int value) { this->m_height = value; }

	void Set_ID(int layer_current_size_index) { this->m_id = layer_current_size_index; }
	int Get_ID(void) const { return this->m_id; }

	void Set_Texture(RenderInterface_DX12::TextureHandleType* p_texture) { this->m_p_texture = p_texture; }
	RenderInterface_DX12::TextureHandleType* Get_Texture(void) const { return this->m_p_texture; }

	// ResourceView means RTV or DSV
	void Set_DescriptorResourceView(const D3D12_CPU_DESCRIPTOR_HANDLE& handle) { this->m_texture_descriptor_resource_view = handle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& Get_DescriptorResourceView() const { return this->m_texture_descriptor_resource_view; }

	void Set_SharedDepthStencilTexture(FramebufferData* p_data) { this->m_p_texture_depth_stencil = p_data; }
	FramebufferData* Get_SharedDepthStencilTexture(void) const { return this->m_p_texture_depth_stencil; }

	D3D12MA::VirtualAllocation* Get_VirtualAllocation_Descriptor() { return &this->m_allocation_descriptor_offset; }

	bool Is_RenderTarget(void) const { return this->m_is_render_target; }
	void Set_RenderTarget(bool value) { this->m_is_render_target = value; }

	// overload because we don't need to copy information about how our object was allocated (from storage or on stack, see m_is_allocated_on_stack)
	inline FramebufferData& operator=(const FramebufferData& data)
	{
		this->m_is_render_target = data.m_is_render_target;
		this->m_width = data.m_width;
		this->m_height = data.m_height;
		this->m_id = data.m_id;
		this->m_p_texture = data.m_p_texture;
		this->m_p_texture_depth_stencil = data.m_p_texture_depth_stencil;
		this->m_texture_descriptor_resource_view = data.m_texture_descriptor_resource_view;
		this->m_allocation_descriptor_offset = data.m_allocation_descriptor_offset;

		return *this;
	}

	// overload because we don't need to copy information about how our object was allocated (from storage or on stack, see m_is_allocated_on_stack)
	inline FramebufferData& operator=(FramebufferData&& data)
	{
		this->m_is_render_target = data.m_is_render_target;
		this->m_width = data.m_width;
		this->m_height = data.m_height;
		this->m_id = data.m_id;
		this->m_p_texture = data.m_p_texture;
		this->m_p_texture_depth_stencil = data.m_p_texture_depth_stencil;
		this->m_texture_descriptor_resource_view = data.m_texture_descriptor_resource_view;
		this->m_allocation_descriptor_offset = data.m_allocation_descriptor_offset;

		data.m_is_render_target = true;
		data.m_width = 0;
		data.m_height = 0;
		data.m_id = -1;
		data.m_p_texture = nullptr;
		data.m_p_texture_depth_stencil = nullptr;
		data.m_texture_descriptor_resource_view = D3D12_CPU_DESCRIPTOR_HANDLE();
		data.m_allocation_descriptor_offset = D3D12MA::VirtualAllocation();

		return *this;
	}

	#ifdef RMLUI_DEBUG
	// in order to prevent false triggering assert in destructor we determine if object was created on stack or not
	// if not we manually set this field to false
	bool m_is_allocated_on_stack;
	#endif

private:
	bool m_is_render_target;
	int m_width;
	int m_height;
	int m_id;
	RenderInterface_DX12::TextureHandleType* m_p_texture;
	// this is shared texture and original pointer stored and managed at renderlayerstack class
	FramebufferData* m_p_texture_depth_stencil;
	D3D12_CPU_DESCRIPTOR_HANDLE m_texture_descriptor_resource_view;
	// from dsv or rtv heap!!
	D3D12MA::VirtualAllocation m_allocation_descriptor_offset;
};
} // namespace Gfx

RenderInterface_DX12::RenderInterface_DX12(void* p_window_handle, bool use_vsync, unsigned char msaa_sample_count) :
	m_is_full_initialization{true}, m_is_shutdown_called{}, m_is_use_vsync{use_vsync}, m_is_use_tearing{}, m_is_scissor_was_set{},
	m_is_stencil_enabled{}, m_is_stencil_equal{}, m_is_use_msaa{true}, m_msaa_sample_count{msaa_sample_count},
	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
	m_render_state{static_cast<unsigned char>(Gfx::RenderState::Valid)},
	#endif
	m_width{}, m_height{}, m_current_clip_operation{-1}, m_active_program_id{}, m_size_descriptor_heap_render_target_view{},
	m_size_descriptor_heap_shaders{}, m_current_back_buffer_index{}, m_stencil_ref_value{}, m_p_device{}, m_p_command_queue{}, m_p_copy_queue{},
	m_p_swapchain{}, m_p_command_graphics_list{}, m_p_descriptor_heap_render_target_view{},
	m_p_descriptor_heap_render_target_view_for_texture_manager{}, m_p_descriptor_heap_depth_stencil_view_for_texture_manager{},
	m_p_descriptor_heap_shaders{}, m_p_descriptor_heap_depthstencil{}, m_p_depthstencil_resource{}, m_p_backbuffer_fence{}, m_p_adapter{},
	m_p_copy_allocator{}, m_p_copy_command_list{}, m_p_allocator{}, m_p_offset_allocator_for_descriptor_heap_shaders{}, m_p_window_handle{},
	m_p_fence_event{}, m_fence_value{}, m_precompiled_fullscreen_quad_geometry{}
{
	RMLUI_ZoneScopedN("DirectX 12 - Constructor (non user)");

	RMLUI_ASSERT(p_window_handle && "you can't pass an empty window handle! (also it must be castable to HWND)");
	RMLUI_ASSERT(RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT >= 1 && "must be non zero!");
	RMLUI_ASSERT((sizeof(this->m_pipelines) / sizeof(this->m_pipelines[0])) == static_cast<int>(ProgramId::Count) &&
		"you didn't update size of your variable");
	RMLUI_ASSERT((sizeof(this->m_root_signatures) / sizeof(this->m_root_signatures[0])) == static_cast<int>(ProgramId::Count) &&
		"you didn't update size of your variable");

	this->m_p_window_handle = static_cast<HWND>(p_window_handle);

	std::memset(this->m_pipelines, 0, sizeof(this->m_pipelines));
	std::memset(this->m_root_signatures, 0, sizeof(this->m_root_signatures));
	std::memset(this->m_constant_buffer_count_per_frame.data(), 0, sizeof(this->m_constant_buffer_count_per_frame));

	#ifdef RMLUI_DX_DEBUG
	this->m_default_shader_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
	this->m_default_shader_flags = 0;
	#endif
}

RenderInterface_DX12::~RenderInterface_DX12()
{
	RMLUI_ZoneScopedN("DirectX 12 - Destructor");
	RMLUI_ASSERT(this->m_is_shutdown_called == true && "something is wrong and you didn't provide a calling for shutdown method outside of class!");
	RMLUI_ASSERT(RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT >= 1 && "must be non zero!");

	// library developers must gurantee that shutdown was called, but if it is not we gurantee that shutdown will be called in destructor
	if (!this->m_is_shutdown_called)
	{
		this->Shutdown();
	}
}

// TODO: finish complex checking of all managers not just only device
RenderInterface_DX12::operator bool() const
{
	RMLUI_ZoneScopedN("DirectX 12 - operator bool()");
	return !!(this->m_p_device);
}

void RenderInterface_DX12::SetViewport(int viewport_width, int viewport_height)
{
	RMLUI_ZoneScopedN("DirectX 12 - SetViewport");

	if (this->m_width != viewport_width || this->m_height != viewport_height)
	{
		this->Flush();

		if (this->m_p_depthstencil_resource)
		{
			this->Destroy_Resource_DepthStencil();
		}

		m_width = viewport_width;
		m_height = viewport_height;

		this->m_projection =
			Rml::Matrix4f::ProjectOrtho(0, static_cast<float>(viewport_width), static_cast<float>(viewport_height), 0, -10000, 10000);

		if (this->m_p_swapchain)
		{
			// TODO: think how to tell user for recreating swapchain on his side, providing callback???
			if (this->m_is_full_initialization)
			{
				this->Destroy_Resources_DependentOnSize();

				for (int i = 0; i < RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT; ++i)
				{
					this->m_backbuffers_fence_values[i] = this->m_backbuffers_fence_values[this->m_current_back_buffer_index];
				}

				DXGI_SWAP_CHAIN_DESC desc;
				RMLUI_DX_ASSERTMSG(this->m_p_swapchain->GetDesc(&desc), "failed to GetDesc");
				RMLUI_DX_ASSERTMSG(this->m_p_swapchain->ResizeBuffers(static_cast<UINT>(RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT),
									   static_cast<UINT>(this->m_width), static_cast<UINT>(this->m_height), desc.BufferDesc.Format, desc.Flags),
					"failed to ResizeBuffers");

				this->m_current_back_buffer_index = this->m_p_swapchain->GetCurrentBackBufferIndex();

				this->Create_Resources_DependentOnSize();
				this->Create_Resource_DepthStencil();
			}
		}
	}
}

void RenderInterface_DX12::BeginFrame()
{
	RMLUI_ZoneScopedN("DirectX 12 - BeginFrame");
	//	this->Update_PendingForDeletion_Texture();
	this->Update_PendingForDeletion_Geometry();

	auto* p_command_allocator = this->m_backbuffers_allocators.at(this->m_current_back_buffer_index);

	RMLUI_ASSERT(p_command_allocator && "should be allocated and initialized! Probably early calling");
	RMLUI_ASSERT(this->m_p_command_graphics_list && "must be allocated and initialized! Probably early calling");

	if (p_command_allocator)
	{
		p_command_allocator->Reset();
	}

	if (this->m_p_command_graphics_list)
	{
		this->m_p_command_graphics_list->Reset(p_command_allocator, nullptr);

		RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "BeginFrame");

	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
		if (static_cast<Gfx::RenderState>(this->m_render_state) != Gfx::RenderState::Valid)
		{
		#ifdef RMLUI_DEBUG
			Rml::Log::Message(Rml::Log::LT_WARNING, "[RenderInterface_DX12::BeginFrame] you have invalid rendering state: %s",
				Gfx::convert_render_state_to_str(this->m_render_state));
		#endif

			this->m_manager_render_layer.BeginFrame(this->m_width, this->m_height);
			return;
		}
	#endif

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle_rtv(this->m_p_descriptor_heap_render_target_view->GetCPUDescriptorHandleForHeapStart(),
			this->m_current_back_buffer_index, this->m_size_descriptor_heap_render_target_view);

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle_dsv(this->m_p_descriptor_heap_depthstencil->GetCPUDescriptorHandleForHeapStart());

		this->m_stencil_ref_value = 0;

		// TODO: я не должен устанавливать здесь backbuffer который из свапчейна, а смотреть код GL3 реализации
		//	this->m_p_command_graphics_list->OMSetRenderTargets(1, &handle_rtv, FALSE, &handle_dsv);

		D3D12_RECT scissor;

		/*
		if (!this->m_is_scissor_was_set)
		{
		    scissor.left = 0;
		    scissor.top = 0;
		    scissor.right = this->m_width;
		    scissor.bottom = this->m_height;
		}
		else
		{
		    RMLUI_ASSERT(this->m_scissor.Valid() &&
		        "must be valid! it means that SetScissor was called and we apply these changes and scissor always set valid region, if it was called "
		        "by makeinvalid we just use default (directx-12 forced for using scissor test always so we always have to set scissors)");

		    scissor.left = this->m_scissor.Left();
		    scissor.right = this->m_width;
		    scissor.bottom = this->m_height;
		    scissor.top = this->m_height - this->m_scissor.Bottom();
		    this->m_is_scissor_was_set = false;
		}
		*/

		if (this->m_is_scissor_was_set)
		{
			this->m_is_scissor_was_set = false;
		}

		//
		//	this->m_is_scissor_was_set = false;
		this->SetTransform(nullptr);

		this->m_manager_render_layer.BeginFrame(this->m_width, this->m_height);

		//		auto* p_handle_rtv = &this->m_manager_render_layer.GetTopLayer().Get_DescriptorResourceView();
		//		auto* p_handle_dsv = &this->m_manager_render_layer.GetTopLayer().Get_SharedDepthStencilTexture()->Get_DescriptorResourceView();

		//		this->m_p_command_graphics_list->OMSetRenderTargets(1, p_handle_rtv, FALSE, p_handle_dsv);

		this->BindRenderTarget(this->m_manager_render_layer.GetTopLayer());

		D3D12_VIEWPORT viewport{};
		viewport.Height = static_cast<FLOAT>(this->m_height);
		viewport.Width = static_cast<FLOAT>(this->m_width);
		viewport.MaxDepth = 1.0f;
		this->m_p_command_graphics_list->RSSetViewports(1, &viewport);

		this->UseProgram(ProgramId::None);
		this->m_is_stencil_equal = false;
		//	this->m_program_state_transform_dirty.set();

		RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
	}
}

void RenderInterface_DX12::EndFrame()
{
	RMLUI_ZoneScopedN("DirectX 12 - EndFrame");

	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
	// this was implemented due to nature of verbose validation layers of DirectX 12 (I suppose the same thing in Vulkan) and if we get different
	// invalid states we don't need to render them but in real developers need to resolve a such situation and don't issue rendering at all so it is
	// some kind of "hack" but tbh getting verbose assertion on driver is not good thing at all a such situation was arised when scissor comes from
	// Rml side as invalid and instead of having valid rectengular region we get a line (y0==y1)
	if (static_cast<Gfx::RenderState>(this->m_render_state) != Gfx::RenderState::Valid)
	{
		#ifdef RMLUI_DEBUG
		Rml::Log::Message(Rml::Log::LT_WARNING, "[RenderInterface_DX12::EndFrame] you have invalid rendering state: %s",
			Gfx::convert_render_state_to_str(this->m_render_state));
		#endif

		// suppose that next frame will be valid and thus we make our state valid
		// since we didn't execute "invalid" rendering state
		this->m_render_state = static_cast<unsigned char>(Gfx::RenderState::Valid);

		if (this->m_p_command_graphics_list)
		{
			RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "EndFrame");
			RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
			RMLUI_DX_ASSERTMSG(this->m_p_command_graphics_list->Close(), "failed to Close");
		}
		this->m_manager_render_layer.EndFrame();
		return;
	}
	#endif

	auto* p_resource_backbuffer = this->m_backbuffers_resources.at(this->m_current_back_buffer_index);

	RMLUI_ASSERT(p_resource_backbuffer && "should be allocated and initialized! Probably early calling");
	RMLUI_ASSERT(this->m_p_command_graphics_list && "Must be allocated and initialzied. Probably early calling!");
	RMLUI_ASSERT(this->m_p_command_queue && "Must be allocated and initialzied. Probably early calling!");

	// TODO: здесь я должен выставлять backbuffer из свапчейна и при этом делать резолв для последнего постпроцесса (см. код реализации GL3)
	if (this->m_p_command_graphics_list)
	{
		RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "EndFrame");
		CD3DX12_RESOURCE_BARRIER backbuffer_barrier_from_rt_to_present =
			CD3DX12_RESOURCE_BARRIER::Transition(p_resource_backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		// делаем резовл из лейра для постпроцесса
		// делаем OMSetRenderTargets на наш swapchain's backbuffer
		// рендеринг квад и натягиваем результат постпроцесса (резолв)
		// закрываем запись команд делаем презент

		const Gfx::FramebufferData& fb_active = this->m_manager_render_layer.GetTopLayer();
		const Gfx::FramebufferData& fb_postprocess = this->m_manager_render_layer.GetPostprocessPrimary();

		ID3D12Resource* p_msaa_texture{};
		ID3D12Resource* p_postprocess_texture{};

		TextureHandleType* p_handle_postprocess_texture = fb_postprocess.Get_Texture();

		if (fb_active.Get_Texture())
		{
			TextureHandleType* p_resource = fb_active.Get_Texture();
			RMLUI_ASSERT(p_resource->Get_Info().Get_BufferIndex() == -1 && "can't be allocated as placed resource no sense!");

			D3D12MA::Allocation* p_allocation = static_cast<D3D12MA::Allocation*>(p_resource->Get_Resource());
			p_msaa_texture = p_allocation->GetResource();
		}

		if (fb_postprocess.Get_Texture())
		{
			TextureHandleType* p_resource = fb_postprocess.Get_Texture();
			RMLUI_ASSERT(p_resource->Get_Info().Get_BufferIndex() == -1 && "can't be allocated as place resource no sense!");

			D3D12MA::Allocation* p_allocation = static_cast<D3D12MA::Allocation*>(p_resource->Get_Resource());
			p_postprocess_texture = p_allocation->GetResource();
		}

		RMLUI_ASSERT(p_msaa_texture && "can't be, must be a valid texture!");
		RMLUI_ASSERT(p_postprocess_texture && "can't be, must be a valid texture!");
		RMLUI_ASSERT(p_handle_postprocess_texture && "must be valid!");

		CD3DX12_RESOURCE_BARRIER barriers[2]{};
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(p_msaa_texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

		barriers[1] =
			CD3DX12_RESOURCE_BARRIER::Transition(p_postprocess_texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_DEST);

		CD3DX12_RESOURCE_BARRIER barrier_transition_from_msaa_resolve_source_to_rt =
			CD3DX12_RESOURCE_BARRIER::Transition(p_msaa_texture, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		this->m_p_command_graphics_list->ResourceBarrier(2, barriers);

		this->m_p_command_graphics_list->ResolveSubresource(p_postprocess_texture, 0, p_msaa_texture, 0,
			RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT);

		this->m_p_command_graphics_list->ResourceBarrier(1, &barrier_transition_from_msaa_resolve_source_to_rt);

		CD3DX12_RESOURCE_BARRIER offscreen_texture_barrier_for_shader = CD3DX12_RESOURCE_BARRIER::Transition(p_postprocess_texture,
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		CD3DX12_RESOURCE_BARRIER restore_state_of_postprocess_texture_return_to_rt = CD3DX12_RESOURCE_BARRIER::Transition(p_postprocess_texture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		this->m_p_command_graphics_list->ResourceBarrier(1, &offscreen_texture_barrier_for_shader);

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle_rtv(this->m_p_descriptor_heap_render_target_view->GetCPUDescriptorHandleForHeapStart(),
			this->m_current_back_buffer_index, this->m_size_descriptor_heap_render_target_view);
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle_dsv(this->m_p_descriptor_heap_depthstencil->GetCPUDescriptorHandleForHeapStart());

		this->m_p_command_graphics_list->OMSetRenderTargets(1, &handle_rtv, FALSE, &handle_dsv);

		this->UseProgram(ProgramId::Passthrough);

		this->BindTexture(p_handle_postprocess_texture);

		this->DrawFullscreenQuad();

		this->m_manager_render_layer.EndFrame();

		this->m_p_command_graphics_list->ResourceBarrier(1, &backbuffer_barrier_from_rt_to_present);
		this->m_p_command_graphics_list->ResourceBarrier(1, &restore_state_of_postprocess_texture_return_to_rt);

		RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

		RMLUI_DX_ASSERTMSG(this->m_p_command_graphics_list->Close(), "failed to Close");

		ID3D12CommandList* const lists[] = {this->m_p_command_graphics_list};

		if (this->m_p_command_queue)
		{
			this->m_p_command_queue->ExecuteCommandLists(_countof(lists), lists);
		}

		auto fence_value = this->Signal();

		UINT sync_interval = this->m_is_use_vsync ? 1 : 0;
		UINT present_flags = (this->m_is_use_tearing && !this->m_is_use_vsync) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		RMLUI_DX_ASSERTMSG(this->m_p_swapchain->Present(sync_interval, present_flags), "failed to Present");

	#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] current allocated constant buffers per draw (for frame[%d]): %zu",
			this->m_current_back_buffer_index, this->m_constant_buffer_count_per_frame[m_current_back_buffer_index]);
	#endif

		this->m_constant_buffer_count_per_frame[this->m_current_back_buffer_index] = 0;

		this->m_current_back_buffer_index = (uint32_t)this->m_p_swapchain->GetCurrentBackBufferIndex();

		this->WaitForFenceValue(fence_value);
	}
}

void RenderInterface_DX12::Clear()
{
	RMLUI_ZoneScopedN("DirectX 12 - Clear");

	RMLUI_ASSERT(this->m_p_command_graphics_list && "early calling prob!");

	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "Clear");

	auto* p_backbuffer = this->m_backbuffers_resources.at(this->m_current_back_buffer_index);

	CD3DX12_RESOURCE_BARRIER barrier =
		CD3DX12_RESOURCE_BARRIER::Transition(p_backbuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	this->m_p_command_graphics_list->ResourceBarrier(1, &barrier);

	constexpr FLOAT clear_color[] = {RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VAlUE};
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(this->m_p_descriptor_heap_render_target_view->GetCPUDescriptorHandleForHeapStart(),
		this->m_current_back_buffer_index, this->m_size_descriptor_heap_render_target_view);

	this->m_p_command_graphics_list->ClearDepthStencilView(this->m_p_descriptor_heap_depthstencil->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	this->m_p_command_graphics_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);

	auto& p_current_rtv = this->m_manager_render_layer.GetTopLayer().Get_DescriptorResourceView();
	auto& p_current_dsv = this->m_manager_render_layer.GetTopLayer().Get_SharedDepthStencilTexture()->Get_DescriptorResourceView();

	constexpr FLOAT clear_color_framebuffer[] = {0.0f, 0.0f, 0.0f, 0.0f};
	this->m_p_command_graphics_list->ClearRenderTargetView(p_current_rtv, clear_color_framebuffer, 0, nullptr);
//	this->m_p_command_graphics_list->ClearDepthStencilView(p_current_dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderGeometry");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "RenderGeometry");

	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
	if (static_cast<Gfx::RenderState>(this->m_render_state) != Gfx::RenderState::Valid)
	{
		#ifdef RMLUI_DEBUG
		Rml::Log::Message(Rml::Log::LT_WARNING, "[RenderInterface_DX12::RenderGeometry] you have invalid rendering state: %s",
			Gfx::convert_render_state_to_str(this->m_render_state));
		#endif

		RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
		return;
	}
	#endif

	GeometryHandleType* p_handle_geometry = reinterpret_cast<GeometryHandleType*>(geometry);
	TextureHandleType* p_handle_texture{};

	RMLUI_ASSERT(p_handle_geometry && "expected valid pointer!");

	ConstantBufferType* p_constant_buffer{};
	if (texture == TexturePostprocess)
	{}
	else if (texture)
	{
		p_handle_texture = reinterpret_cast<TextureHandleType*>(texture);
		RMLUI_ASSERT(p_handle_texture && "expected valid pointer!");

		if (this->m_is_stencil_enabled)
		{
			if (this->m_is_stencil_equal)
			{
				this->UseProgram(ProgramId::Texture_Stencil_Equal);
			}
			else
			{
				this->UseProgram(ProgramId::Texture_Stencil_Always);
			}
		}
		else
		{
			this->UseProgram(ProgramId::Texture_Stencil_Disabled);
		}

		// this->SubmitTransformUniform(p_handle_geometry->Get_ConstantBuffer(), translation);

		p_constant_buffer = this->Get_ConstantBuffer(this->m_current_back_buffer_index);

		this->SubmitTransformUniform(*p_constant_buffer, translation);

		if (texture != TextureEnableWithoutBinding)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE srv_handle;
			srv_handle.ptr = this->m_p_descriptor_heap_shaders->GetGPUDescriptorHandleForHeapStart().ptr +
				p_handle_texture->Get_Allocation_DescriptorHeap().offset;

			this->m_p_command_graphics_list->SetGraphicsRootDescriptorTable(1, srv_handle);
		}
	}
	else
	{
		if (this->m_current_clip_operation == -1)
		{
			if (this->m_is_stencil_enabled)
			{
				if (this->m_is_stencil_equal)
				{
					this->UseProgram(ProgramId::Color_Stencil_Equal);
				}
				else
				{
					this->UseProgram(ProgramId::Color_Stencil_Always);
				}
			}
			else
			{
				this->UseProgram(ProgramId::Color_Stencil_Disabled);
			}
		}
		else if (this->m_current_clip_operation == static_cast<int>(Rml::ClipMaskOperation::Intersect))
		{
			if (this->m_is_stencil_enabled)
			{
				this->UseProgram(ProgramId::Color_Stencil_Intersect);
			}
		}
		else if (this->m_current_clip_operation == static_cast<int>(Rml::ClipMaskOperation::Set))
		{
			if (this->m_is_stencil_enabled)
			{
				this->UseProgram(ProgramId::Color_Stencil_Set);
			}
		}
		else if (this->m_current_clip_operation == static_cast<int>(Rml::ClipMaskOperation::SetInverse))
		{
			if (this->m_is_stencil_enabled)
			{
				this->UseProgram(ProgramId::Color_Stencil_SetInverse);
			}
		}
		else
		{
			RMLUI_ASSERT(!"not reached code point, something is missing or corrupted data"[0]);
		}

		// this->SubmitTransformUniform(p_handle_geometry->Get_ConstantBuffer(), translation);
		p_constant_buffer = this->Get_ConstantBuffer(this->m_current_back_buffer_index);

		this->SubmitTransformUniform(*p_constant_buffer, translation);
	}

	if (!this->m_is_scissor_was_set)
	{
		D3D12_RECT scissor;
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = this->m_width;
		scissor.bottom = this->m_height;

		this->m_p_command_graphics_list->RSSetScissorRects(1, &scissor);
		OutputDebugStringA("GraphicsQueue::RSSetScissorRects | !this->m_is_scissor_was_set\n");
	}

	if (p_constant_buffer)
	{
		auto* p_dx_constant_buffer = this->m_manager_buffer.Get_BufferByIndex(p_constant_buffer->Get_AllocInfo().Get_BufferIndex());
		RMLUI_ASSERT(p_dx_constant_buffer && "must be valid!");

		if (p_dx_constant_buffer)
		{
			auto* p_dx_resource = p_dx_constant_buffer->GetResource();

			RMLUI_ASSERT(p_dx_resource && "must be valid!");

			if (p_dx_resource)
			{
				this->m_p_command_graphics_list->SetGraphicsRootConstantBufferView(0,
					p_dx_resource->GetGPUVirtualAddress() + p_constant_buffer->Get_AllocInfo().Get_Offset());
			}
		}
	}

	auto* p_dx_buffer_vertex = this->m_manager_buffer.Get_BufferByIndex(p_handle_geometry->Get_InfoVertex().Get_BufferIndex());

	RMLUI_ASSERT(p_dx_buffer_vertex && "must be valid!");

	this->m_p_command_graphics_list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (p_dx_buffer_vertex)
	{
		auto* p_dx_resource = p_dx_buffer_vertex->GetResource();

		RMLUI_ASSERT(p_dx_resource && "must be valid!");

		if (p_dx_resource)
		{
			D3D12_VERTEX_BUFFER_VIEW view_vertex_buffer = {};

			view_vertex_buffer.BufferLocation = p_dx_resource->GetGPUVirtualAddress() + p_handle_geometry->Get_InfoVertex().Get_Offset();
			view_vertex_buffer.StrideInBytes = sizeof(Rml::Vertex);
			view_vertex_buffer.SizeInBytes = static_cast<UINT>(p_handle_geometry->Get_InfoVertex().Get_Size());

			this->m_p_command_graphics_list->IASetVertexBuffers(0, 1, &view_vertex_buffer);
		}
	}

	auto* p_dx_buffer_index = this->m_manager_buffer.Get_BufferByIndex(p_handle_geometry->Get_InfoIndex().Get_BufferIndex());

	RMLUI_ASSERT(p_dx_buffer_index && "must be valid!");

	if (p_dx_buffer_index)
	{
		auto* p_dx_resource = p_dx_buffer_index->GetResource();

		RMLUI_ASSERT(p_dx_resource && "must be valid!");

		if (p_dx_resource)
		{
			D3D12_INDEX_BUFFER_VIEW view_index_buffer = {};

			view_index_buffer.BufferLocation = p_dx_resource->GetGPUVirtualAddress() + p_handle_geometry->Get_InfoIndex().Get_Offset();
			view_index_buffer.Format = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
			view_index_buffer.SizeInBytes = p_handle_geometry->Get_InfoIndex().Get_Size();

			this->m_p_command_graphics_list->IASetIndexBuffer(&view_index_buffer);
		}
	}

	this->m_p_command_graphics_list->DrawIndexedInstanced(p_handle_geometry->Get_NumIndecies(), 1, 0, 0, 0);

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

Rml::CompiledGeometryHandle RenderInterface_DX12::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	RMLUI_ZoneScopedN("DirectX 12 - CompileGeometry");
	//	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "CompileGeometry");

	GeometryHandleType* p_handle = new GeometryHandleType();

	if (p_handle)
	{
		this->m_manager_buffer.Alloc_Vertex(vertices.data(), static_cast<int>(vertices.size()), sizeof(Rml::Vertex), p_handle);
		this->m_manager_buffer.Alloc_Index(indices.data(), static_cast<int>(indices.size()), sizeof(int), p_handle);
		//	p_handle->Get_ConstantBuffer().Set_AllocInfo(this->m_manager_buffer.Alloc_ConstantBuffer(&p_handle->Get_ConstantBuffer(), 72));
	}
	//	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

	return reinterpret_cast<Rml::CompiledGeometryHandle>(p_handle);
}

void RenderInterface_DX12::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	RMLUI_ZoneScopedN("DirectX 12 - ReleaseGeometry");
	GeometryHandleType* p_handle = reinterpret_cast<GeometryHandleType*>(geometry);
	this->m_pending_for_deletion_geometry.push_back(p_handle);
}

void RenderInterface_DX12::EnableScissorRegion(bool enable)
{
	RMLUI_ZoneScopedN("DirectX 12 - EnableScissorRegion");

	OutputDebugStringA("::EnableScissorRegion\n");
	if (!enable)
	{
		OutputDebugStringA("\t");
		RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "EnableScissorRegion");
		SetScissor(Rml::Rectanglei::MakeInvalid(), false);
		RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
	}
}

void RenderInterface_DX12::SetScissorRegion(Rml::Rectanglei region)
{
	RMLUI_ZoneScopedN("DirectX 12 - SetScissorRegion");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "SetScissorRegion");

	static int call_count = 0;
	++call_count;
	char msg[32];
	std::sprintf(msg, "::SetScissorRegion=%d\n\t", call_count);
	OutputDebugStringA(msg);

	SetScissor(region);

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::EnableClipMask(bool enable)
{
	RMLUI_ZoneScopedN("DirectX 12 - EnableClipMask");

	this->m_is_stencil_enabled = enable;

	if (enable)
	{
		RMLUI_ASSERT(this->m_p_command_graphics_list && "must be valid!");
		if (this->m_p_command_graphics_list)
		{
			this->m_p_command_graphics_list->OMSetStencilRef(m_stencil_ref_value);
		}
	}
	else
	{
		RMLUI_ASSERT(this->m_p_command_graphics_list && "mmust be valid!");
		if (this->m_p_command_graphics_list)
		{
			this->m_p_command_graphics_list->OMSetStencilRef(0);
		}
	}
}

void RenderInterface_DX12::RenderToClipMask(Rml::ClipMaskOperation mask_operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderToClipMask");
	RMLUI_ASSERT(this->m_is_stencil_enabled && "must be enabled!");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "RenderToClipMask");

	const bool clear_stencil = (mask_operation == Rml::ClipMaskOperation::Set || mask_operation == Rml::ClipMaskOperation::SetInverse);

	if (clear_stencil)
	{
		// todo: probably I need to remove DS for swapchain's RT
	//	this->m_p_command_graphics_list->ClearDepthStencilView(this->m_p_descriptor_heap_depthstencil->GetCPUDescriptorHandleForHeapStart(),
		//	D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		Rml::LayerHandle layer_handle = m_manager_render_layer.GetTopLayerHandle();
		const Gfx::FramebufferData& framebuffer = m_manager_render_layer.GetLayer(layer_handle);

		this->m_p_command_graphics_list->ClearDepthStencilView(framebuffer.Get_SharedDepthStencilTexture()->Get_DescriptorResourceView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	}

	switch (mask_operation)
	{
	case Rml::ClipMaskOperation::Set:
	{
		this->m_current_clip_operation = static_cast<int>(Rml::ClipMaskOperation::Set);
		//	this->m_p_command_graphics_list->OMSetStencilRef(1);
		m_stencil_ref_value = 1;
		break;
	}
	case Rml::ClipMaskOperation::SetInverse:
	{
		this->m_current_clip_operation = static_cast<int>(Rml::ClipMaskOperation::SetInverse);
		//	this->m_p_command_graphics_list->OMSetStencilRef(0);
		m_stencil_ref_value = 0;
		break;
	}
	case Rml::ClipMaskOperation::Intersect:
	{
		this->m_current_clip_operation = static_cast<int>(Rml::ClipMaskOperation::Intersect);
		m_stencil_ref_value += 1;
		break;
	}
	}

	this->m_p_command_graphics_list->OMSetStencilRef(1);

	RenderGeometry(geometry, translation, {});

	this->m_is_stencil_equal = true;
	this->m_current_clip_operation = -1;
	this->m_p_command_graphics_list->OMSetStencilRef(m_stencil_ref_value);
	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
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

static int nGlobalID{};
Rml::TextureHandle RenderInterface_DX12::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	RMLUI_ZoneScopedN("DirectX 12 - LoadTexture");

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
	Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer.get(), sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	const size_t image_size = header.width * header.height * 4; // We always make 32bit textures

	if (header.dataType != 2)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return false;
	}

	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported.");
		return false;
	}

	const byte* image_src = buffer.get() + sizeof(TGAHeader);
	Rml::UniquePtr<byte[]> image_dest_buffer(new byte[image_size]);
	byte* image_dest = image_dest_buffer.get();

	// Targa is BGR, swap to RGB, flip Y axis, and convert to premultiplied alpha.
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
				const byte alpha = image_src[read_index + 3];
				for (size_t j = 0; j < 3; j++)
					image_dest[write_index + j] = byte((image_dest[write_index + j] * alpha) / 255);
				image_dest[write_index + 3] = alpha;
			}
			else
				image_dest[write_index + 3] = 255;

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;

	return GenerateTexture({image_dest, image_size}, texture_dimensions);
}

Rml::TextureHandle RenderInterface_DX12::GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions)
{
	RMLUI_ZoneScopedN("DirectX 12 - GenerateTexture");

	RMLUI_ASSERTMSG(source_data.data(), "must be valid source");
	RMLUI_ASSERTMSG(this->m_p_allocator, "backend requires initialized allocator, but it is not initialized");

	int width = source_dimensions.x;
	int height = source_dimensions.y;

	RMLUI_ASSERTMSG(width > 0, "width is less or equal to 0");
	RMLUI_ASSERTMSG(height > 0, "height is less or equal to 0");

	D3D12_RESOURCE_DESC desc_texture = {};
	desc_texture.MipLevels = 1;
	desc_texture.DepthOrArraySize = 1;
	desc_texture.Format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
	desc_texture.Width = width;
	desc_texture.Height = height;
	desc_texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc_texture.SampleDesc.Count = 1;
	desc_texture.SampleDesc.Quality = 0;

	TextureHandleType* p_resource = new TextureHandleType();

	#ifdef RMLUI_DX_DEBUG
	if (p_resource)
	{
		p_resource->Set_ResourceName(std::to_string(nGlobalID));
		++nGlobalID;
	}
	#endif

	this->m_manager_texture.Alloc_Texture(desc_texture, p_resource, source_data.data()
	#ifdef RMLUI_DX_DEBUG
																		,
		p_resource->Get_ResourceName()
	#endif
	);

	return reinterpret_cast<Rml::TextureHandle>(p_resource);
}

void RenderInterface_DX12::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	RMLUI_ZoneScopedN("DirectX 12 - ReleaseTexture");
	TextureHandleType* p_texture = reinterpret_cast<TextureHandleType*>(texture_handle);
	//	this->m_pending_for_deletion_textures.push_back(p_texture);

	if (p_texture)
	{
	#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::LT_DEBUG, "Destroyed texture: [%s]", p_texture->Get_ResourceName().c_str());
	#endif

		this->m_manager_texture.Free_Texture(p_texture);

		delete p_texture;
	}
}

void RenderInterface_DX12::SetTransform(const Rml::Matrix4f* transform)
{
	RMLUI_ZoneScopedN("DirectX 12 - SetTransform");

	this->m_constant_buffer_data_transform = (transform ? (this->m_projection * (*transform)) : this->m_projection);
	//	this->m_program_state_transform_dirty.set();
}

RenderInterface_DX12::RenderLayerStack::RenderLayerStack() :
	m_msaa_sample_count{1}, m_width{}, m_height{}, m_layers_size{}, m_p_manager_texture{}, m_p_manager_buffer{}, m_p_device{},
	m_p_depth_stencil_for_layers{}, m_p_depth_stencil_for_postprocess{}
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::Constructor");
	this->m_fb_postprocess.resize(4);

	// in order to prevent calling dtor when doing push_back on m_fb_layers
	// we need to reserve memory, like how much we do expect elements in array (vector)
	// otherwise you will get validation assert in dtor of FramebufferData struct and
	// that validation supposed to be for memory leaks or wrong resource handling (like you forgot to delete resource somehow)
	// if you didn't get it check this: https://en.cppreference.com/w/cpp/container/vector/reserve

	// otherwise if your default implementation requires more layers by default, thus we have a field at compile-time (or at runtime as dynamic
	// extension) RMLUI_RENDER_BACKEND_OVERRIDE_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS
	this->m_fb_layers.reserve(RMLUI_RENDER_BACKEND_FIELD_RESERVECOUNT_OF_RENDERSTACK_LAYERS);
	this->m_p_depth_stencil_for_layers = new Gfx::FramebufferData();
	this->m_p_depth_stencil_for_layers->Set_RenderTarget(false);

	this->m_p_depth_stencil_for_postprocess = new Gfx::FramebufferData();
	this->m_p_depth_stencil_for_postprocess->Set_RenderTarget(false);
}

RenderInterface_DX12::RenderLayerStack::~RenderLayerStack()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::Destructor");

	this->m_p_device = nullptr;
	this->m_p_manager_buffer = nullptr;
	this->m_p_manager_texture = nullptr;

	if (this->m_p_depth_stencil_for_layers)
	{
		delete this->m_p_depth_stencil_for_layers;
		this->m_p_depth_stencil_for_layers = nullptr;
	}

	if (this->m_p_depth_stencil_for_postprocess)
	{
		delete this->m_p_depth_stencil_for_postprocess;
		this->m_p_depth_stencil_for_postprocess = nullptr;
	}
}

void RenderInterface_DX12::RenderLayerStack::Initialize(RenderInterface_DX12* p_owner)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::Initialize");

	RMLUI_ASSERT(p_owner && "you must pass a valid pointer of RenderInterface_DX12 instance");

	#ifdef RMLUI_DEBUG
	if (p_owner)
	{
		RMLUI_ASSERT(p_owner->Get_Fence() && "you didn't initialize even fence or you call this method too early!");
		RMLUI_ASSERT(
			p_owner->Get_TextureManager().Is_Initialized() && "early call! you must initialize texture memory manager before calling this method!");
		RMLUI_ASSERT(
			p_owner->Get_BufferManager().Is_Initialized() && "early call! you must initialize buffer memory manager before calling this method!");
		RMLUI_ASSERT(p_owner->Get_Device() && "you must initialize DirectX 12 before calling this method! device is nullptr");
	}
	#endif

	if (p_owner)
	{
		this->m_p_device = p_owner->Get_Device();
		this->m_p_manager_buffer = &p_owner->Get_BufferManager();
		this->m_p_manager_texture = &p_owner->Get_TextureManager();
		this->m_msaa_sample_count = p_owner->Get_MSAASampleCount();
	}
}

void RenderInterface_DX12::RenderLayerStack::Shutdown()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::Shutdown");

	this->DestroyFramebuffers();
}

Rml::LayerHandle RenderInterface_DX12::RenderLayerStack::PushLayer()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::PushLayer");

	RMLUI_ASSERT(this->m_layers_size <= static_cast<int>(this->m_fb_layers.size()) && "overflow of layers!");
	RMLUI_ASSERT(this->m_p_depth_stencil_for_layers && "must be valid!");

	if (this->m_layers_size == static_cast<int>(this->m_fb_layers.size()))
	{
		/*
		// All framebuffers should share a single stencil buffer.
		GLuint shared_depth_stencil = (fb_layers.empty() ? 0 : fb_layers.front().depth_stencil_buffer);

		fb_layers.push_back(Gfx::FramebufferData{});
		Gfx::CreateFramebuffer(fb_layers.back(), width, height, NUM_MSAA_SAMPLES, Gfx::FramebufferAttachment::DepthStencil, shared_depth_stencil);
		*/

		if (this->m_p_depth_stencil_for_layers->Get_Texture() == nullptr)
		{
			this->CreateFramebuffer(this->m_p_depth_stencil_for_layers, m_width, m_height, this->m_msaa_sample_count, true);

	#ifdef RMLUI_DX_DEBUG
			if (this->m_p_depth_stencil_for_layers->Get_Texture())
			{
				wchar_t framebuffer_name[64];
				wsprintf(framebuffer_name, L"framebuffer (layer) shared depth-stencil");
				int index_buffer = this->m_p_depth_stencil_for_layers->Get_Texture()->Get_Info().Get_BufferIndex();
				RMLUI_ASSERT(index_buffer == -1 && "you can't allocate framebuffer as placed resource no sense!");

				if (index_buffer == -1)
				{
					RMLUI_ASSERT(this->m_p_depth_stencil_for_layers->Get_Texture()->Get_Resource() && "failed to allocate framebuffer!");
					D3D12MA::Allocation* p_committed_resource =
						static_cast<D3D12MA::Allocation*>(this->m_p_depth_stencil_for_layers->Get_Texture()->Get_Resource());

					if (p_committed_resource)
					{
						RMLUI_ASSERT(p_committed_resource->GetResource() && "failed to allocate for D3D12MA! (GetResource==nullptr)");
						p_committed_resource->GetResource()->SetName(framebuffer_name);
					}
				}
			}
	#endif
		}

		this->m_fb_layers.push_back(Gfx::FramebufferData{});
		auto* p_buffer = &this->m_fb_layers.back();
		this->CreateFramebuffer(p_buffer, m_width, m_height, this->m_msaa_sample_count, false);
		p_buffer->Set_ID(this->m_fb_layers.size() - 1);

	#ifdef RMLUI_DX_DEBUG
		wchar_t framebuffer_name[32];
		wsprintf(framebuffer_name, L"framebuffer (layer): %d", this->m_layers_size);
		int index_buffer = p_buffer->Get_Texture()->Get_Info().Get_BufferIndex();
		RMLUI_ASSERT(index_buffer == -1 && "you can't allocate framebuffer as placed resource no sense!");

		if (index_buffer == -1)
		{
			RMLUI_ASSERT(p_buffer->Get_Texture()->Get_Resource() && "failed to allocate framebuffer!");
			D3D12MA::Allocation* p_committed_resource = static_cast<D3D12MA::Allocation*>(p_buffer->Get_Texture()->Get_Resource());

			if (p_committed_resource->GetResource())
			{
				RMLUI_ASSERT(p_committed_resource->GetResource() && "failed to allocate for D3D12MA! (GetResource==nullptr)");
				p_committed_resource->GetResource()->SetName(framebuffer_name);
			}
		}
	#endif

		p_buffer->Set_SharedDepthStencilTexture(this->m_p_depth_stencil_for_layers);
	}

	++this->m_layers_size;

	return GetTopLayerHandle();
}

void RenderInterface_DX12::RenderLayerStack::PopLayer()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::PopLayer");

	RMLUI_ASSERT(this->m_layers_size > 0 && "calculations are wrong, debug your code please!");
	this->m_layers_size -= 1;
}

const Gfx::FramebufferData& RenderInterface_DX12::RenderLayerStack::GetLayer(Rml::LayerHandle layer) const
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::GetLayer");

	RMLUI_ASSERT(static_cast<size_t>(layer) < static_cast<size_t>(this->m_layers_size) &&
		"overflow or not correct calculation or something is broken, debug the code!");
	return this->m_fb_layers.at(static_cast<size_t>(layer));
}

const Gfx::FramebufferData& RenderInterface_DX12::RenderLayerStack::GetTopLayer() const
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::GetTopLayer");

	RMLUI_ASSERT(this->m_layers_size > 0 && "early calling!");
	return this->m_fb_layers[this->m_layers_size - 1];
}

const Gfx::FramebufferData& RenderInterface_DX12::RenderLayerStack::Get_SharedDepthStencil_Layers()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::Get_SharedDepthStencil_Layers");

	RMLUI_ASSERT(this->m_p_depth_stencil_for_layers && "early calling!");
	return *this->m_p_depth_stencil_for_layers;
}

const Gfx::FramebufferData& RenderInterface_DX12::RenderLayerStack::Get_SharedDepthStencil_Postprocess()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::Get_SharedDepthStencil_Postprocess");

	RMLUI_ASSERT(this->m_p_depth_stencil_for_postprocess && "early calling!");
	return *this->m_p_depth_stencil_for_postprocess;
}

Rml::LayerHandle RenderInterface_DX12::RenderLayerStack::GetTopLayerHandle() const
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::GetTopLayerHandle");

	RMLUI_ASSERT(m_layers_size > 0 && "early calling or something is broken!");
	return static_cast<Rml::LayerHandle>(m_layers_size - 1);
}

void RenderInterface_DX12::RenderLayerStack::SwapPostprocessPrimarySecondary()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::SwapPostprocessPrimarySecondary");

	std::swap(this->m_fb_postprocess[0], this->m_fb_postprocess[1]);
}

void RenderInterface_DX12::RenderLayerStack::BeginFrame(int width_new, int height_new)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::BeginFrame");

	RMLUI_ASSERT(this->m_layers_size == 0 && "something is wrong and you forgot to clear/delete something!");

	if (this->m_width != width_new || this->m_height != height_new)
	{
		this->m_width = width_new;
		this->m_height = height_new;

		this->DestroyFramebuffers();
	}

	this->PushLayer();
}

void RenderInterface_DX12::RenderLayerStack::EndFrame()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::EndFrame");

	RMLUI_ASSERT(this->m_layers_size == 1 && "order is wrong or something is broken!");
	this->PopLayer();
}

void RenderInterface_DX12::RenderLayerStack::DestroyFramebuffers()
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::DestroyFramebuffers");

	RMLUI_ASSERTMSG(this->m_layers_size == 0, "Do not call this during frame rendering, that is, between BeginFrame() and EndFrame().");
	RMLUI_ASSERT(this->m_p_manager_texture && "you must initialize this manager or it is a early calling or it is a late calling, debug please!");

	// deleting shared depth stencil

	if (this->m_p_manager_texture)
	{
		if (this->m_p_depth_stencil_for_layers->Get_Texture())
		{
			this->m_p_manager_texture->Free_Texture(this->m_p_depth_stencil_for_layers);
		}

		if (this->m_p_depth_stencil_for_postprocess->Get_Texture())
		{
			this->m_p_manager_texture->Free_Texture(this->m_p_depth_stencil_for_postprocess);
		}
	}

	for (auto& fb : this->m_fb_layers)
	{
		if (fb.Get_Texture())
		{
			this->DestroyFramebuffer(&fb);
		}
	}

	this->m_fb_layers.clear();

	for (auto& fb : this->m_fb_postprocess)
	{
		if (fb.Get_Texture())
		{
			this->DestroyFramebuffer(&fb);
		}
	}
}

const Gfx::FramebufferData& RenderInterface_DX12::RenderLayerStack::EnsureFramebufferPostprocess(int index)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::EnsureFramebufferPostprocess");

	RMLUI_ASSERT(index < static_cast<int>(this->m_fb_postprocess.size()) && "overflow or wrong calculation, debug the code!");

	Gfx::FramebufferData& fb = this->m_fb_postprocess.at(index);

	if (!fb.Get_Texture())
	{
		this->CreateFramebuffer(&fb, this->m_width, this->m_height, 1, false);
		fb.Set_ID(index);

		RMLUI_ASSERT(this->m_p_depth_stencil_for_postprocess && "must be depth stencil initialized here!");

		if (this->m_p_depth_stencil_for_postprocess)
		{
			if (this->m_p_depth_stencil_for_postprocess->Get_Texture() == nullptr)
			{
				this->CreateFramebuffer(this->m_p_depth_stencil_for_postprocess, this->m_width, this->m_height, 1, true);

	#ifdef RMLUI_DX_DEBUG
				if (this->m_p_depth_stencil_for_postprocess->Get_Texture())
				{
					wchar_t framebuffer_name[64];
					wsprintf(framebuffer_name, L"framebuffer (postprocess) shared depth-stencil");

					int buffer_index = this->m_p_depth_stencil_for_postprocess->Get_Texture()->Get_Info().Get_BufferIndex();
					RMLUI_ASSERT(buffer_index == -1 && "you can't allocate framebuffer as placed resource, no sense!");

					if (buffer_index == -1)
					{
						RMLUI_ASSERT(
							this->m_p_depth_stencil_for_postprocess->Get_Texture()->Get_Resource() && "must be valid resource failed to allocate!");

						D3D12MA::Allocation* p_allocation =
							static_cast<D3D12MA::Allocation*>(this->m_p_depth_stencil_for_postprocess->Get_Texture()->Get_Resource());

						if (p_allocation)
						{
							ID3D12Resource* p_resource = p_allocation->GetResource();
							RMLUI_ASSERT(p_resource && "failed to allocate for D3D12MA! (GetResource==nullptr)");

							if (p_resource)
							{
								p_resource->SetName(framebuffer_name);
							}
						}
					}
				}
	#endif
			}
		}

		fb.Set_SharedDepthStencilTexture(this->m_p_depth_stencil_for_postprocess);

	#ifdef RMLUI_DX_DEBUG
		fb.m_is_allocated_on_stack = false;
		if (fb.Get_Texture())
		{
			wchar_t framebuffer_name[32];
			wsprintf(framebuffer_name, L"framebuffer (postprocess): %d", index);
			int buffer_index = fb.Get_Texture()->Get_Info().Get_BufferIndex();

			RMLUI_ASSERT(buffer_index == -1 && "you can't allocate framebuffer as placed resource no sense!");
			if (buffer_index == -1)
			{
				RMLUI_ASSERT(fb.Get_Texture()->Get_Resource() && "must be valid resource failed to allocate!");
				D3D12MA::Allocation* p_alloc = static_cast<D3D12MA::Allocation*>(fb.Get_Texture()->Get_Resource());

				if (p_alloc)
				{
					ID3D12Resource* p_resource = p_alloc->GetResource();
					RMLUI_ASSERT(p_resource && "failed to allocate for D3D12MA");

					if (p_resource)
					{
						p_resource->SetName(framebuffer_name);
					}
				}
			}
		}
	#endif
	}

	return fb;
}

// TODO: add argument for depth stencil thing!!!!
void RenderInterface_DX12::RenderLayerStack::CreateFramebuffer(Gfx::FramebufferData* p_result, int width, int height, int sample_count,
	bool is_depth_stencil)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::CreateFramebuffer");

	RMLUI_ASSERT(p_result && "you must pass a valid pointer!");
	RMLUI_ASSERT(sample_count > 0 && "you must pass a valid value! it must be positive");

	RMLUI_ASSERT(width > 0 && "pass a valid width!");
	RMLUI_ASSERT(height > 0 && "pass a valid height");
	RMLUI_ASSERT(this->m_p_manager_texture && "you must register manager texture befor calling this method (manager texture is nullptr)");
	if (this->m_p_manager_texture)
	{
		D3D12_RESOURCE_DESC desc_texture = {};

		DXGI_FORMAT format{};

		if (is_depth_stencil)
			format = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
		else
			format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;

		desc_texture.MipLevels = 1;
		desc_texture.DepthOrArraySize = 1;
		desc_texture.Format = format;
		desc_texture.Width = static_cast<UINT64>(width);
		desc_texture.Height = static_cast<UINT64>(height);
		desc_texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc_texture.SampleDesc.Count = sample_count;
		desc_texture.SampleDesc.Quality = 0;

		TextureHandleType* p_resource = new TextureHandleType();

		RMLUI_ASSERT(p_resource && "[OS][ERROR] not enough memory for allocation!");

	#ifdef RMLUI_DX_DEBUG
		if (p_resource)
		{
			const char* pWhatTypeOfTexture{};

			constexpr const char* kHardcodedRenderTargetName = "[RT] ";
			constexpr const char* kHardcodedDepthStencilName = "[DS] ";

			if (is_depth_stencil)
				pWhatTypeOfTexture = kHardcodedDepthStencilName;
			else
				pWhatTypeOfTexture = kHardcodedRenderTargetName;

			Rml::String msaa_info;

			if (sample_count > 1)
				msaa_info = " | MSAA[" + std::to_string(sample_count) + "]";

			p_resource->Set_ResourceName(pWhatTypeOfTexture + std::string("Render2Texture[") + std::to_string(this->m_layers_size) + "]" + msaa_info);
		}
	#endif

		// todo: move to debug build configuration because this information is not useful (?)
		p_result->Set_Width(width);
		p_result->Set_Height(height);
		p_result->Set_Texture(p_resource);
		D3D12_RESOURCE_FLAGS flags{};

		if (is_depth_stencil)
			flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		else
			flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_RESOURCE_STATES states{};

		if (is_depth_stencil)
			states = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
		else
			states = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;

	#ifdef RMLUI_DEBUG
		const char* pWhatTypeOfTextureForAllocationName{};

		constexpr const char* kHardcodedNameForDepthStencil = "Render2Texture|Depth-Stencil";
		constexpr const char* kHardcodedNameForRenderTarget = "Render2Texture|Render-Target";

		if (is_depth_stencil)
			pWhatTypeOfTextureForAllocationName = kHardcodedNameForDepthStencil;
		else
			pWhatTypeOfTextureForAllocationName = kHardcodedNameForRenderTarget;
	#endif

		this->m_p_manager_texture->Alloc_Texture(desc_texture, p_result, flags, states
	#ifdef RMLUI_DEBUG
			,
			pWhatTypeOfTextureForAllocationName
	#endif
		);
	}
}

void RenderInterface_DX12::RenderLayerStack::DestroyFramebuffer(Gfx::FramebufferData* p_data)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderLayerStack::DestroyFramebuffer");

	RMLUI_ASSERT(p_data && "you must pass a valid data");
	RMLUI_ASSERT(this->m_p_manager_texture && "early/late calling?");

	if (p_data)
	{
		this->m_p_manager_texture->Free_Texture(p_data);
		p_data->Set_Width(-1);
		p_data->Set_Height(-1);
	}
}

Rml::LayerHandle RenderInterface_DX12::PushLayer()
{
	RMLUI_ZoneScopedN("DirectX 12 - PushLayer");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "PushLayer");

	const Rml::LayerHandle layer_handle = this->m_manager_render_layer.PushLayer();

	const auto& framebuffer = this->m_manager_render_layer.GetLayer(layer_handle);

	RMLUI_ASSERT(framebuffer.Get_SharedDepthStencilTexture() && "you have to set shared depth stencil texture for layer!");
	const auto& shared_depthstencil = *framebuffer.Get_SharedDepthStencilTexture();

	this->m_p_command_graphics_list->OMSetRenderTargets(1, &framebuffer.Get_DescriptorResourceView(), FALSE,
		&shared_depthstencil.Get_DescriptorResourceView());

	constexpr FLOAT clear_color[] = {0.0f, 0.0f, 0.0f, 0.0f};

	this->m_p_command_graphics_list->ClearRenderTargetView(framebuffer.Get_DescriptorResourceView(), clear_color, 0, nullptr);
//	this->m_p_command_graphics_list->ClearDepthStencilView(shared_depthstencil.Get_DescriptorResourceView(),
//		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

	return layer_handle;
}

void RenderInterface_DX12::BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_id)
{
	RMLUI_ZoneScopedN("DirectX 12 - BlitLayerToPostprocessPrimary");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "BlitLayerToPostprocessPrimary");

	const Gfx::FramebufferData& source_framebuffer = this->m_manager_render_layer.GetLayer(layer_id);
	const Gfx::FramebufferData& destination_framebuffer = this->m_manager_render_layer.GetPostprocessPrimary();

	RMLUI_ASSERT(source_framebuffer.Get_Texture() && "texture must be presented when you call this method!");
	RMLUI_ASSERT(destination_framebuffer.Get_Texture() && "texture must be presented when you call this method!");

	RMLUI_ASSERT(source_framebuffer.Get_Texture()->Get_Info().Get_BufferIndex() == -1 &&
		"expected that this texture was allocated as committed since it is 'framebuffer' ");
	RMLUI_ASSERT(destination_framebuffer.Get_Texture()->Get_Info().Get_BufferIndex() == -1 &&
		"expected that this texture was allocated as committed since it is 'framebuffer' ");

	RMLUI_ASSERT(source_framebuffer.Get_Texture()->Get_Resource() && "texture must contain allocated and valid resource!");
	RMLUI_ASSERT(destination_framebuffer.Get_Texture()->Get_Resource() && "texture must contain allocated and valid resource!");

	ID3D12Resource* p_src = static_cast<D3D12MA::Allocation*>(source_framebuffer.Get_Texture()->Get_Resource())->GetResource();
	ID3D12Resource* p_dst = static_cast<D3D12MA::Allocation*>(destination_framebuffer.Get_Texture()->Get_Resource())->GetResource();

	RMLUI_ASSERT(p_src && "must be valid && allocated");
	RMLUI_ASSERT(p_dst && "must be valid && allocated");

	RMLUI_ASSERT(this->m_p_command_graphics_list && "must be initialized before calling this method");

	if (!this->m_p_command_graphics_list)
		return;

	D3D12_RESOURCE_BARRIER barriers[2] = {CD3DX12_RESOURCE_BARRIER::Transition(p_src, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
											  D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(p_dst, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST)};

	this->m_p_command_graphics_list->ResourceBarrier(2, barriers);

	this->m_p_command_graphics_list->ResolveSubresource(p_dst, 0, p_src, 0, RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT);

	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(p_dst, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(p_src, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	// todo: should we convert src to render target ? probably yes, but need to be sure later after debugging
	this->m_p_command_graphics_list->ResourceBarrier(2, barriers);

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

static Rml::Pair<int, float> SigmaToParameters(const float desired_sigma)
{
	RMLUI_ZoneScopedN("DirectX 12 - SigmaToParameters");

	constexpr int max_num_passes = 10;
	static_assert(max_num_passes < 31, "");
	constexpr float max_single_pass_sigma = 3.0f;

	Rml::Pair<int, float> result;

	result.first = Rml::Math::Clamp(Rml::Math::Log2(int(desired_sigma * (2.f / max_single_pass_sigma))), 0, max_num_passes);
	result.second = Rml::Math::Clamp(desired_sigma / float(1 << result.first), 0.0f, max_single_pass_sigma);

	return result;
}

void RenderInterface_DX12::DrawFullscreenQuad()
{
	RMLUI_ZoneScopedN("DirectX 12 - DrawFullscreenQuad()");
	RMLUI_ASSERT(this->m_p_command_graphics_list && "must be valid!");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "DrawFullscreenQuad");
	RenderGeometry(this->m_precompiled_fullscreen_quad_geometry, {}, TexturePostprocess);
	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling)
{
	RMLUI_ZoneScopedN("DirectX 12 - DrawfullscreenQuad(uv_offset,uv_scaling)");
	RMLUI_ASSERT(this->m_p_command_graphics_list && "must be valid!");
}

void RenderInterface_DX12::BindTexture(TextureHandleType* p_texture, UINT root_parameter_index)
{
	RMLUI_ASSERT(p_texture && "you have to pass a valid pointer!");
	RMLUI_ASSERT(this->m_p_command_graphics_list && "early calling must be initialized before calling this method!");
	RMLUI_ASSERT(this->m_p_descriptor_heap_shaders && "early calling must be initialzed before calling this method!");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "BindTexture");

	if (this->m_p_command_graphics_list)
	{
		if (p_texture)
		{
			if (this->m_p_descriptor_heap_shaders)
			{
				D3D12_GPU_DESCRIPTOR_HANDLE srv_handle;
				srv_handle.ptr =
					this->m_p_descriptor_heap_shaders->GetGPUDescriptorHandleForHeapStart().ptr + p_texture->Get_Allocation_DescriptorHeap().offset;

				this->m_p_command_graphics_list->SetGraphicsRootDescriptorTable(root_parameter_index, srv_handle);
			}
		}
	}

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::BindRenderTarget(const Gfx::FramebufferData& framebuffer, bool depth_included)
{
	RMLUI_ASSERT(this->m_p_command_graphics_list && "early calling must be initialized before calling this method!");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "BindRenderTarget");

	if (this->m_p_command_graphics_list)
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle = nullptr;

		if (depth_included)
		{
			RMLUI_ASSERT(framebuffer.Get_SharedDepthStencilTexture() && "expects that it was allocated and set to render target textures!");
			p_dsv_handle = &framebuffer.Get_SharedDepthStencilTexture()->Get_DescriptorResourceView();
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE* p_rtv_handle = &framebuffer.Get_DescriptorResourceView();

		this->m_p_command_graphics_list->OMSetRenderTargets(1, p_rtv_handle, FALSE, p_dsv_handle);
	}

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

unsigned char RenderInterface_DX12::GetMSAASupportedSampleCount(unsigned char max_samples)
{
	RMLUI_ASSERT(this->m_p_device && "early calling this must be initialized before calling!");

	if (this->m_p_device)
	{
	#ifdef RMLUI_DEBUG
		bool sample_counts[64]{};
	#endif

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS desc{};
		desc.SampleCount = static_cast<UINT>(max_samples);
		desc.Format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		desc.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

		unsigned char max_result = 1;

		while (desc.SampleCount > 1)
		{
			HRESULT result = this->m_p_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &desc, sizeof(desc));

			if (SUCCEEDED(result))
			{
	#ifdef RMLUI_DEBUG
				sample_counts[desc.SampleCount - 1] = true;
	#endif

				if (desc.SampleCount > max_result)
					max_result = desc.SampleCount;

	#ifndef RMLUI_DEBUG
				// on debug we accumulate information about all possible (for debug purposes) max sample sample_counts[0] = false always be false due
				// to sample count checking in while loop
				break;
	#endif
			}

			--desc.SampleCount;
		}

		return max_result;
	}

	return 1;
}

void RenderInterface_DX12::RenderBlur(float sigma, const Gfx::FramebufferData& source_destination, const Gfx::FramebufferData& temp,
	const Rml::Rectanglei window_flipped)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderBlur");
	RMLUI_ASSERT(&source_destination != &temp && "you can't pass the same object to source_destination and temp arguments!");
	RMLUI_ASSERT(source_destination.Get_Width() == temp.Get_Width() && "must be equal to the same size!");
	RMLUI_ASSERT(source_destination.Get_Height() == temp.Get_Height() && "must be equal to the same size!");
	RMLUI_ASSERT(window_flipped.Valid() && "must be valid!");

	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "RenderBlur");

	int pass_level = 0;
	const Rml::Pair<int, float>& pass_level_and_sigma = SigmaToParameters(sigma);

	if (pass_level_and_sigma.second == 0.0f)
		return;

	const Rml::Rectanglei original_scissor = this->m_scissor;
	Rml::Rectanglei scissor = window_flipped;

	UseProgram(ProgramId::Passthrough_NoBlend);
	SetScissor(scissor, true);

	D3D12_VIEWPORT vp{};

	vp.TopLeftX = 0;
	vp.TopLeftY = source_destination.Get_Height() / 2.0f;
	vp.Width = source_destination.Get_Width() / 2.0f;
	vp.Height = source_destination.Get_Height() / 2.0f;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	this->m_p_command_graphics_list->RSSetViewports(1, &vp);

	const Rml::Vector2f uv_scaling = {(source_destination.Get_Width() % 2 == 1) ? (1.f - 1.f / float(source_destination.Get_Width())) : 1.f,
		(source_destination.Get_Height() % 2 == 1) ? (1.f - 1.f / float(source_destination.Get_Height())) : 1.f};

	for (int i = 0; i < pass_level; i++)
	{
		scissor.p0 = (scissor.p0 + Rml::Vector2i(1)) / 2;
		scissor.p1 = Rml::Math::Max(scissor.p1 / 2, scissor.p0);
		const bool from_source = (i % 2 == 0);

		this->BindRenderTarget(from_source ? temp : source_destination);

		RenderInterface_DX12::TextureHandleType* p_texture = nullptr;

		if (from_source)
		{
			p_texture = temp.Get_Texture();
		}
		else
		{
			p_texture = source_destination.Get_Texture();
		}

		RMLUI_ASSERT(p_texture && "must be valid!");

		this->BindTexture(p_texture);

		SetScissor(scissor, true);

		DrawFullscreenQuad({}, uv_scaling);
	}

	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<FLOAT>(source_destination.Get_Width());
	vp.Height = static_cast<FLOAT>(source_destination.Get_Height());
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	this->m_p_command_graphics_list->RSSetViewports(1, &vp);
	const bool transfer_to_temp_buffer = (pass_level % 2 == 0);

	if (transfer_to_temp_buffer)
	{
		this->BindRenderTarget(temp);

		RenderInterface_DX12::TextureHandleType* p_texture = temp.Get_Texture();

		RMLUI_ASSERT(p_texture && "must be valid!");

		this->BindTexture(p_texture);

		DrawFullscreenQuad();
	}

	this->UseProgram(ProgramId::Blur);

	ConstantBufferType* p_cb = this->Get_ConstantBuffer(this->m_current_back_buffer_index);

	RMLUI_ASSERT(p_cb && "unable to allocate constant buffer for blur!");
	std::uint8_t* p_cb_begin = reinterpret_cast<std::uint8_t*>(p_cb->Get_GPU_StartMemoryForBindingData());

	RMLUI_ASSERT(p_cb_begin && "must be valid pointer of buffer where CB was allocated!");

	std::uint8_t* p_cb_real_begin = p_cb_begin +
		p_cb->Get_AllocInfo().Get_Offset();

	struct {
		Rml::Vector4f weights;
		Rml::Vector2f texel_offset;
		Rml::Vector2f texcoord_min;
		Rml::Vector2f texcoord_max;	
	} ShaderConstantBufferMapping_Blur;

	SetBlurWeights(ShaderConstantBufferMapping_Blur.weights, sigma);
	SetTexCoordLimits(ShaderConstantBufferMapping_Blur.texcoord_min, ShaderConstantBufferMapping_Blur.texcoord_max, scissor,
		{source_destination.Get_Width(), source_destination.Get_Height()});

	ShaderConstantBufferMapping_Blur.texel_offset = Rml::Vector2f(0.f, 1.f) *
		(1.0f / (temp.Get_Height()));

	std::memcpy(p_cb_real_begin, &ShaderConstantBufferMapping_Blur, sizeof(ShaderConstantBufferMapping_Blur));

	if (p_cb)
	{
		auto* p_dx_constant_buffer = this->m_manager_buffer.Get_BufferByIndex(p_cb->Get_AllocInfo().Get_BufferIndex());
		RMLUI_ASSERT(p_dx_constant_buffer && "must be valid!");

		if (p_dx_constant_buffer)
		{
			auto* p_dx_resource = p_dx_constant_buffer->GetResource();

			RMLUI_ASSERT(p_dx_resource && "must be valid!");

			if (p_dx_resource)
			{
				this->m_p_command_graphics_list->SetGraphicsRootConstantBufferView(0,
					p_dx_resource->GetGPUVirtualAddress() + p_cb->Get_AllocInfo().Get_Offset());
			}
		}
	}

	this->BindRenderTarget(temp);
	this->BindTexture(source_destination.Get_Texture());

	// Add a 1px transparent border around the blur region by first clearing with a padded scissor. This helps prevent
	// artifacts when upscaling the blur result in the later step. On Intel and AMD, we have observed that during
	// blitting with linear filtering, pixels outside the 'src' region can be blended into the output. On the other
	// hand, it looks like Nvidia clamps the pixels to the source edge, which is what we really want. Regardless, we
	// work around the issue with this extra step.
//	scissor.Extend(1);
//	SetScissor(scissor, true);

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderFilters");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "RenderFilters");

	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
	if (static_cast<Gfx::RenderState>(this->m_render_state) != Gfx::RenderState::Valid)
	{
		#ifdef RMLUI_DEBUG
		Rml::Log::Message(Rml::Log::LT_WARNING, "[RenderInterface_DX12::RenderFilters] you have invalid rendering state: %s",
			Gfx::convert_render_state_to_str(this->m_render_state));
		#endif

		RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

		return;
	}
	#endif

	for (const Rml::CompiledFilterHandle filter_handle : filter_handles)
	{
		const CompiledFilter& filter = *reinterpret_cast<const CompiledFilter*>(filter_handle);
		const FilterType type = filter.type;

		switch (type)
		{
		case FilterType::Passthrough:
		{
			const Gfx::FramebufferData& source = this->m_manager_render_layer.GetPostprocessPrimary();
			const Gfx::FramebufferData& destination = this->m_manager_render_layer.GetPostprocessSecondary();

			TextureHandleType* p_texture_source = source.Get_Texture();
			TextureHandleType* p_texture_destination = destination.Get_Texture();

			RMLUI_ASSERT(p_texture_source && "must be valid pointer!");
			RMLUI_ASSERT(p_texture_destination && "must be valid pointer!");

			RMLUI_ASSERT(p_texture_source->Get_Info().Get_BufferIndex() == -1 &&
				"expected that a such texture was allocated as committed (e.g. not placed resource), so in that case you can't cast to "
				"D3D12MA::Allocation");
			RMLUI_ASSERT(p_texture_destination->Get_Info().Get_BufferIndex() == -1 &&
				"expected that a such texture was allocated as committed (e.g. not placed resource) so in that case you can't cast to "
				"D3D12MA::Allocation");

			RMLUI_ASSERT(p_texture_source->Get_Resource() && "must contain a valid resource");
			RMLUI_ASSERT(p_texture_destination->Get_Resource() && "must contain a valid resource");

			D3D12MA::Allocation* p_allocation_source = static_cast<D3D12MA::Allocation*>(p_texture_source->Get_Resource());
			D3D12MA::Allocation* p_allocation_destination = static_cast<D3D12MA::Allocation*>(p_texture_destination->Get_Resource());

			RMLUI_ASSERT(p_allocation_source->GetResource() && "allocation must contain a valid pointer to resource! something is broken");
			RMLUI_ASSERT(p_allocation_destination->GetResource() && "allocation must contain a valid pointer to resource! something is broken");

			ID3D12Resource* p_resource_as_srv = p_allocation_source->GetResource();
			// ID3D12Resource* p_resource_as_rt = p_allocation_destination->GetResource();

			this->BindRenderTarget(destination);

			const FLOAT blend_factor[] = {filter.blend_factor, filter.blend_factor, filter.blend_factor, filter.blend_factor};
			this->m_p_command_graphics_list->OMSetBlendFactor(blend_factor);

			this->UseProgram(ProgramId::Passthrough_ColorMask);

			this->BindTexture(p_texture_source);

			// todo: useprogram and bind texture here!
			this->DrawFullscreenQuad();
			this->m_manager_render_layer.SwapPostprocessPrimarySecondary();

			break;
		}
		case FilterType::Blur:
		{
			break;
			const Gfx::FramebufferData& source = this->m_manager_render_layer.GetPostprocessPrimary();
			const Gfx::FramebufferData& temp = this->m_manager_render_layer.GetPostprocessSecondary();

			const Rml::Rectanglei window_flipped = VerticallyFlipped(this->m_scissor, this->m_height);
			RenderBlur(filter.sigma, source, temp, window_flipped);

			break;
		}
		case FilterType::DropShadow:
		{
			break;
		}
		case FilterType::ColorMatrix:
		{
			break;
		}
		case FilterType::MaskImage:
		{
			break;
		}
		case FilterType::Invalid:
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Unhandled render filter: %d", static_cast<int>(type));
			break;
		}
		default:
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Unknown render filter: %d", static_cast<int>(type));
			break;
		}
		}
	}

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
	Rml::Span<const Rml::CompiledFilterHandle> filters)
{
	RMLUI_ZoneScopedN("DirectX 12 - CompositeLayers");

	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "CompositeLayers");

	BlitLayerToPostprocessPrimary(source);

	RenderFilters(filters);

	this->BindRenderTarget(this->m_manager_render_layer.GetLayer(destination));

	if (blend_mode == Rml::BlendMode::Replace)
	{
		this->UseProgram(ProgramId::Passthrough_NoBlend);
	}
	else
	{
		// since we use msaa render target we should use appropriate version of pipeline
		this->UseProgram(ProgramId::Passthrough_MSAA);
	}

	this->BindTexture(this->m_manager_render_layer.GetPostprocessPrimary().Get_Texture());

	this->DrawFullscreenQuad();

	// should we set like return blend state as enabled?
	if (blend_mode == Rml::BlendMode::Replace)
	{
		this->UseProgram(ProgramId::Passthrough);
	}

	if (destination != this->m_manager_render_layer.GetTopLayerHandle())
	{
		this->BindRenderTarget(this->m_manager_render_layer.GetTopLayer());
	}

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::PopLayer()
{
	RMLUI_ZoneScopedN("DirectX 12 - PopLayer");
	//	RMLUI_ASSERT(false && "todo");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "PopLayer");
	this->m_manager_render_layer.PopLayer();
	this->BindRenderTarget(this->m_manager_render_layer.GetTopLayer());
	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

Rml::TextureHandle RenderInterface_DX12::SaveLayerAsTexture(Rml::Vector2i dimensions)
{
	RMLUI_ZoneScopedN("DirectX 12 - SaveLayerAsTexture");
	//	RMLUI_ASSERT(false && "todo");
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "SaveLayerAsTexture");

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

	return Rml::TextureHandle();
}

Rml::CompiledFilterHandle RenderInterface_DX12::SaveLayerAsMaskImage()
{
	RMLUI_ZoneScopedN("DirectX 12 - SaveLayerAsMaskImage");
	//	RMLUI_ASSERT(false && "todo");

	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "SaveLayerAsMaskImage");

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

	return Rml::CompiledFilterHandle();
}

Rml::CompiledFilterHandle RenderInterface_DX12::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters)
{
	RMLUI_ZoneScopedN("DirectX 12 - CompileFilter");
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
	else if (name == "drop-shadow")
	{
		filter.type = FilterType::DropShadow;
		filter.sigma = Rml::Get(parameters, "sigma", 0.f);
		filter.color = Rml::Get(parameters, "color", Rml::Colourb());
		filter.offset = Rml::Get(parameters, "offset", Rml::Vector2f(0.f));
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

void RenderInterface_DX12::ReleaseFilter(Rml::CompiledFilterHandle filter)
{
	RMLUI_ZoneScopedN("DirectX 12 - ReleaseFilter");
	delete reinterpret_cast<CompiledFilter*>(filter);
}

Rml::CompiledShaderHandle RenderInterface_DX12::CompileShader(const Rml::String& name, const Rml::Dictionary& parameters)
{
	RMLUI_ZoneScopedN("DirectX 12 - CompileShader");
	auto ApplyColorStopList = [](CompiledShader& shader, const Rml::Dictionary& shader_parameters) {
		auto it = shader_parameters.find("color_stop_list");
		RMLUI_ASSERT(it != shader_parameters.end() && it->second.GetType() == Rml::Variant::COLORSTOPLIST);
		const Rml::ColorStopList& color_stop_list = it->second.GetReference<Rml::ColorStopList>();
		const int num_stops = Rml::Math::Min((int)color_stop_list.size(), MAX_NUM_STOPS);

		shader.stop_positions.resize(num_stops);
		shader.stop_colors.resize(num_stops);
		for (int i = 0; i < num_stops; i++)
		{
			const Rml::ColorStop& stop = color_stop_list[i];
			RMLUI_ASSERT(stop.position.unit == Rml::Unit::NUMBER);
			shader.stop_positions[i] = stop.position.number;
			shader.stop_colors[i] = ConvertToColorf(stop.color);
		}
	};

	CompiledShader shader = {};

	if (name == "linear-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingLinear : ShaderGradientFunction::Linear);
		shader.p = Rml::Get(parameters, "p0", Rml::Vector2f(0.f));
		shader.v = Rml::Get(parameters, "p1", Rml::Vector2f(0.f)) - shader.p;
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "radial-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingRadial : ShaderGradientFunction::Radial);
		shader.p = Rml::Get(parameters, "center", Rml::Vector2f(0.f));
		shader.v = Rml::Vector2f(1.f) / Rml::Get(parameters, "radius", Rml::Vector2f(1.f));
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "conic-gradient")
	{
		shader.type = CompiledShaderType::Gradient;
		const bool repeating = Rml::Get(parameters, "repeating", false);
		shader.gradient_function = (repeating ? ShaderGradientFunction::RepeatingConic : ShaderGradientFunction::Conic);
		shader.p = Rml::Get(parameters, "center", Rml::Vector2f(0.f));
		const float angle = Rml::Get(parameters, "angle", 0.f);
		shader.v = {Rml::Math::Cos(angle), Rml::Math::Sin(angle)};
		ApplyColorStopList(shader, parameters);
	}
	else if (name == "shader")
	{
		const Rml::String value = Rml::Get(parameters, "value", Rml::String());
		if (value == "creation")
		{
			shader.type = CompiledShaderType::Creation;
			shader.dimensions = Rml::Get(parameters, "dimensions", Rml::Vector2f(0.f));
		}
	}

	if (shader.type != CompiledShaderType::Invalid)
		return reinterpret_cast<Rml::CompiledShaderHandle>(new CompiledShader(std::move(shader)));

	Rml::Log::Message(Rml::Log::LT_WARNING, "Unsupported shader type '%s'.", name.c_str());
	return {};
}

void RenderInterface_DX12::RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle,
	Rml::Vector2f translation, Rml::TextureHandle texture)
{
	RMLUI_ZoneScopedN("DirectX 12 - RenderShader");
	RMLUI_ASSERT(shader_handle && geometry_handle);
	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "RenderShader");

	#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
	if (static_cast<Gfx::RenderState>(this->m_render_state) != Gfx::RenderState::Valid)
	{
		#ifdef RMLUI_DEBUG
		Rml::Log::Message(Rml::Log::LT_WARNING, "[RenderInterface_DX12::RenderShader] you have invalid rendering state: %s",
			Gfx::convert_render_state_to_str(this->m_render_state));
		#endif

		RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

		return;
	}
	#endif

	const CompiledShader& shader = *reinterpret_cast<CompiledShader*>(shader_handle);
	const CompiledShaderType type = shader.type;
	const GeometryHandleType* geometry = reinterpret_cast<GeometryHandleType*>(geometry_handle);

	switch (type)
	{
	case CompiledShaderType::Gradient:
	{
		break;
	}
	case CompiledShaderType::Creation:
	{
		break;
	}
	case CompiledShaderType::Invalid:
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Unhandled render shader %d", (int)type);
		break;
	}
	}

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
}

void RenderInterface_DX12::ReleaseShader(Rml::CompiledShaderHandle effect_handle)
{
	RMLUI_ZoneScopedN("DirectX 12 - ReleaseShader");
	delete reinterpret_cast<CompiledShader*>(effect_handle);
}

void RenderInterface_DX12::Shutdown() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Shutdown");
	if (this->m_is_shutdown_called)
		return;

	if (!this->m_is_shutdown_called)
	{
		this->m_is_shutdown_called = true;
	}

	this->Flush();

	this->m_manager_render_layer.Shutdown();

	this->Destroy_Resource_Pipelines();

	if (this->m_precompiled_fullscreen_quad_geometry)
	{
		this->Free_Geometry(reinterpret_cast<GeometryHandleType*>(this->m_precompiled_fullscreen_quad_geometry));
		this->m_precompiled_fullscreen_quad_geometry = {};
	}

	this->m_manager_buffer.Shutdown();
	this->m_manager_texture.Shutdown();

	if (this->m_p_offset_allocator_for_descriptor_heap_shaders)
	{
		delete this->m_p_offset_allocator_for_descriptor_heap_shaders;
		this->m_p_offset_allocator_for_descriptor_heap_shaders = nullptr;
	}

	if (this->m_is_full_initialization)
	{
		this->Destroy_Allocator();
		this->Destroy_SyncPrimitives();
		this->Destroy_Resource_RenderTagetViews();
		this->Destroy_Swapchain();
		this->Destroy_CommandAllocators();
		if (this->m_p_device)
		{
			this->m_p_device->Release();
		}

		if (this->m_p_adapter)
		{
			this->m_p_adapter->Release();
		}

		if (this->m_p_command_graphics_list)
		{
			this->m_p_command_graphics_list->Release();
		}

		if (this->m_p_command_queue)
		{
			this->m_p_command_queue->Release();
		}

		if (this->m_p_descriptor_heap_render_target_view)
		{
			this->m_p_descriptor_heap_render_target_view->Release();
		}

		if (this->m_p_descriptor_heap_render_target_view_for_texture_manager)
		{
			this->m_p_descriptor_heap_render_target_view_for_texture_manager->Release();
		}

		if (this->m_p_descriptor_heap_depth_stencil_view_for_texture_manager)
		{
			this->m_p_descriptor_heap_depth_stencil_view_for_texture_manager->Release();
		}

		if (this->m_p_descriptor_heap_shaders)
		{
			this->m_p_descriptor_heap_shaders->Release();
		}

		if (this->m_p_descriptor_heap_depthstencil)
		{
			this->m_p_descriptor_heap_depthstencil->Release();
		}

		if (this->m_p_copy_command_list)
		{
			this->m_p_copy_command_list->Release();
		}

		if (this->m_p_copy_allocator)
		{
			this->m_p_copy_allocator->Release();
		}

		if (this->m_p_copy_queue)
		{
			this->m_p_copy_queue->Release();
		}
	}

	if (this->m_p_device)
	{
		this->m_p_device = nullptr;
	}

	if (this->m_p_adapter)
	{
		this->m_p_adapter = nullptr;
	}

	if (this->m_p_window_handle)
	{
		this->m_p_window_handle = nullptr;
	}

	if (this->m_p_swapchain)
	{
		this->m_p_swapchain = nullptr;
	}

	if (this->m_p_command_graphics_list)
	{
		this->m_p_command_graphics_list = nullptr;
	}

	if (this->m_p_command_queue)
	{
		this->m_p_command_queue = nullptr;
	}

	if (this->m_p_descriptor_heap_render_target_view)
	{
		this->m_p_descriptor_heap_render_target_view = nullptr;
	}

	if (this->m_p_allocator)
	{
		this->m_p_allocator = nullptr;
	}

	if (this->m_p_descriptor_heap_shaders)
	{
		this->m_p_descriptor_heap_shaders = nullptr;
	}

	if (this->m_p_descriptor_heap_depthstencil)
	{
		this->m_p_descriptor_heap_depthstencil = nullptr;
	}

	if (this->m_p_copy_command_list)
	{
		this->m_p_copy_command_list = nullptr;
	}

	if (this->m_p_copy_allocator)
	{
		this->m_p_copy_allocator = nullptr;
	}

	#ifdef RMLUI_DX_DEBUG
	{
		auto dll_dxgidebug = LoadLibraryEx(TEXT("dxgidebug.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

		if (dll_dxgidebug)
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "loaded dxgidebug.dll for detecting memory leaks on DirectX's side!");

			typedef HRESULT(WINAPI * LPDXGIGETDEBUGINTERFACE)(REFIID, void**);
			auto callback_DXGIGetDebugInterface =
				reinterpret_cast<LPDXGIGETDEBUGINTERFACE>(reinterpret_cast<void*>(GetProcAddress(dll_dxgidebug, "DXGIGetDebugInterface")));

			if (callback_DXGIGetDebugInterface)
			{
				IDXGIDebug* p_debug_references{};

				auto status = callback_DXGIGetDebugInterface(IID_PPV_ARGS(&p_debug_references));

				RMLUI_DX_ASSERTMSG(status, "failed to DXGIGetDebugInterface");

				if (SUCCEEDED(status))
				{
					if (p_debug_references)
					{
						p_debug_references->ReportLiveObjects(DXGI_DEBUG_ALL,
							DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
						p_debug_references->Release();
					}
				}
				else
				{
					Rml::Log::Message(Rml::Log::Type::LT_DEBUG,
						"Failed to initialize IDXGIDebug interface by DXGIGetDebugInterface function from dxgidebug.dll!");
				}
			}
		}
		else
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG,
				"your Windows version is too old for loading dxgidebug.dll! (your OS's SDK doesn't provide a such dll)");
		}
	}
	#endif

	Rml::Log::Message(Rml::Log::Type::LT_INFO, "Backend DirectX 12 is destroyed!");
}

void RenderInterface_DX12::Initialize(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize");
	if (this->m_is_shutdown_called)
	{
		this->m_is_shutdown_called = false;
	}

	if (this->m_is_full_initialization)
	{
		this->Initialize_DebugLayer();
		this->Initialize_Adapter();
		this->Initialize_Device();

		unsigned char max_msaa_supported_sample_count = this->GetMSAASupportedSampleCount(64);

		this->m_is_use_msaa = this->m_msaa_sample_count <= max_msaa_supported_sample_count;

		// requested count is 1 so we forcely set to false because in case if GPU doesn't support multisampling and returns 1 we get 1 == 1 situation
		// and m_is_use_msaa will be true but it is not right and validation layers will get assertions about this situation like we want to resolve
		// resource that as source with sample count equal to 1 but it expects to be >= 2
		if (this->m_msaa_sample_count == 1)
			this->m_is_use_msaa = false;

	#ifdef RMLUI_DEBUG
		Rml::Log::Message(Rml::Log::LT_INFO, "[DirectX-12] Max supported MSAA sample count (Quality:0): %d", max_msaa_supported_sample_count);
		Rml::Log::Message(Rml::Log::LT_INFO, "[DirectX-12] Requested MSAA level: %d (compile-time: %d | supported: %d)", this->m_msaa_sample_count,
			RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT, max_msaa_supported_sample_count);
		Rml::Log::Message(Rml::Log::LT_INFO, "[DirectX-12] MSAA: %s",
			this->m_is_use_msaa ? "supported and enabled" : "Not supported by hardware and disabled");
	#endif

		this->m_p_command_queue = this->Create_CommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		this->m_p_copy_queue = this->Create_CommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

		RMLUI_ASSERT(this->m_p_command_queue && "must create command queue!");
		RMLUI_ASSERT(this->m_p_copy_queue && "must create copy queue!");

	#ifdef RMLUI_DX_DEBUG
		this->m_p_copy_queue->SetName(TEXT("Copy Queue (for texture manager)"));
	#endif

		this->Initialize_Swapchain(0, 0);

		this->m_p_descriptor_heap_render_target_view = this->Create_Resource_DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT);

		this->m_p_descriptor_heap_render_target_view_for_texture_manager = this->Create_Resource_DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV);

		this->m_p_descriptor_heap_shaders = this->Create_Resource_DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, RMLUI_RENDER_BACKEND_FIELD_DESCRIPTORAMOUNT_FOR_SRV_CBV_UAV);

		this->m_p_descriptor_heap_depthstencil =
			this->Create_Resource_DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

		this->m_p_descriptor_heap_depth_stencil_view_for_texture_manager = this->Create_Resource_DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_DSV);

		this->m_handle_shaders = CD3DX12_CPU_DESCRIPTOR_HANDLE(this->m_p_descriptor_heap_shaders->GetCPUDescriptorHandleForHeapStart());

		this->m_size_descriptor_heap_render_target_view = this->m_p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		this->m_size_descriptor_heap_shaders = this->m_p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		this->Create_Resources_DependentOnSize();

		this->Initialize_CommandAllocators();
		this->m_p_command_graphics_list = this->Create_CommandList(this->m_backbuffers_allocators.at(this->m_current_back_buffer_index),
			D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT);
		this->Initialize_Allocator();
		this->Initialize_SyncPrimitives();

		this->m_p_copy_allocator = this->Create_CommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY);
		this->m_p_copy_command_list = this->Create_CommandList(this->m_p_copy_allocator, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY);

		this->m_p_offset_allocator_for_descriptor_heap_shaders =
			new OffsetAllocator::Allocator(RMLUI_RENDER_BACKEND_FIELD_DESCRIPTORAMOUNT_FOR_SRV_CBV_UAV * this->m_size_descriptor_heap_shaders);

	#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "amount of srv_cbv_uav: %d size of increment: %d",
			RMLUI_RENDER_BACKEND_FIELD_DESCRIPTORAMOUNT_FOR_SRV_CBV_UAV, this->m_size_descriptor_heap_shaders);
	#endif

		this->m_manager_buffer.Initialize(this->m_p_device, this->m_p_allocator, this->m_p_offset_allocator_for_descriptor_heap_shaders,
			&this->m_handle_shaders, this->m_size_descriptor_heap_shaders);
		this->m_manager_texture.Initialize(this->m_p_allocator, this->m_p_offset_allocator_for_descriptor_heap_shaders, this->m_p_device,
			this->m_p_copy_command_list, this->m_p_copy_allocator, this->m_p_descriptor_heap_shaders,
			this->m_p_descriptor_heap_render_target_view_for_texture_manager, this->m_p_descriptor_heap_depth_stencil_view_for_texture_manager,
			this->m_p_copy_queue, &this->m_handle_shaders, this);
		this->m_manager_render_layer.Initialize(this);

		this->Create_Resource_Pipelines();

		Rml::Mesh mesh;
		Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(-1.f), Rml::Vector2f(2.f), {});

		this->m_precompiled_fullscreen_quad_geometry = this->CompileGeometry(mesh.vertices, mesh.indices);

	#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "DirectX 12 Initialize type: full");
	#endif
	}
	else
	{
	#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "DirectX 12 Initialize type: user");
	#endif
	}
}

bool RenderInterface_DX12::IsSwapchainValid() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - IsSwapchainValid");
	return this->m_p_swapchain != nullptr;
}

void RenderInterface_DX12::RecreateSwapchain() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - RecreateSwapchain");
	SetViewport(m_width, m_height);
}

ID3D12Fence* RenderInterface_DX12::Get_Fence(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_Fence");
	return this->m_p_backbuffer_fence;
}

HANDLE RenderInterface_DX12::Get_FenceEvent(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_FenceEvent");
	return this->m_p_fence_event;
}

Rml::Array<uint64_t, RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT>& RenderInterface_DX12::Get_FenceValues(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_FenceValues");
	return this->m_backbuffers_fence_values;
}

uint32_t RenderInterface_DX12::Get_CurrentFrameIndex(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_CurrentFrameIndex");
	return this->m_current_back_buffer_index;
}

ID3D12Device* RenderInterface_DX12::Get_Device(void) const
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_Device");
	return this->m_p_device;
}

RenderInterface_DX12::TextureMemoryManager& RenderInterface_DX12::Get_TextureManager(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_TextureManager");
	return this->m_manager_texture;
}

RenderInterface_DX12::BufferMemoryManager& RenderInterface_DX12::Get_BufferManager(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_BufferManager");
	return this->m_manager_buffer;
}

unsigned char RenderInterface_DX12::Get_MSAASampleCount(void) const
{
	return this->m_msaa_sample_count;
}

void RenderInterface_DX12::Initialize_Device(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize_Device");
	RMLUI_ASSERT(this->m_p_adapter && "you must call this when you initialized adapter!");

	ID3D12Device2* p_device{};

	RMLUI_DX_ASSERTMSG(D3D12CreateDevice(this->m_p_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&p_device)), "failed to D3D12CreateDevice");

	this->m_p_device = p_device;

	#ifdef RMLUI_DX_DEBUG
	ID3D12InfoQueue* p_queue{};
	if (p_device)
	{
		RMLUI_DX_ASSERTMSG(p_device->QueryInterface(IID_PPV_ARGS(&p_queue)), "failed to QueryInterface of ID3D12InfoQueue");

		if (p_queue)
		{
			// if implemention is good breaking will NOT be caused at all, but if something is bad you will get __debugbreak in Debug after
			// ReportLiveObjects calling that means system works not correctly for D3D12 API at all and you must fix these problems what your device
			// reported (it might be annoying because you will not see the result of ReportLiveObjects and you should comment these line of
			// SetBreakOnSeverity in order to see full report from ReportLiveObjects calling)
			p_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, 1);
			p_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, 1);
			p_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, 1);

			// Suppress messages based on their severity level
			D3D12_MESSAGE_SEVERITY p_sevs[] = {D3D12_MESSAGE_SEVERITY_INFO};

			D3D12_INFO_QUEUE_FILTER info_filter = {};
			info_filter.DenyList.NumSeverities = _countof(p_sevs);
			info_filter.DenyList.pSeverityList = p_sevs;

			RMLUI_DX_ASSERTMSG(p_queue->PushStorageFilter(&info_filter), "failed to PushStorageFilter");

			p_queue->Release();
		}
	}
	#endif
}

void RenderInterface_DX12::Initialize_Adapter(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize_Adapter");
	if (this->m_p_adapter)
	{
		this->m_p_adapter->Release();
	}

	this->m_p_adapter = this->Get_Adapter(false);
}

void RenderInterface_DX12::Initialize_DebugLayer(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize_DebugLayer");
	#ifdef RMLUI_DX_DEBUG
	ID3D12Debug* p_debug{};

	RMLUI_DX_ASSERTMSG(D3D12GetDebugInterface(IID_PPV_ARGS(&p_debug)), "failed to D3D12GetDebugInterface");

	if (p_debug)
	{
		p_debug->EnableDebugLayer();
		p_debug->Release();
	}
	#endif
}

ID3D12CommandQueue* RenderInterface_DX12::Create_CommandQueue(D3D12_COMMAND_LIST_TYPE type) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_CommandQueue");
	RMLUI_ASSERT(this->m_p_device && "you must initialize device before calling this method");

	ID3D12CommandQueue* p_result{};
	if (this->m_p_device)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};

		desc.Type = type;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		RMLUI_DX_ASSERTMSG(this->m_p_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&p_result)), "failed to CreateCommandQueue");
	}

	return p_result;
}

void RenderInterface_DX12::Initialize_Swapchain(int width, int height) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize_Swapchain");
	RMLUI_ASSERT(width >= 0 && "must not be a negative value");
	RMLUI_ASSERT(height >= 0 && "must not be a negative value");

	// in dx12 0 means it will take size of window automatically
	if (width < 0)
		width = 0;

	if (height < 0)
		height = 0;

	IDXGISwapChain4* p_swapchain{};
	IDXGIFactory4* p_factory{};

	uint32_t create_factory_flags{};

	#ifdef RMLUI_DX_DEBUG
	create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
	#endif

	RMLUI_DX_ASSERTMSG(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&p_factory)), "failed to CreateDXGIFactory2");

	this->m_desc_sample.Count = this->m_msaa_sample_count;
	this->m_desc_sample.Quality = 0;

	DXGI_SWAP_CHAIN_DESC1 desc = {};

	desc.Width = width;
	desc.Height = height;
	desc.Format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
	desc.Stereo = 0;
	// since we can't use rt textures with different sample count than Swapchain's so we create framebuffers and presenting them on NO-MSAA
	// Swapchain
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	uint32_t flags_swapchain{};

	if (this->CheckTearingSupport())
	{
		flags_swapchain = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		this->m_is_use_tearing = true;
	}

	desc.Flags = flags_swapchain;

	IDXGISwapChain1* p_swapchain1{};

	RMLUI_DX_ASSERTMSG(p_factory->CreateSwapChainForHwnd(this->m_p_command_queue, this->m_p_window_handle, &desc, nullptr, nullptr, &p_swapchain1),
		"failed to CreateSwapChainForHwnd");

	RMLUI_DX_ASSERTMSG(p_factory->MakeWindowAssociation(this->m_p_window_handle, DXGI_MWA_NO_ALT_ENTER), "failed to MakeWindowAssociation");

	RMLUI_DX_ASSERTMSG(p_swapchain1->QueryInterface(IID_PPV_ARGS(&p_swapchain)), "failed to QueryInterface of IDXGISwapChain4");

	this->m_p_swapchain = p_swapchain;

	if (p_swapchain1)
	{
		p_swapchain1->Release();
	}

	if (p_factory)
	{
		p_factory->Release();
	}
}

void RenderInterface_DX12::Initialize_SyncPrimitives(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize_SyncPrimitives");
	RMLUI_ASSERT(this->m_p_device && "you must initialize device before calling this method!");

	if (this->m_p_device)
	{
		if (this->m_is_full_initialization)
		{
			for (int i = 0; i < RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT; ++i)
			{
				this->m_backbuffers_fence_values[i] = 0;
			}

			this->m_p_device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_p_backbuffer_fence));

			this->m_p_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			RMLUI_DX_ASSERTMSG(this->m_p_fence_event, "failed to CreateEvent (WinAPI)");
		}
	}
}

void RenderInterface_DX12::Initialize_CommandAllocators(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize_CommandAllocators");
	for (int i = 0; i < RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT; ++i)
	{
		this->m_backbuffers_allocators[i] = this->Create_CommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
	}
}

void RenderInterface_DX12::Initialize_Allocator(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Initialize_Allocator");
	// user provides allocator from his implementation or we signal that this backend initializes own allocator
	if (this->m_is_full_initialization)
	{
		RMLUI_ASSERT(!this->m_p_allocator && "forgot to destroy before initialization!");
		RMLUI_ASSERT(this->m_p_device && "must be valid when you call this method!");
		RMLUI_ASSERT(this->m_p_adapter && "must be valid when you call this method!");

		D3D12MA::ALLOCATOR_DESC desc = {};
		desc.pDevice = this->m_p_device;
		desc.pAdapter = this->m_p_adapter;

		D3D12MA::Allocator* p_allocator{};

		RMLUI_DX_ASSERTMSG(D3D12MA::CreateAllocator(&desc, &p_allocator), "failed to D3D12MA::CreateAllocator!");
		RMLUI_ASSERT(p_allocator && "failed to create allocator!");

		this->m_p_allocator = p_allocator;
	}
}

void RenderInterface_DX12::Destroy_Swapchain() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_Swapchain");
	if (this->m_p_swapchain)
	{
		if (this->m_is_full_initialization)
		{
			this->m_p_swapchain->Release();
		}
	}

	this->m_p_swapchain = nullptr;
}

void RenderInterface_DX12::Destroy_SyncPrimitives(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_SyncPrimitives");
	if (this->m_is_full_initialization)
	{
		if (this->m_p_backbuffer_fence)
		{
			this->m_p_backbuffer_fence->Release();
		}
	}

	this->m_p_backbuffer_fence = nullptr;
}

void RenderInterface_DX12::Destroy_CommandAllocators(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_CommandAllocators");
	for (ID3D12CommandAllocator* p_allocator : this->m_backbuffers_allocators)
	{
		RMLUI_ASSERT(p_allocator, "early calling or object is damaged!");
		if (p_allocator)
		{
			p_allocator->Release();
		}
	}
}

void RenderInterface_DX12::Destroy_CommandList(void) noexcept {}

void RenderInterface_DX12::Destroy_Allocator(void) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_Allocator");
	if (this->m_is_full_initialization)
	{
		if (this->m_p_allocator)
		{
			this->m_p_allocator->Release();
		}
	}
}

void RenderInterface_DX12::Flush() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Flush");
	auto value = this->Signal();
	this->WaitForFenceValue(value);
}

uint64_t RenderInterface_DX12::Signal() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Signal");
	RMLUI_ASSERT(this->m_p_command_queue && "you must initialize it first before calling this method!");
	RMLUI_ASSERT(this->m_p_backbuffer_fence && "you must initialize it first before calling this method!");

	if (this->m_p_command_queue)
	{
		if (this->m_p_backbuffer_fence)
		{
			auto value = (this->m_backbuffers_fence_values.at(this->m_current_back_buffer_index)++);

			RMLUI_DX_ASSERTMSG(this->m_p_command_queue->Signal(this->m_p_backbuffer_fence, value), "failed to command queue::Signal!");

			return value;
		}
	}

	return 0;
}

void RenderInterface_DX12::WaitForFenceValue(uint64_t fence_value, std::chrono::milliseconds time)
{
	RMLUI_ZoneScopedN("DirectX 12 - WaitForFenceValue");
	RMLUI_ASSERT(this->m_p_backbuffer_fence && "you must initialize ID3D12Fence first!");
	RMLUI_ASSERT(this->m_p_fence_event && "you must initialize fence event (HANDLE)");

	if (this->m_p_backbuffer_fence)
	{
		if (this->m_p_fence_event)
		{
			if (this->m_p_backbuffer_fence->GetCompletedValue() < fence_value)
			{
				RMLUI_DX_ASSERTMSG(this->m_p_backbuffer_fence->SetEventOnCompletion(fence_value, this->m_p_fence_event),
					"failed to SetEventOnCompletion");
				WaitForSingleObject(this->m_p_fence_event, static_cast<DWORD>(time.count()));
			}
		}
	}
}

void RenderInterface_DX12::Create_Resources_DependentOnSize() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resources_DependentOnSize");
	this->Create_Resource_RenderTargetViews();
	//	this->Create_Resource_Pipelines();
}

void RenderInterface_DX12::Destroy_Resources_DependentOnSize() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_Resources_DependentOnSize");
	//	this->Destroy_Resource_Pipelines();
	this->Destroy_Resource_RenderTagetViews();
}

void RenderInterface_DX12::Create_Resource_DepthStencil()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_DepthStencil");
	RMLUI_ASSERT(this->m_p_descriptor_heap_depthstencil && "you must initialize this descriptor heap before calling this method!");
	RMLUI_ASSERT(this->m_p_device && "you must create device!");
	RMLUI_ASSERT(this->m_width > 0 && "invalid width");
	RMLUI_ASSERT(this->m_height > 0 && "invalid height");

	D3D12MA::ALLOCATION_DESC desc_alloc = {};
	desc_alloc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc_texture = {};
	desc_texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc_texture.Format = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
	desc_texture.MipLevels = 1;
	desc_texture.Width = this->m_width;
	desc_texture.Height = this->m_height;
	desc_texture.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc_texture.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc_texture.SampleDesc.Count = 1;
	desc_texture.SampleDesc.Quality = 0;
	desc_texture.DepthOrArraySize = 1;
	desc_texture.Alignment = 0;

	D3D12_CLEAR_VALUE depth_optimized_clear_value = {};
	depth_optimized_clear_value.Format = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
	depth_optimized_clear_value.DepthStencil.Depth = RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE;
	depth_optimized_clear_value.DepthStencil.Stencil = RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE;

	ID3D12Resource* p_temp{};
	auto status = this->m_p_allocator->CreateResource(&desc_alloc, &desc_texture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depth_optimized_clear_value, &this->m_p_depthstencil_resource, IID_PPV_ARGS(&p_temp));

	RMLUI_DX_ASSERTMSG(status, "failed to create resource as depth stencil texture!");

	RMLUI_ASSERT(this->m_p_depthstencil_resource && "must be created!");
	RMLUI_ASSERT(p_temp && "must be created!");

	#ifdef RMLUI_DX_DEBUG
	this->m_p_depthstencil_resource->SetName(L"DepthStencil texture (resource)");
	#endif

	D3D12_DEPTH_STENCIL_VIEW_DESC desc_view = {};
	desc_view.Format = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
	desc_view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc_view.Flags = D3D12_DSV_FLAG_NONE;

	this->m_p_device->CreateDepthStencilView(this->m_p_depthstencil_resource->GetResource(), &desc_view,
		this->m_p_descriptor_heap_depthstencil->GetCPUDescriptorHandleForHeapStart());
}

void RenderInterface_DX12::Destroy_Resource_DepthStencil()
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_Resource_DepthStencil");
	RMLUI_ASSERT(this->m_p_depthstencil_resource && "you must create resource for calling this method!");
	RMLUI_ASSERT(this->m_p_depthstencil_resource->GetResource() && "must be valid!");

	if (this->m_p_depthstencil_resource)
	{
		if (this->m_p_depthstencil_resource->GetResource())
		{
			this->m_p_depthstencil_resource->GetResource()->Release();
		}

		auto count = this->m_p_depthstencil_resource->Release();
		RMLUI_ASSERT(count == 0 && "leak!");

		this->m_p_depthstencil_resource = nullptr;
	}
}

ID3D12DescriptorHeap* RenderInterface_DX12::Create_Resource_DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags,
	uint32_t descriptor_count) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_DescriptorHeap");
	RMLUI_ASSERT(this->m_p_device && "early calling you have to initialize device first");

	ID3D12DescriptorHeap* p_result{};

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = descriptor_count;
	desc.Type = type;
	desc.Flags = flags;

	RMLUI_DX_ASSERTMSG(this->m_p_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&p_result)), "failed to CreateDescriptorHeap");

	return p_result;
}

ID3D12CommandAllocator* RenderInterface_DX12::Create_CommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_CommandAllocator");
	ID3D12CommandAllocator* p_result{};

	RMLUI_ASSERT(this->m_p_device && "you must initialize device first!");

	if (this->m_p_device)
	{
		RMLUI_DX_ASSERTMSG(this->m_p_device->CreateCommandAllocator(type, IID_PPV_ARGS(&p_result)), "failed to CreateCommandAllocator");

		RMLUI_ASSERT(p_result && "can't allocate command allocator!");
	}

	return p_result;
}

ID3D12GraphicsCommandList* RenderInterface_DX12::Create_CommandList(ID3D12CommandAllocator* p_allocator, D3D12_COMMAND_LIST_TYPE type) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_CommandList");
	ID3D12GraphicsCommandList* p_result{};

	RMLUI_ASSERT(this->m_p_device && "you must initialize device first!");
	RMLUI_ASSERT(p_allocator && "you must pass a valid instance of ID3D12CommandAllocator*");

	if (this->m_p_device)
	{
		RMLUI_DX_ASSERTMSG(this->m_p_device->CreateCommandList(0, type, p_allocator, nullptr, IID_PPV_ARGS(&p_result)),
			"failed to CreateCommandList");
		RMLUI_ASSERT(p_result && "can't allocator command list!");

		if (p_result)
		{
			RMLUI_DX_ASSERTMSG(p_result->Close(), "failed to Close command list!");
		}
	}

	return p_result;
}

void RenderInterface_DX12::Create_Resource_RenderTargetViews()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_RenderTargetViews");
	RMLUI_ASSERT(this->m_p_device && "early calling you have to initialize device first!");
	RMLUI_ASSERT(this->m_p_swapchain && "early calling you have to initialize swapchain first!");
	RMLUI_ASSERT(
		this->m_p_descriptor_heap_render_target_view && "early calling you have to initialize descriptor heap for render target views first!");

	if (this->m_p_device)
	{
		if (this->m_p_swapchain)
		{
			if (this->m_p_descriptor_heap_render_target_view)
			{
				auto rtv_size = this->m_p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(this->m_p_descriptor_heap_render_target_view->GetCPUDescriptorHandleForHeapStart());

				for (auto i = 0; i < RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT; ++i)
				{
					ID3D12Resource* p_back_buffer{};

					RMLUI_DX_ASSERTMSG(this->m_p_swapchain->GetBuffer(i, IID_PPV_ARGS(&p_back_buffer)), "failed to GetBuffer from swapchain");

					this->m_p_device->CreateRenderTargetView(p_back_buffer, nullptr, rtv_handle);

					this->m_backbuffers_resources[i] = p_back_buffer;

					rtv_handle.Offset(rtv_size);
				}
			}
		}
	}
}

void RenderInterface_DX12::Destroy_Resource_RenderTagetViews()
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_Resource_RenderTargetViews");
	for (ID3D12Resource* p_backbuffer : this->m_backbuffers_resources)
	{
		RMLUI_ASSERT(p_backbuffer && "it is strange must be always valid pointer!");

		if (p_backbuffer)
		{
			p_backbuffer->Release();
		}
	}

	for (int i = 0; i < RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT; ++i)
	{
		this->m_backbuffers_resources[i] = nullptr;
	}
}

void RenderInterface_DX12::Create_Resource_For_Shaders(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_For_Shaders");
	this->Create_Resource_For_Shaders_ConstantBufferHeap();
}

void RenderInterface_DX12::Create_Resource_For_Shaders_ConstantBufferHeap(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_For_Shaders_ConstanceBufferHeap");
	RMLUI_ASSERT(this->m_p_allocator && "must be valid when you call this method!");
	RMLUI_ASSERT(RMLUI_RENDER_BACKEND_FIELD_SWAPCHAIN_BACKBUFFER_COUNT >= 1 && "must be non zero!");

	// todo: delete
	//	this->m_constantbuffer.Set_AllocInfo(this->m_manager_buffer.Alloc_ConstantBuffer(&this->m_constantbuffer, 72));
}

void RenderInterface_DX12::Destroy_Resource_For_Shaders_ConstantBufferHeap(void)
{
	// todo: delete
	//	this->m_manager_buffer.Free_ConstantBuffer(&this->m_constantbuffer);
}

void RenderInterface_DX12::Destroy_Resource_For_Shaders(void)
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_Resource_For_Shaders");
	for (auto& vec_cb_per_frame : this->m_constantbuffers)
	{
		for (auto& cb : vec_cb_per_frame)
		{
			this->m_manager_buffer.Free_ConstantBuffer(&cb);
		}
	}

	this->m_constantbuffers[0].clear();

	this->Update_PendingForDeletion_Geometry();
	//	this->Update_PendingForDeletion_Texture();

	this->Destroy_Resource_For_Shaders_ConstantBufferHeap();
}

void RenderInterface_DX12::Free_Geometry(RenderInterface_DX12::GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("DirectX 12 - Free_Geometry");
	RMLUI_ASSERT(p_handle && "invalid handle");

	if (p_handle)
	{
		this->m_manager_buffer.Free_Geometry(p_handle);
		delete p_handle;
	}
}

void RenderInterface_DX12::Free_Texture(RenderInterface_DX12::TextureHandleType* p_handle)
{
	RMLUI_ZoneScopedN("DirectX 12 - Free_Texture");
	RMLUI_ASSERT(p_handle && "must be valid!");

	if (p_handle)
	{
	#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] Destroyed texture: [%s]", p_handle->Get_ResourceName().c_str());
	#endif

		this->m_manager_texture.Free_Texture(p_handle);
		delete p_handle;
	}
}

void RenderInterface_DX12::Update_PendingForDeletion_Geometry()
{
	RMLUI_ZoneScopedN("DirectX 12 - Update_PendingForDeletion_Geometry");
	for (auto& p_handle : this->m_pending_for_deletion_geometry)
	{
		this->Free_Geometry(p_handle);
	}

	this->m_pending_for_deletion_geometry.clear();
}

void RenderInterface_DX12::Update_PendingForDeletion_Texture()
{
	RMLUI_ZoneScopedN("DirectX 12 - Update_PendingForDeletion_Texture");
	for (auto& p_handle : this->m_pending_for_deletion_textures)
	{
		this->Free_Texture(p_handle);
	}

	this->m_pending_for_deletion_textures.clear();
}

void RenderInterface_DX12::Create_Resource_Pipelines()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipelines");
	for (auto& vec_cb : this->m_constantbuffers)
	{
		vec_cb.resize(RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS);
	}

	for (auto& vec_cb : this->m_constantbuffers)
	{
		for (auto& cb : vec_cb)
		{
			const auto& info = this->m_manager_buffer.Alloc_ConstantBuffer(&cb, kAllocationSizeMax_ConstantBuffer);
			cb.Set_AllocInfo(info);
		}
	}

	this->m_pending_for_deletion_geometry.reserve(RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS);
	this->m_pending_for_deletion_textures.reserve(RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS);

	this->Create_Resource_For_Shaders();
	this->Create_Resource_Pipeline_BlendMask();
//	this->Create_Resource_Pipeline_Blur();
	this->Create_Resource_Pipeline_Color();
	this->Create_Resource_Pipeline_ColorMatrix();
	this->Create_Resource_Pipeline_Count();
	this->Create_Resource_Pipeline_Creation();
	this->Create_Resource_Pipeline_DropShadow();
	this->Create_Resource_Pipeline_Gradient();
	this->Create_Resource_Pipeline_Passthrough();
	this->Create_Resource_Pipeline_Passthrough_ColorMask();
	this->Create_Resource_Pipeline_Passthrough_NoBlend();
	this->Create_Resource_Pipeline_Texture();
}

void RenderInterface_DX12::Create_Resource_Pipeline_Color()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Color");
	RMLUI_ASSERT(this->m_p_device && "must be valid when we call this method!");
	RMLUI_ASSERT(Rml::GetFileInterface() && "must be valid when we call this method!");

	auto* p_filesystem = Rml::GetFileInterface();

	if (this->m_p_device && p_filesystem)
	{
		/*
		D3D12_DESCRIPTOR_RANGE ranges[1];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_DESCRIPTOR_TABLE table{};
		table.NumDescriptorRanges = 1;
		table.pDescriptorRanges = ranges;
		*/

		D3D12_ROOT_DESCRIPTOR descriptor_cbv;
		descriptor_cbv.RegisterSpace = 0;
		descriptor_cbv.ShaderRegister = 0;

		D3D12_ROOT_PARAMETER parameters[1];
		//	parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		//	parameters[0].DescriptorTable = table;
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameters[0].Descriptor = descriptor_cbv;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		CD3DX12_ROOT_SIGNATURE_DESC desc_rootsignature;
		desc_rootsignature.Init(_countof(parameters), parameters, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ID3DBlob* p_signature{};
		ID3DBlob* p_error{};
		auto status = D3D12SerializeRootSignature(&desc_rootsignature, D3D_ROOT_SIGNATURE_VERSION_1, &p_signature, &p_error);
		RMLUI_DX_ASSERTMSG(status, "failed to D3D12SerializeRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Always)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		// stencil version
		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Intersect)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Set)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_SetInverse)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Equal)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Disabled)]));
		RMLUI_DX_ASSERTMSG(status, "failed to Color_Stencil_Disabled");

		if (p_signature)
		{
			p_signature->Release();
			p_signature = nullptr;
		}

		if (p_error)
		{
			p_error->Release();
			p_error = nullptr;
		}

		ID3DBlob* p_shader_vertex{};
		ID3DBlob* p_shader_pixel{};
		ID3DBlob* p_error_buff{};

		status = D3DCompile(pShaderSourceText_Vertex, sizeof(pShaderSourceText_Vertex), nullptr, nullptr, nullptr, "main", "vs_5_0",
			this->m_default_shader_flags, 0, &p_shader_vertex, &p_error_buff);
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
		}
	#endif

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		status = D3DCompile(pShaderSourceText_Color, sizeof(pShaderSourceText_Color), nullptr, nullptr, nullptr, "main", "ps_5_0",
			this->m_default_shader_flags, 0, &p_shader_pixel, &p_error_buff);
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to compile shader: %s", (char*)(p_error_buff->GetBufferPointer()));
		}
	#endif

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		D3D12_SHADER_BYTECODE desc_bytecode_pixel_shader = {};
		desc_bytecode_pixel_shader.BytecodeLength = p_shader_pixel->GetBufferSize();
		desc_bytecode_pixel_shader.pShaderBytecode = p_shader_pixel->GetBufferPointer();

		D3D12_SHADER_BYTECODE desc_bytecode_vertex_shader = {};
		desc_bytecode_vertex_shader.BytecodeLength = p_shader_vertex->GetBufferSize();
		desc_bytecode_vertex_shader.pShaderBytecode = p_shader_vertex->GetBufferPointer();

		D3D12_INPUT_ELEMENT_DESC desc_input_layout_elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

		D3D12_INPUT_LAYOUT_DESC desc_input_layout = {};

		desc_input_layout.NumElements = sizeof(desc_input_layout_elements) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		desc_input_layout.pInputElementDescs = desc_input_layout_elements;

		D3D12_RASTERIZER_DESC desc_rasterizer = {};

		desc_rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
		desc_rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		desc_rasterizer.FrontCounterClockwise = FALSE;
		desc_rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		desc_rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc_rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc_rasterizer.DepthClipEnable = FALSE;
		desc_rasterizer.MultisampleEnable = FALSE;
		desc_rasterizer.AntialiasedLineEnable = FALSE;
		desc_rasterizer.ForcedSampleCount = 0;
		desc_rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_BLEND_DESC desc_blend_state = {};

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
			TRUE,
			FALSE,
			D3D12_BLEND_ONE,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = defaultRenderTargetBlendDesc;

		D3D12_DEPTH_STENCIL_DESC desc_depth_stencil = {};

		desc_depth_stencil.DepthEnable = FALSE;
		desc_depth_stencil.StencilEnable = TRUE;
		desc_depth_stencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc_depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc_depth_stencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		desc_depth_stencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_pipeline = {};

		desc_pipeline.InputLayout = desc_input_layout;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Always)];
		desc_pipeline.VS = desc_bytecode_vertex_shader;
		desc_pipeline.PS = desc_bytecode_pixel_shader;
		desc_pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc_pipeline.RTVFormats[0] = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		desc_pipeline.DSVFormat = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
		desc_pipeline.SampleDesc = this->m_desc_sample;
		desc_pipeline.SampleMask = 0xffffffff;
		desc_pipeline.RasterizerState = desc_rasterizer;
		desc_pipeline.BlendState = desc_blend_state;
		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.NumRenderTargets = 1;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Always)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (color)");

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
		desc_depth_stencil.BackFace = desc_depth_stencil.FrontFace;

		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Intersect)];

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC editedRenderTargetBlend = {
			TRUE,
			FALSE,
			D3D12_BLEND_ONE,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			0,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = editedRenderTargetBlend;

		desc_pipeline.BlendState = desc_blend_state;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Intersect)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (color_stencil_intersect)");

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		desc_depth_stencil.BackFace = desc_depth_stencil.FrontFace;

		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_SetInverse)];

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_SetInverse)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (color_stencil_setinverse)");

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		desc_depth_stencil.BackFace = desc_depth_stencil.FrontFace;

		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Set)];

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Set)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (color_stencil_set)");

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc_depth_stencil.BackFace = desc_depth_stencil.FrontFace;

		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Equal)];

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC editedRenderTargetBlend2 = {
			TRUE,
			FALSE,
			D3D12_BLEND_ONE,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE,
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = editedRenderTargetBlend2;

		desc_pipeline.BlendState = desc_blend_state;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Equal)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Color_Stencil_Equal)");

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc_depth_stencil.BackFace = desc_depth_stencil.FrontFace;
		desc_depth_stencil.StencilEnable = FALSE;

		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Disabled)];

		desc_pipeline.BlendState = desc_blend_state;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Disabled)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Color_Stencil_Disabled)");

	#ifdef RMLUI_DX_DEBUG
		this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Always)]->SetName(TEXT("rs of Color_Stencil_Always"));
		this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Always)]->SetName(TEXT("pipeline Color_Stencil_Always"));

		this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Set)]->SetName(TEXT("rs of Color_Stencil_Set"));
		this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Set)]->SetName(TEXT("pipeline Color_Stencil_Set"));

		this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_SetInverse)]->SetName(TEXT("rs of Color_Stencil_SetInverse"));
		this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_SetInverse)]->SetName(TEXT("pipeline Color_Stencil_SetInverse"));

		this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Intersect)]->SetName(TEXT("rs of Color_Stencil_Intersect"));
		this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Intersect)]->SetName(TEXT("pipeline Color_Stencil_Intersect"));

		this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Equal)]->SetName(TEXT("rs of Color_Stencil_Equal"));
		this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Equal)]->SetName(TEXT("pipeline Color_Stencil_Equal"));

		this->m_root_signatures[static_cast<int>(ProgramId::Color_Stencil_Disabled)]->SetName(TEXT("rs of Color_Stencil_Disabled"));
		this->m_pipelines[static_cast<int>(ProgramId::Color_Stencil_Disabled)]->SetName(TEXT("pipeline Color_Stencil_Disabled"));
	#endif
	}
}

void RenderInterface_DX12::Create_Resource_Pipeline_Texture()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Texture");
	RMLUI_ASSERT(this->m_p_device && "must be valid when we call this method!");
	RMLUI_ASSERT(Rml::GetFileInterface() && "must be valid when we call this method!");

	auto* p_filesystem = Rml::GetFileInterface();

	if (this->m_p_device && p_filesystem)
	{
		D3D12_DESCRIPTOR_RANGE ranges[1];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		/*
		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[1].NumDescriptors = 1;
		ranges[1].BaseShaderRegister = 0;
		ranges[1].RegisterSpace = 0;
		ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		*/

		D3D12_ROOT_DESCRIPTOR_TABLE table{};
		table.NumDescriptorRanges = sizeof(ranges) / sizeof(decltype(ranges[0]));
		table.pDescriptorRanges = ranges;

		D3D12_ROOT_DESCRIPTOR descriptor_cbv{};
		descriptor_cbv.RegisterSpace = 0;
		descriptor_cbv.ShaderRegister = 0;

		D3D12_ROOT_PARAMETER parameters[2];
		parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[1].DescriptorTable = table;
		parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameters[0].Descriptor = descriptor_cbv;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC desc_rootsignature;
		desc_rootsignature.Init(_countof(parameters), parameters, 1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ID3DBlob* p_signature{};
		ID3DBlob* p_error{};
		auto status = D3D12SerializeRootSignature(&desc_rootsignature, D3D_ROOT_SIGNATURE_VERSION_1, &p_signature, &p_error);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to D3D12SerializeRootSignature: %s",
				(char*)p_error->GetBufferPointer());
		}
	#endif

		RMLUI_DX_ASSERTMSG(status, "failed to D3D12SerializeRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Always)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Equal)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Disabled)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		if (p_signature)
		{
			p_signature->Release();
			p_signature = nullptr;
		}

		if (p_error)
		{
			p_error->Release();
			p_error = nullptr;
		}

		ID3DBlob* p_shader_vertex{};
		ID3DBlob* p_shader_pixel{};
		ID3DBlob* p_error_buff{};

		const D3D_SHADER_MACRO macros[] = {NULL, NULL, NULL, NULL};

		status = D3DCompile(pShaderSourceText_Vertex, sizeof(pShaderSourceText_Vertex), nullptr, macros, nullptr, "main", "vs_5_0",
			this->m_default_shader_flags, 0, &p_shader_vertex, &p_error_buff);
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
		}
	#endif

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		status = D3DCompile(pShaderSourceText_Texture, sizeof(pShaderSourceText_Texture), nullptr, nullptr, nullptr, "main", "ps_5_0",
			this->m_default_shader_flags, 0, &p_shader_pixel, &p_error_buff);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s",
				(char*)(p_error_buff->GetBufferPointer()));
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		D3D12_SHADER_BYTECODE desc_bytecode_pixel_shader = {};
		desc_bytecode_pixel_shader.BytecodeLength = p_shader_pixel->GetBufferSize();
		desc_bytecode_pixel_shader.pShaderBytecode = p_shader_pixel->GetBufferPointer();

		D3D12_SHADER_BYTECODE desc_bytecode_vertex_shader = {};
		desc_bytecode_vertex_shader.BytecodeLength = p_shader_vertex->GetBufferSize();
		desc_bytecode_vertex_shader.pShaderBytecode = p_shader_vertex->GetBufferPointer();

		D3D12_INPUT_ELEMENT_DESC desc_input_layout_elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

		D3D12_INPUT_LAYOUT_DESC desc_input_layout = {};

		desc_input_layout.NumElements = sizeof(desc_input_layout_elements) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		desc_input_layout.pInputElementDescs = desc_input_layout_elements;

		D3D12_RASTERIZER_DESC desc_rasterizer = {};

		desc_rasterizer.AntialiasedLineEnable = FALSE;
		desc_rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc_rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		desc_rasterizer.DepthBias = 0;
		desc_rasterizer.DepthBiasClamp = 0;
		desc_rasterizer.DepthClipEnable = FALSE;
		desc_rasterizer.ForcedSampleCount = 0;
		desc_rasterizer.FrontCounterClockwise = FALSE;
		desc_rasterizer.MultisampleEnable = FALSE;
		desc_rasterizer.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		desc_rasterizer.SlopeScaledDepthBias = 0;

		D3D12_BLEND_DESC desc_blend_state = {};

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
			TRUE,
			FALSE,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = defaultRenderTargetBlendDesc;

		D3D12_DEPTH_STENCIL_DESC desc_depth_stencil = {};

		desc_depth_stencil.DepthEnable = FALSE;
		desc_depth_stencil.StencilEnable = TRUE;
		desc_depth_stencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc_depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc_depth_stencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		desc_depth_stencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_pipeline = {};

		desc_pipeline.InputLayout = desc_input_layout;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Always)];
		desc_pipeline.VS = desc_bytecode_vertex_shader;
		desc_pipeline.PS = desc_bytecode_pixel_shader;
		desc_pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc_pipeline.RTVFormats[0] = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		desc_pipeline.DSVFormat = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
		desc_pipeline.SampleDesc = this->m_desc_sample;
		desc_pipeline.SampleMask = 0xffffffff;
		desc_pipeline.RasterizerState = desc_rasterizer;
		desc_pipeline.BlendState = desc_blend_state;
		desc_pipeline.NumRenderTargets = 1;
		desc_pipeline.DepthStencilState = desc_depth_stencil;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Always)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Texture_Stencil_Always)");

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc_depth_stencil.BackFace = desc_depth_stencil.FrontFace;

		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Equal)];

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Equal)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Texture_Stencil_Equal)");

		desc_depth_stencil.StencilEnable = FALSE;
		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc_depth_stencil.BackFace = desc_depth_stencil.FrontFace;

		desc_pipeline.DepthStencilState = desc_depth_stencil;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Disabled)];

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Disabled)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Texture_Stencil_Disabled)");

	#ifdef RMLUI_DX_DEBUG
		this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Always)]->SetName(TEXT("rs of Texture_Stencil_Always"));
		this->m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Always)]->SetName(TEXT("pipeline Texture_Stencil_Always"));

		this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Equal)]->SetName(TEXT("rs of Texture_Stencil_Equal"));
		this->m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Equal)]->SetName(TEXT("pipeline Texture_Stencil_Equal"));

		this->m_root_signatures[static_cast<int>(ProgramId::Texture_Stencil_Disabled)]->SetName(TEXT("rs of Texture_Stencil_Disabled"));
		this->m_pipelines[static_cast<int>(ProgramId::Texture_Stencil_Disabled)]->SetName(TEXT("pipeline Texture_Stencil_Disabled"));
	#endif
	}
}

void RenderInterface_DX12::Create_Resource_Pipeline_Gradient()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Gradient");
}

void RenderInterface_DX12::Create_Resource_Pipeline_Creation()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Creation");
}

void RenderInterface_DX12::Create_Resource_Pipeline_Passthrough()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Passthrough");
	RMLUI_ASSERT(this->m_p_device && "must be valid when we call this method!");
	RMLUI_ASSERT(Rml::GetFileInterface() && "must be valid when we call this method!");

	auto* p_filesystem = Rml::GetFileInterface();

	if (this->m_p_device && p_filesystem)
	{
		D3D12_DESCRIPTOR_RANGE ranges[1];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_DESCRIPTOR_TABLE table{};
		table.NumDescriptorRanges = sizeof(ranges) / sizeof(decltype(ranges[0]));
		table.pDescriptorRanges = ranges;

		D3D12_ROOT_PARAMETER parameters[1];
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[0].DescriptorTable = table;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC desc_rootsignature;
		desc_rootsignature.Init(_countof(parameters), parameters, 1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ID3DBlob* p_signature{};
		ID3DBlob* p_error{};
		auto status = D3D12SerializeRootSignature(&desc_rootsignature, D3D_ROOT_SIGNATURE_VERSION_1, &p_signature, &p_error);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to D3D12SerializeRootSignature: %s",
				(char*)p_error->GetBufferPointer());
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3D12SerializeRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Passthrough)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_MSAA)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		if (p_signature)
		{
			p_signature->Release();
			p_signature = nullptr;
		}

		if (p_error)
		{
			p_error->Release();
			p_error = nullptr;
		}

		ID3DBlob* p_shader_vertex{};
		ID3DBlob* p_shader_pixel{};
		ID3DBlob* p_error_buff{};

		const D3D_SHADER_MACRO macros[] = {"RMLUI_PREMULTIPLIED_ALPHA", NULL, NULL, NULL};

		status = D3DCompile(pShaderSourceText_Vertex_PassThrough, sizeof(pShaderSourceText_Vertex_PassThrough), nullptr, macros, nullptr, "main",
			"vs_5_0", this->m_default_shader_flags, 0, &p_shader_vertex, &p_error_buff);
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
		}
	#endif

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		status = D3DCompile(pShaderSourceText_Pixel_Passthrough, sizeof(pShaderSourceText_Pixel_Passthrough), nullptr, nullptr, nullptr, "main",
			"ps_5_0", this->m_default_shader_flags, 0, &p_shader_pixel, &p_error_buff);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s",
				(char*)(p_error_buff->GetBufferPointer()));
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		D3D12_SHADER_BYTECODE desc_bytecode_pixel_shader = {};
		desc_bytecode_pixel_shader.BytecodeLength = p_shader_pixel->GetBufferSize();
		desc_bytecode_pixel_shader.pShaderBytecode = p_shader_pixel->GetBufferPointer();

		D3D12_SHADER_BYTECODE desc_bytecode_vertex_shader = {};
		desc_bytecode_vertex_shader.BytecodeLength = p_shader_vertex->GetBufferSize();
		desc_bytecode_vertex_shader.pShaderBytecode = p_shader_vertex->GetBufferPointer();

		D3D12_INPUT_ELEMENT_DESC desc_input_layout_elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

		D3D12_INPUT_LAYOUT_DESC desc_input_layout = {};

		desc_input_layout.NumElements = sizeof(desc_input_layout_elements) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		desc_input_layout.pInputElementDescs = desc_input_layout_elements;

		D3D12_RASTERIZER_DESC desc_rasterizer = {};

		desc_rasterizer.AntialiasedLineEnable = FALSE;
		desc_rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc_rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		desc_rasterizer.DepthBias = 0;
		desc_rasterizer.DepthBiasClamp = 0;
		desc_rasterizer.DepthClipEnable = FALSE;
		desc_rasterizer.ForcedSampleCount = 0;
		desc_rasterizer.FrontCounterClockwise = FALSE;
		desc_rasterizer.MultisampleEnable = FALSE;
		desc_rasterizer.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		desc_rasterizer.SlopeScaledDepthBias = 0;

		D3D12_BLEND_DESC desc_blend_state = {};

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
			TRUE,
			FALSE,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = defaultRenderTargetBlendDesc;

		D3D12_DEPTH_STENCIL_DESC desc_depth_stencil = {};

		desc_depth_stencil.DepthEnable = FALSE;
		desc_depth_stencil.StencilEnable = TRUE;
		desc_depth_stencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc_depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc_depth_stencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		desc_depth_stencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_pipeline = {};

		desc_pipeline.InputLayout = desc_input_layout;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Passthrough)];
		desc_pipeline.VS = desc_bytecode_vertex_shader;
		desc_pipeline.PS = desc_bytecode_pixel_shader;
		desc_pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc_pipeline.RTVFormats[0] = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		desc_pipeline.DSVFormat = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;

		// since it is used for presenting MSAA texture on screen, we create swapchain and all RTs as NO-MSAA, keep this in mind
		// otherwise it is wrong to mix target texture with different sample count than swapchain's
		desc_pipeline.SampleDesc.Count = 1;
		desc_pipeline.SampleDesc.Quality = 0;

		desc_pipeline.SampleMask = 0xffffffff;
		desc_pipeline.RasterizerState = desc_rasterizer;
		desc_pipeline.BlendState = desc_blend_state;
		desc_pipeline.NumRenderTargets = 1;
		desc_pipeline.DepthStencilState = desc_depth_stencil;

		status =
			this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline, IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Passthrough)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Passthrough)");

	#ifdef RMLUI_DX_DEBUG
		this->m_root_signatures[static_cast<int>(ProgramId::Passthrough)]->SetName(TEXT("rs of Passthrough"));
		this->m_pipelines[static_cast<int>(ProgramId::Passthrough)]->SetName(TEXT("pipeline Passthrough"));
	#endif

		desc_pipeline.SampleDesc = this->m_desc_sample;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Passthrough_MSAA)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Passthrough_MSAA)");

	#ifdef RMLUI_DX_DEBUG
		this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_MSAA)]->SetName(TEXT("rs of Passthrough_MSAA"));
		this->m_pipelines[static_cast<int>(ProgramId::Passthrough_MSAA)]->SetName(TEXT("pipeline Passthrough_MSAA"));
	#endif
	}
}

void RenderInterface_DX12::Create_Resource_Pipeline_Passthrough_ColorMask()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Passthrough");
	RMLUI_ASSERT(this->m_p_device && "must be valid when we call this method!");
	RMLUI_ASSERT(Rml::GetFileInterface() && "must be valid when we call this method!");

	auto* p_filesystem = Rml::GetFileInterface();

	if (this->m_p_device && p_filesystem)
	{
		D3D12_DESCRIPTOR_RANGE ranges[1];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_DESCRIPTOR_TABLE table{};
		table.NumDescriptorRanges = sizeof(ranges) / sizeof(decltype(ranges[0]));
		table.pDescriptorRanges = ranges;

		D3D12_ROOT_PARAMETER parameters[1];
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[0].DescriptorTable = table;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC desc_rootsignature;
		desc_rootsignature.Init(_countof(parameters), parameters, 1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ID3DBlob* p_signature{};
		ID3DBlob* p_error{};
		auto status = D3D12SerializeRootSignature(&desc_rootsignature, D3D_ROOT_SIGNATURE_VERSION_1, &p_signature, &p_error);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to D3D12SerializeRootSignature: %s",
				(char*)p_error->GetBufferPointer());
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3D12SerializeRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_ColorMask)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		if (p_signature)
		{
			p_signature->Release();
			p_signature = nullptr;
		}

		if (p_error)
		{
			p_error->Release();
			p_error = nullptr;
		}

		ID3DBlob* p_shader_vertex{};
		ID3DBlob* p_shader_pixel{};
		ID3DBlob* p_error_buff{};

		const D3D_SHADER_MACRO macros[] = {"RMLUI_PREMULTIPLIED_ALPHA", NULL, NULL, NULL};

		status = D3DCompile(pShaderSourceText_Vertex_PassThrough, sizeof(pShaderSourceText_Vertex_PassThrough), nullptr, macros, nullptr, "main",
			"vs_5_0", this->m_default_shader_flags, 0, &p_shader_vertex, &p_error_buff);
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
		}
	#endif

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		status = D3DCompile(pShaderSourceText_Pixel_Passthrough, sizeof(pShaderSourceText_Pixel_Passthrough), nullptr, nullptr, nullptr, "main",
			"ps_5_0", this->m_default_shader_flags, 0, &p_shader_pixel, &p_error_buff);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s",
				(char*)(p_error_buff->GetBufferPointer()));
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		D3D12_SHADER_BYTECODE desc_bytecode_pixel_shader = {};
		desc_bytecode_pixel_shader.BytecodeLength = p_shader_pixel->GetBufferSize();
		desc_bytecode_pixel_shader.pShaderBytecode = p_shader_pixel->GetBufferPointer();

		D3D12_SHADER_BYTECODE desc_bytecode_vertex_shader = {};
		desc_bytecode_vertex_shader.BytecodeLength = p_shader_vertex->GetBufferSize();
		desc_bytecode_vertex_shader.pShaderBytecode = p_shader_vertex->GetBufferPointer();

		D3D12_INPUT_ELEMENT_DESC desc_input_layout_elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

		D3D12_INPUT_LAYOUT_DESC desc_input_layout = {};

		desc_input_layout.NumElements = sizeof(desc_input_layout_elements) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		desc_input_layout.pInputElementDescs = desc_input_layout_elements;

		D3D12_RASTERIZER_DESC desc_rasterizer = {};

		desc_rasterizer.AntialiasedLineEnable = FALSE;
		desc_rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc_rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		desc_rasterizer.DepthBias = 0;
		desc_rasterizer.DepthBiasClamp = 0;
		desc_rasterizer.DepthClipEnable = FALSE;
		desc_rasterizer.ForcedSampleCount = 0;
		desc_rasterizer.FrontCounterClockwise = FALSE;
		desc_rasterizer.MultisampleEnable = FALSE;
		desc_rasterizer.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		desc_rasterizer.SlopeScaledDepthBias = 0;

		D3D12_BLEND_DESC desc_blend_state = {};

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc;

		defaultRenderTargetBlendDesc.BlendEnable = TRUE;
		defaultRenderTargetBlendDesc.LogicOpEnable = FALSE;
		defaultRenderTargetBlendDesc.SrcBlend = D3D12_BLEND_BLEND_FACTOR;
		defaultRenderTargetBlendDesc.DestBlend = D3D12_BLEND_ZERO;
		defaultRenderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		defaultRenderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_BLEND_FACTOR;
		defaultRenderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		defaultRenderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		defaultRenderTargetBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		defaultRenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = defaultRenderTargetBlendDesc;

		D3D12_DEPTH_STENCIL_DESC desc_depth_stencil = {};

		desc_depth_stencil.DepthEnable = FALSE;
		desc_depth_stencil.StencilEnable = TRUE;
		desc_depth_stencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc_depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc_depth_stencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		desc_depth_stencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_pipeline = {};

		desc_pipeline.InputLayout = desc_input_layout;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_ColorMask)];
		desc_pipeline.VS = desc_bytecode_vertex_shader;
		desc_pipeline.PS = desc_bytecode_pixel_shader;
		desc_pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc_pipeline.RTVFormats[0] = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		desc_pipeline.DSVFormat = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;

		// since it is used for presenting MSAA texture on screen, we create swapchain and all RTs as NO-MSAA, keep this in mind
		// otherwise it is wrong to mix target texture with different sample count than swapchain's
		desc_pipeline.SampleDesc.Count = 1;
		desc_pipeline.SampleDesc.Quality = 0;

		desc_pipeline.SampleMask = 0xffffffff;
		desc_pipeline.RasterizerState = desc_rasterizer;
		desc_pipeline.BlendState = desc_blend_state;
		desc_pipeline.NumRenderTargets = 1;
		desc_pipeline.DepthStencilState = desc_depth_stencil;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Passthrough_ColorMask)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Passthrough_ColorMask)");

	#ifdef RMLUI_DX_DEBUG
		this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_ColorMask)]->SetName(TEXT("rs of Passthrough_ColorMask"));
		this->m_pipelines[static_cast<int>(ProgramId::Passthrough_ColorMask)]->SetName(TEXT("pipeline Passthrough_ColorMask"));
	#endif
	}
}

void RenderInterface_DX12::Create_Resource_Pipeline_Passthrough_NoBlend()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Passthrough");
	RMLUI_ASSERT(this->m_p_device && "must be valid when we call this method!");
	RMLUI_ASSERT(Rml::GetFileInterface() && "must be valid when we call this method!");

	auto* p_filesystem = Rml::GetFileInterface();

	if (this->m_p_device && p_filesystem)
	{
		D3D12_DESCRIPTOR_RANGE ranges[1];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_DESCRIPTOR_TABLE table{};
		table.NumDescriptorRanges = sizeof(ranges) / sizeof(decltype(ranges[0]));
		table.pDescriptorRanges = ranges;

		D3D12_ROOT_PARAMETER parameters[1];
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[0].DescriptorTable = table;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC desc_rootsignature;
		desc_rootsignature.Init(_countof(parameters), parameters, 1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ID3DBlob* p_signature{};
		ID3DBlob* p_error{};
		auto status = D3D12SerializeRootSignature(&desc_rootsignature, D3D_ROOT_SIGNATURE_VERSION_1, &p_signature, &p_error);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to D3D12SerializeRootSignature: %s",
				(char*)p_error->GetBufferPointer());
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3D12SerializeRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_NoBlend)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		if (p_signature)
		{
			p_signature->Release();
			p_signature = nullptr;
		}

		if (p_error)
		{
			p_error->Release();
			p_error = nullptr;
		}

		ID3DBlob* p_shader_vertex{};
		ID3DBlob* p_shader_pixel{};
		ID3DBlob* p_error_buff{};

		const D3D_SHADER_MACRO macros[] = {"RMLUI_PREMULTIPLIED_ALPHA", NULL, NULL, NULL};

		status = D3DCompile(pShaderSourceText_Vertex_PassThrough, sizeof(pShaderSourceText_Vertex_PassThrough), nullptr, macros, nullptr, "main",
			"vs_5_0", this->m_default_shader_flags, 0, &p_shader_vertex, &p_error_buff);
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
		}
	#endif

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		status = D3DCompile(pShaderSourceText_Pixel_Passthrough, sizeof(pShaderSourceText_Pixel_Passthrough), nullptr, nullptr, nullptr, "main",
			"ps_5_0", this->m_default_shader_flags, 0, &p_shader_pixel, &p_error_buff);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s",
				(char*)(p_error_buff->GetBufferPointer()));
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		D3D12_SHADER_BYTECODE desc_bytecode_pixel_shader = {};
		desc_bytecode_pixel_shader.BytecodeLength = p_shader_pixel->GetBufferSize();
		desc_bytecode_pixel_shader.pShaderBytecode = p_shader_pixel->GetBufferPointer();

		D3D12_SHADER_BYTECODE desc_bytecode_vertex_shader = {};
		desc_bytecode_vertex_shader.BytecodeLength = p_shader_vertex->GetBufferSize();
		desc_bytecode_vertex_shader.pShaderBytecode = p_shader_vertex->GetBufferPointer();

		D3D12_INPUT_ELEMENT_DESC desc_input_layout_elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

		D3D12_INPUT_LAYOUT_DESC desc_input_layout = {};

		desc_input_layout.NumElements = sizeof(desc_input_layout_elements) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		desc_input_layout.pInputElementDescs = desc_input_layout_elements;

		D3D12_RASTERIZER_DESC desc_rasterizer = {};

		desc_rasterizer.AntialiasedLineEnable = FALSE;
		desc_rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc_rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		desc_rasterizer.DepthBias = 0;
		desc_rasterizer.DepthBiasClamp = 0;
		desc_rasterizer.DepthClipEnable = FALSE;
		desc_rasterizer.ForcedSampleCount = 0;
		desc_rasterizer.FrontCounterClockwise = FALSE;
		desc_rasterizer.MultisampleEnable = FALSE;
		desc_rasterizer.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		desc_rasterizer.SlopeScaledDepthBias = 0;

		D3D12_BLEND_DESC desc_blend_state = {};

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
			FALSE,
			FALSE,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = defaultRenderTargetBlendDesc;

		D3D12_DEPTH_STENCIL_DESC desc_depth_stencil = {};

		desc_depth_stencil.DepthEnable = FALSE;
		desc_depth_stencil.StencilEnable = TRUE;
		desc_depth_stencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc_depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc_depth_stencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		desc_depth_stencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_pipeline = {};

		desc_pipeline.InputLayout = desc_input_layout;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_NoBlend)];
		desc_pipeline.VS = desc_bytecode_vertex_shader;
		desc_pipeline.PS = desc_bytecode_pixel_shader;
		desc_pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc_pipeline.RTVFormats[0] = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		desc_pipeline.DSVFormat = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
		desc_pipeline.SampleDesc = this->m_desc_sample; // depends on layer texture and layer texture depends on msaa level that user specifies
		desc_pipeline.SampleMask = 0xffffffff;
		desc_pipeline.RasterizerState = desc_rasterizer;
		desc_pipeline.BlendState = desc_blend_state;
		desc_pipeline.NumRenderTargets = 1;
		desc_pipeline.DepthStencilState = desc_depth_stencil;

		status = this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline,
			IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Passthrough_NoBlend)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Passthrough_NoBlend)");

	#ifdef RMLUI_DX_DEBUG
		this->m_root_signatures[static_cast<int>(ProgramId::Passthrough_NoBlend)]->SetName(TEXT("rs of Passthrough_NoBlend"));
		this->m_pipelines[static_cast<int>(ProgramId::Passthrough_NoBlend)]->SetName(TEXT("pipeline Passthrough_NoBlend"));
	#endif
	}
}

void RenderInterface_DX12::Create_Resource_Pipeline_ColorMatrix()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_ColorMatrix");
}

void RenderInterface_DX12::Create_Resource_Pipeline_BlendMask()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_BlendMask");
}

void RenderInterface_DX12::Create_Resource_Pipeline_Blur()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Blur");

	RMLUI_ASSERT(this->m_p_device && "must be valid when we call this method!");
	RMLUI_ASSERT(Rml::GetFileInterface() && "must be valid when we call this method!");

	auto* p_filesystem = Rml::GetFileInterface();

	if (this->m_p_device && p_filesystem)
	{
		D3D12_DESCRIPTOR_RANGE ranges[1];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_DESCRIPTOR_TABLE table{};
		table.NumDescriptorRanges = sizeof(ranges) / sizeof(decltype(ranges[0]));
		table.pDescriptorRanges = ranges;

		D3D12_ROOT_PARAMETER parameters[1];
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[0].DescriptorTable = table;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC desc_rootsignature;
		desc_rootsignature.Init(_countof(parameters), parameters, 1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ID3DBlob* p_signature{};
		ID3DBlob* p_error{};
		auto status = D3D12SerializeRootSignature(&desc_rootsignature, D3D_ROOT_SIGNATURE_VERSION_1, &p_signature, &p_error);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to D3D12SerializeRootSignature: %s",
				(char*)p_error->GetBufferPointer());
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3D12SerializeRootSignature");

		status = this->m_p_device->CreateRootSignature(0, p_signature->GetBufferPointer(), p_signature->GetBufferSize(),
			IID_PPV_ARGS(&this->m_root_signatures[static_cast<int>(ProgramId::Blur)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateRootSignature");

		if (p_signature)
		{
			p_signature->Release();
			p_signature = nullptr;
		}

		if (p_error)
		{
			p_error->Release();
			p_error = nullptr;
		}

		ID3DBlob* p_shader_vertex{};
		ID3DBlob* p_shader_pixel{};
		ID3DBlob* p_error_buff{};

		const D3D_SHADER_MACRO macros[] = {NULL, NULL, NULL, NULL};

		status = D3DCompile(pShaderSourceText_Vertex_Blur, sizeof(pShaderSourceText_Vertex_Blur), nullptr, macros, nullptr, "main",
			"vs_5_0", this->m_default_shader_flags, 0, &p_shader_vertex, &p_error_buff);
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
		}
	#endif

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		status = D3DCompile(pShaderSourceText_Pixel_Blur, sizeof(pShaderSourceText_Pixel_Blur), nullptr, nullptr, nullptr, "main",
			"ps_5_0", this->m_default_shader_flags, 0, &p_shader_pixel, &p_error_buff);
	#ifdef RMLUI_DX_DEBUG
		if (FAILED(status))
		{
			Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12][ERROR] failed to compile shader: %s",
				(char*)(p_error_buff->GetBufferPointer()));
		}
	#endif
		RMLUI_DX_ASSERTMSG(status, "failed to D3DCompile");

		if (p_error_buff)
		{
			p_error_buff->Release();
			p_error_buff = nullptr;
		}

		D3D12_SHADER_BYTECODE desc_bytecode_pixel_shader = {};
		desc_bytecode_pixel_shader.BytecodeLength = p_shader_pixel->GetBufferSize();
		desc_bytecode_pixel_shader.pShaderBytecode = p_shader_pixel->GetBufferPointer();

		D3D12_SHADER_BYTECODE desc_bytecode_vertex_shader = {};
		desc_bytecode_vertex_shader.BytecodeLength = p_shader_vertex->GetBufferSize();
		desc_bytecode_vertex_shader.pShaderBytecode = p_shader_vertex->GetBufferPointer();

		D3D12_INPUT_ELEMENT_DESC desc_input_layout_elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

		D3D12_INPUT_LAYOUT_DESC desc_input_layout = {};

		desc_input_layout.NumElements = sizeof(desc_input_layout_elements) / sizeof(D3D12_INPUT_ELEMENT_DESC);
		desc_input_layout.pInputElementDescs = desc_input_layout_elements;

		D3D12_RASTERIZER_DESC desc_rasterizer = {};

		desc_rasterizer.AntialiasedLineEnable = FALSE;
		desc_rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc_rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		desc_rasterizer.DepthBias = 0;
		desc_rasterizer.DepthBiasClamp = 0;
		desc_rasterizer.DepthClipEnable = FALSE;
		desc_rasterizer.ForcedSampleCount = 0;
		desc_rasterizer.FrontCounterClockwise = FALSE;
		desc_rasterizer.MultisampleEnable = FALSE;
		desc_rasterizer.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		desc_rasterizer.SlopeScaledDepthBias = 0;

		D3D12_BLEND_DESC desc_blend_state = {};

		desc_blend_state.AlphaToCoverageEnable = FALSE;
		desc_blend_state.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
			TRUE,
			FALSE,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
	#ifdef RMLUI_PREMULTIPLIED_ALPHA
			D3D12_BLEND_ONE,
	#else
			D3D12_BLEND_SRC_ALPHA,
	#endif
			D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc_blend_state.RenderTarget[i] = defaultRenderTargetBlendDesc;

		D3D12_DEPTH_STENCIL_DESC desc_depth_stencil = {};

		desc_depth_stencil.DepthEnable = FALSE;
		desc_depth_stencil.StencilEnable = TRUE;
		desc_depth_stencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc_depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc_depth_stencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		desc_depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		desc_depth_stencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		desc_depth_stencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc_depth_stencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_pipeline = {};

		desc_pipeline.InputLayout = desc_input_layout;
		desc_pipeline.pRootSignature = this->m_root_signatures[static_cast<int>(ProgramId::Passthrough)];
		desc_pipeline.VS = desc_bytecode_vertex_shader;
		desc_pipeline.PS = desc_bytecode_pixel_shader;
		desc_pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc_pipeline.RTVFormats[0] = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
		desc_pipeline.DSVFormat = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;

		desc_pipeline.SampleDesc = this->m_desc_sample;

		desc_pipeline.SampleMask = 0xffffffff;
		desc_pipeline.RasterizerState = desc_rasterizer;
		desc_pipeline.BlendState = desc_blend_state;
		desc_pipeline.NumRenderTargets = 1;
		desc_pipeline.DepthStencilState = desc_depth_stencil;

		status =
			this->m_p_device->CreateGraphicsPipelineState(&desc_pipeline, IID_PPV_ARGS(&this->m_pipelines[static_cast<int>(ProgramId::Blur)]));
		RMLUI_DX_ASSERTMSG(status, "failed to CreateGraphicsPipelineState (Blur)");

	#ifdef RMLUI_DX_DEBUG
		this->m_root_signatures[static_cast<int>(ProgramId::Blur)]->SetName(TEXT("rs of Blur"));
		this->m_pipelines[static_cast<int>(ProgramId::Blur)]->SetName(TEXT("pipeline Blur"));
	#endif
	}
}

void RenderInterface_DX12::Create_Resource_Pipeline_DropShadow()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_DropShadow");
}

void RenderInterface_DX12::Create_Resource_Pipeline_Count()
{
	RMLUI_ZoneScopedN("DirectX 12 - Create_Resource_Pipeline_Count");
}

void RenderInterface_DX12::Destroy_Resource_Pipelines()
{
	RMLUI_ZoneScopedN("DirectX 12 - Destroy_Resource_Pipelines");
	this->Destroy_Resource_For_Shaders();

	if (this->m_p_depthstencil_resource)
	{
		this->Destroy_Resource_DepthStencil();
	}

	for (int i = 1; i < static_cast<int>(ProgramId::Count); ++i)
	{
		if (this->m_pipelines[i])
		{
			this->m_pipelines[i]->Release();
			this->m_pipelines[i] = nullptr;
		}

		if (this->m_root_signatures[i])
		{
			this->m_root_signatures[i]->Release();
			this->m_root_signatures[i] = nullptr;
		}
	}
}

ID3DBlob* RenderInterface_DX12::Compile_Shader(const Rml::String& relative_path_to_shader, const char* entry_point, const char* shader_version,
	UINT flags)
{
	RMLUI_ZoneScopedN("DirectX 12 - Compile_Shader");
	return nullptr;
}

bool RenderInterface_DX12::CheckTearingSupport() noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - CheckTearingSupport");
	int result{};

	IDXGIFactory4* pFactory4{};

	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory4))))
	{
		IDXGIFactory5* pFactory5{};

		if (SUCCEEDED(pFactory4->QueryInterface(IID_PPV_ARGS(&pFactory5))))
		{
			if (FAILED(pFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &result, sizeof(result))))
			{
				result = 0;
			}

			if (pFactory5)
			{
				pFactory5->Release();
			}
		}
	}

	if (pFactory4)
	{
		pFactory4->Release();
	}

	return result == 1;
}

IDXGIAdapter* RenderInterface_DX12::Get_Adapter(bool is_use_warp) noexcept
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_Adapter");
	IDXGIFactory4* p_factory{};

	uint32_t create_flags{};

	#ifdef RMLUI_DX_DEBUG
	create_flags = DXGI_CREATE_FACTORY_DEBUG;
	#endif

	RMLUI_DX_ASSERTMSG(CreateDXGIFactory2(create_flags, IID_PPV_ARGS(&p_factory)), "failed to CreateDXGIFactory2");

	IDXGIAdapter* p_adapter{};
	IDXGIAdapter1* p_adapter1{};
	IDXGIAdapter4* p_adapter4{};

	if (is_use_warp)
	{
		if (p_factory)
		{
			RMLUI_DX_ASSERTMSG(p_factory->EnumWarpAdapter(IID_PPV_ARGS(&p_adapter)), "failed to EnumAdapters");
			RMLUI_ASSERT(p_adapter && "returned invalid COM object from EnumWarpAdapter");

			if (p_adapter)
			{
				p_adapter->QueryInterface(IID_PPV_ARGS(&p_adapter4));
				RMLUI_ASSERT(p_adapter4 && "returned invalid COM object from QueryInterface");
				this->PrintAdapterDesc(p_adapter);
			}
		}
	}
	else
	{
		if (p_factory)
		{
			size_t max_dedicated_video_memory{};

			for (uint32_t i = 0; p_factory->EnumAdapters(i, &p_adapter) != DXGI_ERROR_NOT_FOUND; ++i)
			{
				if (p_adapter)
				{
					DXGI_ADAPTER_DESC1 desc;

					RMLUI_DX_ASSERTMSG(p_adapter->QueryInterface(IID_PPV_ARGS(&p_adapter1)), "failed to QueryInterface of IDXGIAdapter1");

					if (p_adapter1)
					{
						p_adapter1->GetDesc1(&desc);

						ID3D12Device* p_test_device{};

						if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
							SUCCEEDED(D3D12CreateDevice(p_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&p_test_device))) &&
							desc.DedicatedVideoMemory > max_dedicated_video_memory)
						{
							max_dedicated_video_memory = desc.DedicatedVideoMemory;

							// found 'device' with bigger memory means we found our discrete video card (not cpu integrated)
							if (p_adapter4)
							{
								p_adapter4->Release();
								p_adapter4 = nullptr;
							}

							RMLUI_DX_ASSERTMSG(p_adapter->QueryInterface(IID_PPV_ARGS(&p_adapter4)), "failed to QueryInterface of IDXGIAdapter4");

							this->PrintAdapterDesc(p_adapter);
						}

						if (p_test_device)
						{
							p_test_device->Release();
						}
					}

					if (p_adapter)
					{
						p_adapter->Release();
					}

					if (p_adapter1)
					{
						p_adapter1->Release();
						p_adapter1 = nullptr;
					}
				}
			}
		}
	}

	if (p_factory)
	{
		p_factory->Release();
	}

	if (p_adapter)
	{
		p_adapter->Release();
	}

	if (p_adapter1)
	{
		p_adapter1->Release();
	}

	return p_adapter4;
}

void RenderInterface_DX12::PrintAdapterDesc(IDXGIAdapter* p_adapter)
{
	RMLUI_ZoneScopedN("DirectX 12 - PrintAdapterDesc");
	RMLUI_ASSERT(p_adapter && "you can't pass invalid argument");

	if (p_adapter)
	{
		DXGI_ADAPTER_DESC desc;
		p_adapter->GetDesc(&desc);

		char p_converted[_countof(desc.Description)];
		memset(p_converted, 0, sizeof(p_converted));

	#ifdef UNICODE
		sprintf(p_converted, "%ls", desc.Description);
	#else
		p_converted = desc.Description;
	#endif

		float vid_mem_in_bytes = desc.DedicatedVideoMemory;
		float vid_mem_in_kilobytes = vid_mem_in_bytes / 1024.0f;
		float vid_mem_in_megabytes = vid_mem_in_kilobytes / 1024.0f;
		float vid_mem_in_gigabytes = vid_mem_in_megabytes / 1024.0f;

		Rml::Log::Message(Rml::Log::LT_INFO, "Monitor[%s]\n VideoMemory[%f Bytes][%f Mbs][%f Gbs]\n", p_converted, vid_mem_in_bytes,
			vid_mem_in_megabytes, vid_mem_in_gigabytes);
	}
}

#endif

void RenderInterface_DX12::SetScissor(Rml::Rectanglei region, bool vertically_flip)
{
	RMLUI_ZoneScopedN("DirectX 12 - SetScissor");

	static int call_count = 0;
	++call_count;
	char msg[32];
	std::sprintf(msg, "::SetScissor=%d\n", call_count);
	OutputDebugStringA(msg);

	if (region.Valid() != m_scissor.Valid())
	{
		if (!region.Valid())
		{
			this->m_is_scissor_was_set = false;
			return;
		}
	}

	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "SetScissor");

	if (region.Valid() && vertically_flip)
	{
		region = VerticallyFlipped(region, this->m_height);
	}

	if (region.Valid() && region != this->m_scissor)
	{
		if (this->m_p_command_graphics_list)
		{
			D3D12_RECT scissor;

			const int x = Rml::Math::Clamp(region.Left(), 0, this->m_width);
			const int y = Rml::Math::Clamp(this->m_height - region.Bottom(), 0, this->m_height);

			scissor.left = x;
			scissor.right = x + region.Width();
			scissor.bottom = this->m_height - y;
			scissor.top = this->m_height - (y + region.Height());

#if RMLUI_RENDER_BACKEND_FIELD_IGNORE_RENDERER_INVALID_STATES == 1
			if (region.p0.y == region.p1.y || region.p0.x == region.p1.x)
			{
				if (static_cast<Gfx::RenderState>(this->m_render_state) == Gfx::RenderState::Valid)
				{
					this->m_render_state = static_cast<unsigned char>(Gfx::RenderState::Invalid_Scissor);

					this->m_is_scissor_was_set = true;
					RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);
					return;
				}
			}
			else
			{
				if (static_cast<Gfx::RenderState>(this->m_render_state) == Gfx::RenderState::Invalid_Scissor)
				{
					this->m_render_state = static_cast<unsigned char>(Gfx::RenderState::Valid);
				}
			}
#endif

			this->m_p_command_graphics_list->RSSetScissorRects(1, &scissor);
			char msg[256];
			std::sprintf(msg, "GraphicsQueue::RSSetScissorRects(%d,%d,%d,%d) | region.Valid() && region != this->m_scissor\n", scissor.left,
				scissor.right, scissor.bottom, scissor.top);
			OutputDebugStringA(msg);
			this->m_is_scissor_was_set = true;
		}
	}

	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

	// this->m_scissor = region;
}

void RenderInterface_DX12::SubmitTransformUniform(ConstantBufferType& constant_buffer, const Rml::Vector2f& translation)
{
	RMLUI_ZoneScopedN("DirectX 12 - SubmitTransformUniform");
	static_assert((size_t)ProgramId::Count < RMLUI_RENDER_BACKEND_FIELD_MAXNUMPROGRAMS, "Maximum number of pipelines exceeded");

	size_t program_index = (size_t)this->m_active_program_id;

	std::uint8_t* p_gpu_binding_start = reinterpret_cast<std::uint8_t*>(constant_buffer.Get_GPU_StartMemoryForBindingData());

	// if (this->m_program_state_transform_dirty.test(program_index))
	{
		RMLUI_ASSERT(p_gpu_binding_start &&
			"your allocated constant buffer must contain a valid pointer of beginning mapping of its GPU buffer. Otherwise you destroyed it!");

		if (p_gpu_binding_start)
		{
			std::uint8_t* p_gpu_binding_offset_to_transform = p_gpu_binding_start + constant_buffer.Get_AllocInfo().Get_Offset();

			std::memcpy(p_gpu_binding_offset_to_transform, this->m_constant_buffer_data_transform.data(),
				sizeof(this->m_constant_buffer_data_transform));
		}

		//	this->m_program_state_transform_dirty.set(program_index, false);
	}

	if (p_gpu_binding_start)
	{
		std::uint8_t* p_gpu_binding_offset_to_translate =
			p_gpu_binding_start + (constant_buffer.Get_AllocInfo().Get_Offset() + sizeof(this->m_constant_buffer_data_transform));

		std::memcpy(p_gpu_binding_offset_to_translate, &translation.x, sizeof(translation));
	}
}

void RenderInterface_DX12::UseProgram(ProgramId pipeline_id)
{
	RMLUI_ZoneScopedN("DirectX 12 - UseProgram");
	RMLUI_ASSERT(pipeline_id < ProgramId::Count && "overflow, too big value for indexing");

	RMLUI_DX_MARKER_BEGIN(this->m_p_command_graphics_list, "UseProgram");
	if (pipeline_id != ProgramId::None)
	{
		RMLUI_ASSERT(this->m_pipelines[static_cast<int>(pipeline_id)] && "you forgot to initialize or deleted!");
		RMLUI_ASSERT(this->m_root_signatures[static_cast<int>(pipeline_id)] && "you forgot to initialize or deleted!");

		this->m_p_command_graphics_list->SetPipelineState(this->m_pipelines[static_cast<int>(pipeline_id)]);
		this->m_p_command_graphics_list->SetGraphicsRootSignature(this->m_root_signatures[static_cast<int>(pipeline_id)]);

		ID3D12DescriptorHeap* p_heaps[] = {this->m_p_descriptor_heap_shaders};
		this->m_p_command_graphics_list->SetDescriptorHeaps(_countof(p_heaps), p_heaps);
	}
	RMLUI_DX_MARKER_END(this->m_p_command_graphics_list);

	this->m_active_program_id = pipeline_id;
}

RenderInterface_DX12::ConstantBufferType* RenderInterface_DX12::Get_ConstantBuffer(uint32_t current_back_buffer_index)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_ConstantBuffer");
	RMLUI_ASSERT(current_back_buffer_index != uint32_t(-1) && "invalid index!");

	auto current_constant_buffer_index = this->m_constant_buffer_count_per_frame[current_back_buffer_index];

	auto max_index = RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS - 1;

	if (this->m_constantbuffers[current_back_buffer_index].size() > RMLUI_RENDER_BACKEND_FIELD_PREALLOCATED_CONSTANTBUFFERS)
		max_index = this->m_constantbuffers[current_back_buffer_index].size() - 1;

	if (current_constant_buffer_index > max_index)
	{
		// resizing...
		for (auto& vec : this->m_constantbuffers)
		{
			vec.emplace_back(std::move(ConstantBufferType()));
		}

#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] allocated new constant buffer instance for frame[%d], current size of storage[%zu]",
			current_back_buffer_index, this->m_constantbuffers.at(current_back_buffer_index).size());
#endif
	}

	auto& vec_cbs = this->m_constantbuffers.at(current_back_buffer_index);

	ConstantBufferType* p_result = &vec_cbs[current_constant_buffer_index];

	if (p_result->Get_AllocInfo().Get_BufferIndex() == -1)
	{
		const auto& info_alloc = this->m_manager_buffer.Alloc_ConstantBuffer(p_result, kAllocationSizeMax_ConstantBuffer);
		p_result->Set_AllocInfo(info_alloc);
	}

	++this->m_constant_buffer_count_per_frame[current_back_buffer_index];

	return p_result;
}

RenderInterface_DX12* RmlDX12::Initialize(Rml::String* out_message, Backend::RmlRenderInitInfo* p_info)
{
	RMLUI_ZoneScopedN("DirectX 12 - RmlDX12::Initialize");
	RenderInterface_DX12* p_result{};

	if (p_info)
	{
		if (p_info->Is_FullInitialization())
		{
			RMLUI_ASSERT(p_info->Get_WindowHandle() && "you must pass a valid window handle!");

			auto& settings = p_info->Get_Settings();

			if (settings.msaa_sample_count == 0)
			{
				settings.msaa_sample_count = RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT;
			}

			p_result = new RenderInterface_DX12(p_info->Get_WindowHandle(), settings.vsync, settings.msaa_sample_count);
		}
		else
		{
			RMLUI_ASSERT(p_info->Get_UserDevice() && "you must pass a valid pointer of Device");

			auto& settings = p_info->Get_Settings();

			if (settings.msaa_sample_count == 0)
			{
				settings.msaa_sample_count = RMLUI_RENDER_BACKEND_FIELD_MSAA_SAMPLE_COUNT;
			}

			p_result = new RenderInterface_DX12(p_info->Get_WindowHandle(), static_cast<ID3D12Device*>(p_info->Get_UserDevice()), nullptr,
				settings.vsync, settings.msaa_sample_count);
		}
	}

	p_result->Initialize();

	return p_result;
}

void RmlDX12::Shutdown(RenderInterface_DX12* p_instance)
{
	RMLUI_ZoneScopedN("DirectX 12 - RmlDX12::Shutdown");
	RMLUI_ASSERT(p_instance && "you must have a valid instance");

	if (p_instance)
	{
		p_instance->Shutdown();
	}
}

RenderInterface_DX12::BufferMemoryManager::BufferMemoryManager() :
	m_descriptor_increment_size_srv_cbv_uav{}, m_size_for_allocation_in_bytes{}, m_size_alignment_in_bytes{}, m_p_device{}, m_p_allocator{},
	m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav{}, m_p_start_pointer_of_descriptor_heap_srv_cbv_uav{}
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Constructor");
}

RenderInterface_DX12::BufferMemoryManager::~BufferMemoryManager()
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Destructor");
}

bool RenderInterface_DX12::BufferMemoryManager::Is_Initialized(void) const
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferManagerManager::Is_Initialized");
	return static_cast<bool>(this->m_p_device != nullptr);
}

void RenderInterface_DX12::BufferMemoryManager::Initialize(ID3D12Device* p_device, D3D12MA::Allocator* p_allocator,
	OffsetAllocator::Allocator* p_offset_allocator_for_descriptor_heap_srv_cbv_uav, CD3DX12_CPU_DESCRIPTOR_HANDLE* p_handle,
	uint32_t size_descriptor_srv_cbv_uav, size_t size_for_allocation, size_t size_alignment)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Initialize");
	RMLUI_ASSERT(p_allocator && "must be valid!");
	RMLUI_ASSERT(size_for_allocation && "must be greater than 0");
	RMLUI_ASSERT(size_alignment && "must be greater than 0");
	RMLUI_ASSERT(p_offset_allocator_for_descriptor_heap_srv_cbv_uav && "must be valid!");
	RMLUI_ASSERT(p_handle && "must be valid!");
	RMLUI_ASSERT(p_device && "must be valid!");

	this->m_p_allocator = p_allocator;
	this->m_size_for_allocation_in_bytes = size_for_allocation;
	this->m_size_alignment_in_bytes = size_alignment;
	this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav = p_offset_allocator_for_descriptor_heap_srv_cbv_uav;
	this->m_p_start_pointer_of_descriptor_heap_srv_cbv_uav = p_handle;
	this->m_descriptor_increment_size_srv_cbv_uav = size_descriptor_srv_cbv_uav;
	this->m_p_device = p_device;

	this->Alloc_Buffer(size_for_allocation

#ifdef RMLUI_DX_DEBUG
		,
		Rml::String("buffer[") + Rml::ToString(this->m_buffers.size()) + "]"
#endif
	);
}

void RenderInterface_DX12::BufferMemoryManager::Shutdown()
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Shutdown");
	for (auto& pair : m_buffers)
	{
		auto* p_allocation = pair.first;

		if (p_allocation)
		{
			if (p_allocation->GetResource())
			{
				p_allocation->GetResource()->Unmap(0, nullptr);
				p_allocation->GetResource()->Release();
			}

			auto ref_count = p_allocation->Release();
			RMLUI_ASSERT(ref_count == 0 && "leak!");
		}
	}

	for (auto* p_block : m_virtual_buffers)
	{
		if (p_block)
		{
			auto ref_count = p_block->Release();
			RMLUI_ASSERT(ref_count == 0 && "leak! (virtual block)");
		}
	}

	if (this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav)
	{
		this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav = nullptr;
	}

	if (this->m_p_allocator)
	{
		this->m_p_allocator = nullptr;
	}
}

void RenderInterface_DX12::BufferMemoryManager::Alloc_Vertex(const void* p_data, int num_vertices, size_t size_of_one_element_in_p_data,
	GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Alloc_Vertex");
	RMLUI_ASSERT(p_data && "data for mapping to buffer must valid!");
	RMLUI_ASSERT(num_vertices && "amount of vertices must be greater than zero!");
	RMLUI_ASSERT(size_of_one_element_in_p_data > 0 && "size of one element must be greater than 0");
	RMLUI_ASSERT(p_handle && "must be valid!");

	if (p_handle)
	{
		RMLUI_ASSERT(p_handle->Get_InfoVertex().Get_BufferIndex() == -1 &&
			"info is already initialized that means you didn't destroy your buffer! Something is wrong!");

		p_handle->Set_NumVertices(num_vertices);
		p_handle->Set_SizeOfOneVertex(size_of_one_element_in_p_data);

		GraphicsAllocationInfo info;
		this->Alloc(info, num_vertices * size_of_one_element_in_p_data);

		void* p_writable_part = this->Get_WritableMemoryFromBufferByOffset(info);

		RMLUI_ASSERT(p_writable_part && "something is wrong!");

		if (p_writable_part)
		{
			std::memcpy(p_writable_part, p_data, info.Get_Size());
		}

		p_handle->Set_InfoVertex(info);

#ifdef RMLUI_DX_DEBUG
		auto* p_block = this->m_virtual_buffers.at(info.Get_BufferIndex());

		RMLUI_ASSERT(p_block && "can't be invalid!");

		D3D12MA::Statistics stats;
		p_block->GetStatistics(&stats);

		auto available_memory = stats.BlockBytes - stats.AllocationBytes;

		Rml::Log::Message(Rml::Log::Type::LT_DEBUG,
			"[DirectX-12] allocated vertex buffer with size[%zu] (in bytes) in buffer[%d] available memory for this buffer [%zu] (in bytes)",
			info.Get_Size(), info.Get_BufferIndex(), available_memory);
#endif
	}
}

void RenderInterface_DX12::BufferMemoryManager::Alloc_Index(const void* p_data, int num_vertices, size_t size_of_one_element_in_p_data,
	GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Alloc_Index");
	RMLUI_ASSERT(p_data && "data for mapping to buffer must valid!");
	RMLUI_ASSERT(num_vertices && "amount of vertices must be greater than zero!");
	RMLUI_ASSERT(size_of_one_element_in_p_data > 0 && "size of one element must be greater than 0");
	RMLUI_ASSERT(p_handle && "must be valid!");

	if (p_handle)
	{
		RMLUI_ASSERT(p_handle->Get_InfoIndex().Get_BufferIndex() == -1 &&
			"info is already initialized that means you didn't destroy your buffer! Something is wrong!");

		p_handle->Set_NumIndecies(num_vertices);
		p_handle->Set_SizeOfOneIndex(size_of_one_element_in_p_data);

		GraphicsAllocationInfo info;
		this->Alloc(info, num_vertices * size_of_one_element_in_p_data);

		void* p_writable_part = this->Get_WritableMemoryFromBufferByOffset(info);

		RMLUI_ASSERT(p_writable_part && "something is wrong!");

		if (p_writable_part)
		{
			std::memcpy(p_writable_part, p_data, num_vertices * size_of_one_element_in_p_data);
		}

		p_handle->Set_InfoIndex(info);

#ifdef RMLUI_DX_DEBUG
		auto* p_block = this->m_virtual_buffers.at(info.Get_BufferIndex());

		RMLUI_ASSERT(p_block && "can't be invalid!");

		D3D12MA::Statistics stats;
		p_block->GetStatistics(&stats);

		auto available_memory = stats.BlockBytes - stats.AllocationBytes;

		Rml::Log::Message(Rml::Log::Type::LT_DEBUG,
			"[DirectX-12] allocated index buffer with size[%zu] (in bytes) in buffer[%d] available memory for this buffer [%zu] (in bytes)",
			info.Get_Size(), info.Get_BufferIndex(), available_memory);
#endif
	}
}

RenderInterface_DX12::GraphicsAllocationInfo RenderInterface_DX12::BufferMemoryManager::Alloc_ConstantBuffer(ConstantBufferType* p_resource,
	size_t size)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Alloc_ConstantBuffer");
	RMLUI_ASSERT(!this->m_buffers.empty() && "you forgot to allocate buffer on initialize stage of this manager!");
	RMLUI_ASSERT(p_resource && "must be valid!");

	GraphicsAllocationInfo result;
	auto result_index = this->Alloc(result, size, 256);

	if (p_resource)
	{
		if (result_index != -1)
		{
#ifdef RMLUI_DX_DEBUG
			auto* p_block = this->m_virtual_buffers.at(result.Get_BufferIndex());

			RMLUI_ASSERT(p_block && "can't be invalid!");

			D3D12MA::Statistics stats;
			p_block->GetStatistics(&stats);

			auto available_memory = stats.BlockBytes - stats.AllocationBytes;

			Rml::Log::Message(Rml::Log::Type::LT_DEBUG,
				"[DirectX-12] allocated constant buffer with size[%zu] (in bytes) in buffer[%d] available memory for this buffer [%zu] (in bytes)",
				result.Get_Size(), result.Get_BufferIndex(), available_memory);
#endif

			auto* p_dx_allocation = this->m_buffers.at(result_index).first;
			auto* p_dx_resource = p_dx_allocation->GetResource();

			RMLUI_ASSERT(p_dx_allocation && "something is broken!");
			RMLUI_ASSERT(p_dx_resource && "something is broken!");

			p_resource->Set_GPU_StartMemoryForBindingData(this->m_buffers.at(result_index).second);

			/* todo: delete
			if (p_dx_resource)
			{
			    auto descriptor_allocation =
			        this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav->allocate(this->m_descriptor_increment_size_srv_cbv_uav);

			    // should we really here use microsoft-style's casting???
			    auto offset_pointer =
			        SIZE_T(INT64(this->m_p_start_pointer_of_descriptor_heap_srv_cbv_uav->ptr) + INT64(descriptor_allocation.offset));

			    D3D12_CONSTANT_BUFFER_VIEW_DESC desc_cbv = {};
			    desc_cbv.BufferLocation = p_dx_resource->GetGPUVirtualAddress() + result.Get_Offset();
			    desc_cbv.SizeInBytes = size;

			    D3D12_CPU_DESCRIPTOR_HANDLE cast;
			    cast.ptr = offset_pointer;

			    this->m_p_device->CreateConstantBufferView(&desc_cbv, cast);

			    p_resource->Set_Allocation_DescriptorHeap(descriptor_allocation);
			}
			*/
		}
	}

	return result;
}

void RenderInterface_DX12::BufferMemoryManager::Free_ConstantBuffer(ConstantBufferType* p_constantbuffer)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Free_ConstantBuffer");
	RMLUI_ASSERT(p_constantbuffer && "you must pass a valid object!");

	if (p_constantbuffer)
	{
		const auto& info = p_constantbuffer->Get_AllocInfo();
		RMLUI_ASSERT(info.Get_BufferIndex() != -1 && "must be valid data of this info");

#ifdef RMLUI_DX_DEBUG
		Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] deallocated constant buffer with size[%zu] in buffer[%d]", info.Get_Size(),
			info.Get_BufferIndex());
#endif

		// todo: delete
		//	this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav->free(p_constantbuffer->Get_Allocation_DescriptorHeap());

		if (this->m_virtual_buffers.empty() == false)
		{
			auto* p_block = this->m_virtual_buffers.at(info.Get_BufferIndex());

			if (p_block)
			{
				p_block->FreeAllocation(info.Get_VirtualAllocation());

				GraphicsAllocationInfo invalidate;
				p_constantbuffer->Set_AllocInfo(invalidate);
			}
		}
	}
}

void RenderInterface_DX12::BufferMemoryManager::Free_Geometry(GeometryHandleType* p_handle)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Free_Geometry");
	RMLUI_ASSERT(p_handle && "must be valid!");
	RMLUI_ASSERT(p_handle->Get_InfoVertex().Get_BufferIndex() != -1 && "not initialized, maybe you passing twice for deallocation?");
	RMLUI_ASSERT(p_handle->Get_InfoIndex().Get_BufferIndex() != -1 && "not initialized, maybe you passing twice for deallocation?");

	if (p_handle)
	{
		const auto& info_vertex = p_handle->Get_InfoVertex();
		const auto& info_index = p_handle->Get_InfoIndex();

		if (this->m_virtual_buffers.empty() == false)
		{
			auto* p_block_vertex = this->m_virtual_buffers.at(info_vertex.Get_BufferIndex());
			auto* p_block_index = this->m_virtual_buffers.at(info_index.Get_BufferIndex());

			if (p_block_vertex)
			{
#ifdef RMLUI_DX_DEBUG
				Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] deallocated vertex buffer with size[%zu] in buffer[%d]",
					info_vertex.Get_Size(), info_vertex.Get_BufferIndex());
#endif

				p_block_vertex->FreeAllocation(info_vertex.Get_VirtualAllocation());

				GraphicsAllocationInfo invalidate;
				p_handle->Set_InfoVertex(invalidate);
			}

			if (p_block_index)
			{
#ifdef RMLUI_DX_DEBUG
				Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] deallocated index buffer with size[%zu] in buffer[%d]",
					info_index.Get_Size(), info_index.Get_BufferIndex());
#endif

				p_block_index->FreeAllocation(info_index.Get_VirtualAllocation());

				GraphicsAllocationInfo invalidate;
				p_handle->Set_InfoIndex(invalidate);
			}

			// this->Free_ConstantBuffer(&p_handle->Get_ConstantBuffer());
		}
	}
}

void* RenderInterface_DX12::BufferMemoryManager::Get_WritableMemoryFromBufferByOffset(const GraphicsAllocationInfo& info)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Get_WritableMemoryFromBufferByOffset");
	RMLUI_ASSERT(info.Get_BufferIndex() != -1 && "you pass not initialized graphics allocation info!");

	void* p_result{};
	if (info.Get_BufferIndex() != -1)
	{
		std::uint8_t* p_begin = reinterpret_cast<std::uint8_t*>(this->m_buffers.at(info.Get_BufferIndex()).second);

		RMLUI_ASSERT(p_begin && "being pointer is not valid! it's terribly wrong thing!!!!");

		p_result = p_begin + info.Get_Offset();
	}

	return p_result;
}

D3D12MA::Allocation* RenderInterface_DX12::BufferMemoryManager::Get_BufferByIndex(int buffer_index)
{
	RMLUI_ZoneScopedN("DirectX 12 - Get_BufferByIndex");
	RMLUI_ASSERT(buffer_index >= 0 && "index must be valid!");
	RMLUI_ASSERT(buffer_index < this->m_buffers.size() && "overflow index!");

	D3D12MA::Allocation* p_result{};

	if (buffer_index >= 0)
	{
		if (buffer_index < this->m_buffers.size())
		{
			p_result = this->m_buffers.at(buffer_index).first;
		}
	}

	return p_result;
}

void RenderInterface_DX12::BufferMemoryManager::Alloc_Buffer(size_t size
#ifdef RMLUI_DX_DEBUG
	,
	const Rml::String& debug_name
#endif
)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Alloc_Buffer");
	RMLUI_ASSERT(size && "must be greater than 0");

	D3D12_RESOURCE_DESC desc_constantbuffer = {};
	desc_constantbuffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc_constantbuffer.Alignment = 0;
	desc_constantbuffer.Width = size;
	desc_constantbuffer.Height = 1;
	desc_constantbuffer.DepthOrArraySize = 1;
	desc_constantbuffer.MipLevels = 1;
	desc_constantbuffer.Format = DXGI_FORMAT_UNKNOWN;
	desc_constantbuffer.SampleDesc.Count = 1;
	desc_constantbuffer.SampleDesc.Quality = 0;
	desc_constantbuffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc_constantbuffer.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC desc_allocation = {};
	desc_allocation.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	ID3D12Resource* p_resource{};
	D3D12MA::Allocation* p_allocation{};

	auto result = this->m_p_allocator->CreateResource(&desc_allocation, &desc_constantbuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		&p_allocation, IID_PPV_ARGS(&p_resource));

	RMLUI_DX_ASSERTMSG(result, "failed to CreateResource");

	void* p_begin_writable_data{};
	CD3DX12_RANGE range(0, 0);
	result = p_allocation->GetResource()->Map(0, &range, &p_begin_writable_data);

	RMLUI_DX_ASSERTMSG(result, "failed to ID3D12Resource::Map");

#ifdef RMLUI_DX_DEBUG
	p_allocation->SetName(RmlWin32::ConvertToUTF16(debug_name).c_str());
#endif

	this->m_buffers.push_back({p_allocation, p_begin_writable_data});

	D3D12MA::VIRTUAL_BLOCK_DESC desc_virtual = {};
	desc_virtual.Size = size;

	D3D12MA::VirtualBlock* p_block{};
	result = D3D12MA::CreateVirtualBlock(&desc_virtual, &p_block);

	RMLUI_DX_ASSERTMSG(result, "failed to D3D12MA::CreateVirtualBlock");

	this->m_virtual_buffers.push_back(p_block);
}

D3D12MA::VirtualBlock* RenderInterface_DX12::BufferMemoryManager::Get_AvailableBlock(size_t size_for_allocation, int* result_index)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Get_AvailableBlock");
	RMLUI_ASSERT(result_index && "must be valid part of memory!");

	D3D12MA::VirtualBlock* p_result{};

	int index{};
	for (auto* p_block : this->m_virtual_buffers)
	{
		if (p_block)
		{
			D3D12MA::Statistics stats;
			p_block->GetStatistics(&stats);

			if ((stats.BlockBytes - stats.AllocationBytes) >= size_for_allocation)
			{
				p_result = p_block;
				*result_index = index;
				break;
			}
		}
		++index;
	}

	return p_result;
}

D3D12MA::VirtualBlock* RenderInterface_DX12::BufferMemoryManager::Get_NotOutOfMemoryAndAvailableBlock(size_t size_for_allocation, int* result_index)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Get_NotOutOfMemoryAndAvailableBlock");
	RMLUI_ASSERT(result_index && "must be valid part of memory!");
	RMLUI_ASSERT(*result_index != -1,
		"use this method when you found of available block then tried to allocate from it but got out of memory status!");

	D3D12MA::VirtualBlock* p_result{};

	// we skip out of memory block since it shows to us as available
	auto from = *result_index + 1;

	for (int i = from; i < this->m_virtual_buffers.size(); ++i)
	{
		auto* p_block = this->m_virtual_buffers.at(i);
		if (p_block)
		{
			D3D12MA::Statistics stats;
			p_block->GetStatistics(&stats);

			if ((stats.BlockBytes - stats.AllocationBytes) >= size_for_allocation)
			{
				p_result = p_block;
				*result_index = i;
				break;
			}
		}
	}

	return p_result;
}

int RenderInterface_DX12::BufferMemoryManager::Alloc(GraphicsAllocationInfo& info, size_t size, size_t alignment)
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::Alloc");
	RMLUI_ASSERT(!this->m_buffers.empty() && "you forgot to allocate buffer on initialize stage of this manager!");

	// we don't want to use any recursions because it is slow af
	constexpr int kHowManyRequestsWeCanDoForResolvingOutOfMemory = 15;
	int result_index{-1};

	if (alignment > 0)
		size = AlignUp(size, alignment);

	if (!this->m_buffers.empty())
	{
		auto* p_block = this->Get_AvailableBlock(size, &result_index);

		if (p_block)
		{
			D3D12MA::VIRTUAL_ALLOCATION_DESC desc_alloc = {};
			desc_alloc.Size = size;

			if (alignment % 2 == 0)
				desc_alloc.Alignment = alignment;

			D3D12MA::VirtualAllocation alloc;
			UINT64 offset{};

			// TODO: make auto allocation system switchable for user when initialize render
			// TODO: provide option need to allocate buffers if failed after first depth iterations?
			// TODO: provide option for auto resolving large allocated size (otherwise if disabled and situation comes user will get assert)
			auto status = p_block->Allocate(&desc_alloc, &alloc, &offset);

			if (status == E_OUTOFMEMORY)
			{
				bool bWasSucFoundInLoop{};
				for (int i = 0; i < kHowManyRequestsWeCanDoForResolvingOutOfMemory; ++i)
				{
					p_block = this->Get_NotOutOfMemoryAndAvailableBlock(size, &result_index);

					if (!p_block)
					{
						if (size > this->m_size_for_allocation_in_bytes)
						{
#ifdef RMLUI_DX_DEBUG
							Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] auto correction size for buffer from [%zu] to [%zu]",
								this->m_size_for_allocation_in_bytes, size);
#endif

							this->m_size_for_allocation_in_bytes = size;
						}

						this->Alloc_Buffer(this->m_size_for_allocation_in_bytes
#ifdef RMLUI_DX_DEBUG
							,
							Rml::String("buffer[") + Rml::ToString(this->m_buffers.size()) + "]"
#endif
						);
						result_index = this->m_buffers.size() - 1;
					}

					p_block = this->m_virtual_buffers.at(result_index);

					desc_alloc = {};
					desc_alloc.Size = size;

					if (alignment % 2 == 0)
						desc_alloc.Alignment = alignment;

					auto status = p_block->Allocate(&desc_alloc, &alloc, &offset);

					if (status == E_OUTOFMEMORY)
					{
						continue;
					}
					else if (status == S_OK)
					{
						bWasSucFoundInLoop = true;
						break;
					}
#ifdef RMLUI_DX_DEBUG
					else
					{
						RMLUI_ASSERT(false && "report to github");
					}
#endif
				}
			}

			// our heuristic depth iteration failed then, just allocate buffers
			if (offset >= UINT64_MAX)
			{
				bool bWasSucFoundInLoop{};
				for (int i = 0; i < kHowManyRequestsWeCanDoForResolvingOutOfMemory; ++i)
				{
					p_block = this->Get_NotOutOfMemoryAndAvailableBlock(size, &result_index);

					if (!p_block)
					{
						if (size > this->m_size_for_allocation_in_bytes)
						{
#ifdef RMLUI_DX_DEBUG
							Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] auto correction size for buffer from [%zu] to [%zu]",
								this->m_size_for_allocation_in_bytes, size);
#endif

							this->m_size_for_allocation_in_bytes = size;
						}

						this->Alloc_Buffer(this->m_size_for_allocation_in_bytes
#ifdef RMLUI_DX_DEBUG
							,
							Rml::String("buffer[") + Rml::ToString(this->m_buffers.size()) + "]"
#endif
						);
						result_index = this->m_buffers.size() - 1;
					}

					p_block = this->m_virtual_buffers.at(result_index);

					desc_alloc = {};
					desc_alloc.Size = size;

					if (alignment % 2 == 0)
						desc_alloc.Alignment = alignment;

					auto status = p_block->Allocate(&desc_alloc, &alloc, &offset);

					if (status == E_OUTOFMEMORY)
					{
						continue;
					}
					else if (status == S_OK)
					{
						bWasSucFoundInLoop = true;
						break;
					}
#ifdef RMLUI_DX_DEBUG
					else
					{
						RMLUI_ASSERT(false && "report to github");
					}
#endif
				}

				RMLUI_ASSERT(offset == UINT64_MAX &&
					"it was last greedy try for allocating, it is really hard case for handling (report and describe your case on github), try to "
					"optimize your 'stuff' (very calmly saying) "
					"by your own, it is only for really rare specials cases where user doesn't want to think but UI must survive no matter what");
			}

			info.Set_Size(size);
			info.Set_VirtualAllocation(alloc);
			info.Set_Offset(offset);
			info.Set_BufferIndex(result_index);
		}
		else
		{
			if (size > this->m_size_for_allocation_in_bytes)
			{
#ifdef RMLUI_DX_DEBUG
				Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] auto correction size for buffer from [%zu] to [%zu]",
					this->m_size_for_allocation_in_bytes, size);
#endif

				this->m_size_for_allocation_in_bytes = size;
			}

			this->Alloc_Buffer(this->m_size_for_allocation_in_bytes
#ifdef RMLUI_DX_DEBUG
				,
				Rml::String("buffer[") + Rml::ToString(this->m_buffers.size()) + "]"
#endif
			);

			result_index = -1;
			p_block = this->Get_AvailableBlock(size, &result_index);

			RMLUI_ASSERT(p_block && "can't be because in previous line of code you added new allocated fresh buffer!");

			if (p_block)
			{
				D3D12MA::VIRTUAL_ALLOCATION_DESC desc_alloc = {};
				desc_alloc.Size = size;

				if (alignment % 2 == 0)
					desc_alloc.Alignment = alignment;

				D3D12MA::VirtualAllocation alloc;
				UINT64 offset{};

				auto status = p_block->Allocate(&desc_alloc, &alloc, &offset);

				RMLUI_DX_ASSERTMSG(status, "failed to Allocate");

				info.Set_Size(size);
				info.Set_VirtualAllocation(alloc);
				info.Set_Offset(offset);
				info.Set_BufferIndex(result_index);
			}
		}

		this->TryToFreeAvailableBlock();
	}

	return result_index;
}

void RenderInterface_DX12::BufferMemoryManager::TryToFreeAvailableBlock()
{
	RMLUI_ZoneScopedN("DirectX 12 - BufferMemoryManager::TryToFreeAvailableBlock");
	Rml::Array<std::pair<Rml::Vector<D3D12MA::VirtualBlock*>::const_iterator, Rml::Vector<Rml::Pair<D3D12MA::Allocation*, void*>>::const_iterator>, 1>
		max_for_free;

	for (size_t i = 0; i < max_for_free.size(); ++i)
	{
		max_for_free[i].first = this->m_virtual_buffers.end();
		max_for_free[i].second = this->m_buffers.end();
	}

	int total_count{};
	int limit_for_break{max_for_free.size()};
	int index{};

	for (auto cur = this->m_virtual_buffers.begin(); cur != this->m_virtual_buffers.end(); ++cur)
	{
		if (total_count == limit_for_break)
			break;

		if (*cur)
		{
			auto* p_block = *cur;
			D3D12MA::Statistics stats;
			p_block->GetStatistics(&stats);

			if (stats.AllocationCount == 0 && stats.BlockCount == 0)
			{
				auto ref_count = p_block->Release();
				RMLUI_ASSERT(ref_count == 0, "leak");

				ref_count = this->m_buffers.at(index).first->Release();
				RMLUI_ASSERT(ref_count == 0, "leak");

				this->m_buffers.at(index).second = nullptr;

				max_for_free[total_count].first = cur;
				max_for_free[total_count].second = this->m_buffers.begin() + index;

				++total_count;
			}
		}
		++index;
	}

	for (int i = 0; i < total_count; ++i)
	{
		auto& pair = max_for_free.at(i);

		this->m_virtual_buffers.erase(pair.first);
		this->m_buffers.erase(pair.second);
	}
}

RenderInterface_DX12::TextureMemoryManager::TextureMemoryManager() :
	m_size_for_placed_heap{}, m_size_limit_for_being_placed{}, m_size_srv_cbv_uav_descriptor{}, m_size_rtv_descriptor{}, m_size_dsv_descriptor{},
	m_fence_value{}, m_p_allocator{}, m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav{}, m_p_device{}, m_p_command_list{},
	m_p_command_allocator{}, m_p_descriptor_heap_srv{}, m_p_copy_queue{}, m_p_fence{}, m_p_fence_event{}, m_p_handle{}, m_p_renderer{},
	m_p_virtual_block_for_render_target_heap_allocations{}, m_p_virtual_block_for_depth_stencil_heap_allocations{}, m_p_descriptor_heap_rtv{}
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Constructor");
}

RenderInterface_DX12::TextureMemoryManager::~TextureMemoryManager()
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Destructor");
}

bool RenderInterface_DX12::TextureMemoryManager::Is_Initialized(void) const
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Is_Initialized");
	return static_cast<bool>(this->m_p_device != nullptr);
}

void RenderInterface_DX12::TextureMemoryManager::Initialize(D3D12MA::Allocator* p_allocator,
	OffsetAllocator::Allocator* p_offset_allocator_for_descriptor_heap_srv_cbv_uav, ID3D12Device* p_device, ID3D12GraphicsCommandList* p_command_list,
	ID3D12CommandAllocator* p_allocator_command, ID3D12DescriptorHeap* p_descriptor_heap_srv, ID3D12DescriptorHeap* p_descriptor_heap_rtv,
	ID3D12DescriptorHeap* p_descriptor_heap_dsv, ID3D12CommandQueue* p_copy_queue, CD3DX12_CPU_DESCRIPTOR_HANDLE* p_handle,
	RenderInterface_DX12* p_renderer, size_t size_for_placed_heap)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Initialize");
	RMLUI_ASSERT(p_allocator && "you must pass a valid allocator pointer");
	RMLUI_ASSERT(size_for_placed_heap > 0 && "there's no point in creating in such small heap");
	RMLUI_ASSERT(size_for_placed_heap != size_t(-1), "invalid value!");
	RMLUI_ASSERT(p_device && "must be valid!");
	RMLUI_ASSERT(p_command_list && "must be valid!");
	RMLUI_ASSERT(p_allocator_command && "must be valid!");
	RMLUI_ASSERT(p_descriptor_heap_srv && "must be valid!");
	RMLUI_ASSERT(p_copy_queue && "must be valid!");
	RMLUI_ASSERT(p_handle && "must be valid!");
	RMLUI_ASSERT(p_renderer && "must be valid!");
	RMLUI_ASSERT(p_offset_allocator_for_descriptor_heap_srv_cbv_uav && "must be valid!");
	RMLUI_ASSERT(p_descriptor_heap_rtv && "must be valid!");
	RMLUI_ASSERT(p_descriptor_heap_dsv && "must be valid!");

	this->m_p_device = p_device;
	this->m_p_allocator = p_allocator;
	this->m_p_command_list = p_command_list;
	this->m_p_command_allocator = p_allocator_command;
	this->m_size_for_placed_heap = size_for_placed_heap;
	this->m_p_descriptor_heap_srv = p_descriptor_heap_srv;
	this->m_p_descriptor_heap_rtv = p_descriptor_heap_rtv;
	this->m_p_descriptor_heap_dsv = p_descriptor_heap_dsv;
	this->m_p_copy_queue = p_copy_queue;
	this->m_p_renderer = p_renderer;
	this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav = p_offset_allocator_for_descriptor_heap_srv_cbv_uav;

	// if I have 4 Mb then 1 Mb is optimal as a maximum size for determing that resource can be allocated as placed resource so we just take 25% from
	// size_for_placed_heap value
	this->m_size_limit_for_being_placed = size_t((double(size_for_placed_heap) / 100.0) * 25.0);

	if (this->m_p_device)
	{
		this->m_size_srv_cbv_uav_descriptor = this->m_p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		this->m_size_rtv_descriptor = this->m_p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		this->m_size_dsv_descriptor = this->m_p_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	this->m_p_handle = p_handle;

	RMLUI_ASSERT(this->m_size_srv_cbv_uav_descriptor > 0 && "must be positive");
	RMLUI_ASSERT(this->m_size_rtv_descriptor > 0 && "must be positive");

	RMLUI_ASSERT(this->m_size_limit_for_being_placed > 0 && this->m_size_limit_for_being_placed != size_t(-1), "something is wrong!");

	if (this->m_p_device)
	{
		RMLUI_DX_ASSERTMSG(this->m_p_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_p_fence)),
			"failed to create fence for upload queue");

		this->m_p_fence_event = CreateEvent(NULL, FALSE, FALSE, TEXT("Event for copy queue fence"));
	}

	if (this->m_p_descriptor_heap_rtv)
	{
		D3D12MA::VIRTUAL_BLOCK_DESC desc_block{};
		desc_block.Size = this->m_size_rtv_descriptor * RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV;

		auto status = D3D12MA::CreateVirtualBlock(&desc_block, &this->m_p_virtual_block_for_render_target_heap_allocations);

		RMLUI_DX_ASSERTMSG(status, "failed to D3D12MA::CreateVirtualBlock (for rtv descriptor heap, texture manager)");
	}

	if (this->m_p_descriptor_heap_dsv)
	{
		D3D12MA::VIRTUAL_BLOCK_DESC desc_block{};
		desc_block.Size = this->m_size_dsv_descriptor * RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_DSV;

		auto status = D3D12MA::CreateVirtualBlock(&desc_block, &this->m_p_virtual_block_for_depth_stencil_heap_allocations);

		RMLUI_DX_ASSERTMSG(status, "failed to D3D12MA::CreateVirtualBlock (for dsv descriptor heap, texture manager)");
	}
}

void RenderInterface_DX12::TextureMemoryManager::Shutdown()
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Shutdown");
	if (this->m_p_fence)
	{
		this->m_p_fence->Release();
		this->m_p_fence = nullptr;
	}

	if (this->m_p_fence_event)
	{
		CloseHandle(this->m_p_fence_event);
		this->m_p_fence_event = nullptr;
	}

	size_t index{};
	for (auto* p_heap : this->m_heaps_placed)
	{
		p_heap->Release();
		auto ref_count = this->m_blocks.at(index)->Release();

		RMLUI_ASSERT(ref_count == 0 && "leak");
		++index;
	}

	if (this->m_p_virtual_block_for_render_target_heap_allocations)
	{
		auto ref_count = this->m_p_virtual_block_for_render_target_heap_allocations->Release();
		RMLUI_ASSERT(ref_count == 0 && "leak");
	}

	if (this->m_p_virtual_block_for_depth_stencil_heap_allocations)
	{
		auto ref_count = this->m_p_virtual_block_for_depth_stencil_heap_allocations->Release();
		RMLUI_ASSERT(ref_count == 0 && "leak");
	}

	this->m_blocks.clear();
	this->m_heaps_placed.clear();

	this->m_p_allocator = nullptr;
	this->m_p_device = nullptr;
	this->m_p_command_list = nullptr;
	this->m_p_command_allocator = nullptr;
	this->m_p_descriptor_heap_srv = nullptr;
	this->m_p_copy_queue = nullptr;
	this->m_p_handle = nullptr;
	this->m_p_renderer = nullptr;
	this->m_p_descriptor_heap_rtv = nullptr;
	this->m_p_virtual_block_for_render_target_heap_allocations = nullptr;
	this->m_p_virtual_block_for_depth_stencil_heap_allocations = nullptr;
}

ID3D12Resource* RenderInterface_DX12::TextureMemoryManager::Alloc_Texture(D3D12_RESOURCE_DESC& desc, TextureHandleType* p_impl,
	const Rml::byte* p_data
#ifdef RMLUI_DX_DEBUG
	,
	const Rml::String& debug_name
#endif
)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Alloc_Texture");
	RMLUI_ASSERT(desc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
		"this manager doesn't support 1D or 3D textures. (why do we need to support it?)");
	RMLUI_ASSERT(desc.DepthOrArraySize <= 1 && "we don't support a such sizes");
	RMLUI_ASSERT(desc.SampleDesc.Count == 1 && "this manager not for allocating render targets or depth stencil!");
	RMLUI_ASSERT(desc.Width && "must specify value for width field");
	RMLUI_ASSERT(desc.Height && "must specify value for height field");
	RMLUI_ASSERT(p_impl && "must be valid!");
	RMLUI_ASSERT(p_data && "must be valid!");

	ID3D12Resource* p_result{};

	// this size stands for real size of texture and how much it will occupy for allocated block, because if it is larger than 1 Mb it is better to
	// allocate as committed resource instead of using placed resource technique (by default we allocate block for 4Mb because 1.0 Mb textures can be
	// occupy 4 times and smaller even more times, but it is not reasonable to use placed resources for >1Mb textures)
	auto base_memory_size_for_allocation_in_bytes = desc.Width * desc.Height * this->BytesPerPixel(desc.Format);
	size_t total_memory_for_allocation = base_memory_size_for_allocation_in_bytes;
	size_t mipmemory = base_memory_size_for_allocation_in_bytes;
	for (int i = 2; i <= desc.MipLevels; ++i)
	{
		if (mipmemory <= 1)
		{
			break;
		}

		mipmemory /= 4;
		total_memory_for_allocation += mipmemory;
	}

#ifdef RMLUI_DX_DEBUG
	Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX-12] allocating texture with base memory[%zu] and total memory with mips levels[%zu]",
		base_memory_size_for_allocation_in_bytes, total_memory_for_allocation);
#endif

	// we need to pass full memory with mipmaps otherwise the DirectX API will validate it by its own. we must ensure what we do.
	if (this->CanBePlacedResource(total_memory_for_allocation))
	{
		this->Alloc_As_Placed(base_memory_size_for_allocation_in_bytes, total_memory_for_allocation, desc, p_impl, p_data);
		p_result = static_cast<ID3D12Resource*>(p_impl->Get_Resource());
	}
	else
	{
		this->Alloc_As_Committed(base_memory_size_for_allocation_in_bytes, total_memory_for_allocation, desc, p_impl, p_data);
		p_result = static_cast<D3D12MA::Allocation*>(p_impl->Get_Resource())->GetResource();
	}

	return p_result;
}

ID3D12Resource* RenderInterface_DX12::TextureMemoryManager::Alloc_Texture(D3D12_RESOURCE_DESC& desc, Gfx::FramebufferData* p_impl,
	D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state
#ifdef RMLUI_DEBUG
	,
	const Rml::String& debug_name
#endif
)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Alloc_Texture");
	RMLUI_ASSERT(desc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
		"this manager doesn't support 1D or 3D textures. (why do we need to support it?)");
	RMLUI_ASSERT(desc.DepthOrArraySize <= 1 && "we don't support a such sizes");
	RMLUI_ASSERT(desc.Width && "must specify value for width field");
	RMLUI_ASSERT(desc.Height && "must specify value for height field");
	RMLUI_ASSERT(p_impl && "must be valid!");
	RMLUI_ASSERT(p_impl->Get_Texture() &&
		"you must allocate Gfx::FramebufferData's field (texture) before calling this method! So you need to set your allocated pointer to "
		"Set_Texture method, dude...");

	ID3D12Resource* p_result{};

	auto base_memory_size_for_allocation_in_bytes = desc.Width * desc.Height * this->BytesPerPixel(desc.Format);
	size_t total_memory_for_allocation = base_memory_size_for_allocation_in_bytes;
	size_t mipmemory = base_memory_size_for_allocation_in_bytes;
	for (int i = 2; i <= desc.MipLevels; ++i)
	{
		if (mipmemory <= 1)
		{
			break;
		}

		mipmemory /= 4;
		total_memory_for_allocation += mipmemory;
	}

	desc.Flags = flags;

#ifdef RMLUI_DX_DEBUG
	bool is_rt = flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	bool is_ds = flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	RMLUI_ASSERT(is_rt || is_ds && "must be allow for RT or DS otherwise you will get assert from DX12");

	const char* type_of_flag_for_texture_debug_name{};

	constexpr const char* hardcoded_name_render_target = "render target";
	constexpr const char* hardcoded_name_depth_stencil = "depth-stencil";
	constexpr const char* hardcoded_name_unknown = "unknown";

	if (is_rt)
	{
		type_of_flag_for_texture_debug_name = hardcoded_name_render_target;
	}
	else if (is_ds)
	{
		type_of_flag_for_texture_debug_name = hardcoded_name_depth_stencil;
	}
	else
	{
		type_of_flag_for_texture_debug_name = hardcoded_name_unknown;
	}

	Rml::Log::Message(Rml::Log::Type::LT_DEBUG,
		"[DirectX-12] allocating render2texture (%s) with base memory[%zu] and total memory with mips levels[%zu]",
		type_of_flag_for_texture_debug_name, base_memory_size_for_allocation_in_bytes, total_memory_for_allocation);
#endif

	this->Alloc_As_Committed(base_memory_size_for_allocation_in_bytes, total_memory_for_allocation, desc, initial_state, p_impl->Get_Texture(),
		p_impl);

	p_result = static_cast<D3D12MA::Allocation*>(p_impl->Get_Texture()->Get_Resource())->GetResource();

	return p_result;
}

void RenderInterface_DX12::TextureMemoryManager::Free_Texture(TextureHandleType* p_texture)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Free_Texture(TextureHandleType)");
	RMLUI_ASSERT(p_texture && "must be valid");

	if (p_texture)
	{
		auto index = p_texture->Get_Info().Get_BufferIndex();
		RMLUI_ASSERT(p_texture->Get_Resource() && "must be valid! can't be! data is corrupted! or was already destroyed!");

		if (index != -1)
		{
			this->m_blocks.at(index)->FreeAllocation(p_texture->Get_Info().Get_VirtualAllocation());
		}

		if (this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav)
		{
			this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav->free(p_texture->Get_Allocation_DescriptorHeap());
		}

		p_texture->Destroy();
	}
}

void RenderInterface_DX12::TextureMemoryManager::Free_Texture(Gfx::FramebufferData* p_impl)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Free_Texture(FramebufferData)");
	RMLUI_ASSERT(p_impl && "must be valid!");
	RMLUI_ASSERT(p_impl->Get_Texture() && "must be valid!");
	RMLUI_ASSERT(this->m_p_virtual_block_for_render_target_heap_allocations && "must be valid! early calling");
	RMLUI_ASSERT(this->m_p_virtual_block_for_depth_stencil_heap_allocations && "must be valid! early calling");

	if (p_impl)
	{
		this->Free_Texture(p_impl->Get_Texture(), p_impl->Is_RenderTarget(), *(p_impl->Get_VirtualAllocation_Descriptor()));

		delete p_impl->Get_Texture();
		p_impl->Set_Texture(nullptr);

		p_impl->Set_DescriptorResourceView({});
		p_impl->Set_ID(-1);
	}
}

void RenderInterface_DX12::TextureMemoryManager::Free_Texture(TextureHandleType* p_texture, bool is_rt, D3D12MA::VirtualAllocation& allocation)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Free_Texture(TextureHandleType,bool,D3D12MA::VirtualAllocation)");
	RMLUI_ASSERT(p_texture && "you must pass a valid pointer!");

	this->Free_Texture(p_texture);

	if (is_rt)
	{
		if (this->m_p_virtual_block_for_render_target_heap_allocations)
		{
			this->m_p_virtual_block_for_render_target_heap_allocations->FreeAllocation(allocation);
		}
	}
	else
	{
		if (this->m_p_virtual_block_for_depth_stencil_heap_allocations)
		{
			this->m_p_virtual_block_for_depth_stencil_heap_allocations->FreeAllocation(allocation);
		}
	}
}

bool RenderInterface_DX12::TextureMemoryManager::CanAllocate(size_t total_memory_for_allocation, D3D12MA::VirtualBlock* p_block)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::CanAllocate(size_t, D3D12MA::VirtualBlock)");
	RMLUI_ASSERT(total_memory_for_allocation > 0 && total_memory_for_allocation != size_t(-1), "must be a valid number!");

	RMLUI_ASSERT(p_block, "must be valid virtual block");

	bool result{};

	if (p_block)
	{
		D3D12MA::Statistics stats;
		p_block->GetStatistics(&stats);

		result = (stats.BlockBytes - stats.AllocationBytes) >= total_memory_for_allocation;
	}

	return result;
}

bool RenderInterface_DX12::TextureMemoryManager::CanBePlacedResource(size_t total_memory_for_allocation)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::CanBePlacedResource");
	RMLUI_ASSERT(total_memory_for_allocation > 0 && total_memory_for_allocation != size_t(-1), "must be a valid number!");

	bool result{};

	if (total_memory_for_allocation <= this->m_size_limit_for_being_placed)
		result = true;

	return result;
}

bool RenderInterface_DX12::TextureMemoryManager::CanBeSmallResource(size_t base_memory)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::CanBeSmallResource");
	RMLUI_ASSERT(base_memory > 0, "must be greater than zero!");
	RMLUI_ASSERT(base_memory != size_t(-1), "must be a valid number!");

	bool result{};

	constexpr size_t _kNonMSAASmallResourceMemoryLimit = 128 * 128 * 4;

	// if this is not MSAA texture (because for now we didn't implement a such support)
	if (base_memory <= _kNonMSAASmallResourceMemoryLimit)
		result = true;

	return result;
}

D3D12MA::VirtualBlock* RenderInterface_DX12::TextureMemoryManager::Get_AvailableBlock(size_t total_memory_for_allocation, int* result_index)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Get_AvailableBlock");
	RMLUI_ASSERT(this->m_p_device, "must be valid!");
	RMLUI_ASSERT(result_index, "must be valid!");
	RMLUI_ASSERT(total_memory_for_allocation <= this->m_size_limit_for_being_placed, "you can't pass a such size here!");
	RMLUI_ASSERT(this->m_size_limit_for_being_placed < this->m_size_for_placed_heap, "something is wrong and you initialized your manager wrong!!!!");

	D3D12MA::VirtualBlock* p_result{};

	if (this->m_blocks.empty())
	{
		RMLUI_ASSERT(this->m_heaps_placed.empty(), "if blocks are empty heaps must be too!");

		auto pair = this->Create_HeapPlaced(this->m_size_for_placed_heap);

		p_result = pair.second;
		*result_index = 0;
	}
	else
	{
		int index{};
		for (auto* p_block : this->m_blocks)
		{
			if (p_block)
			{
				if (this->CanAllocate(total_memory_for_allocation, p_block))
				{
					p_result = p_block;
					*result_index = index;

					break;
				}
				else
				{
					if (index >= this->m_blocks.size() - 1)
					{
						auto pair = this->Create_HeapPlaced(this->m_size_for_placed_heap);
						++index;
						*result_index = index;
						p_result = pair.second;

						break;
					}
				}
			}
			++index;
		}
	}

	return p_result;
}

void RenderInterface_DX12::TextureMemoryManager::Alloc_As_Committed(size_t base_memory, size_t total_memory, D3D12_RESOURCE_DESC& desc,
	TextureHandleType* p_impl, const Rml::byte* p_data)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Alloc_As_Committed");
	RMLUI_ASSERT(base_memory > 0 && "must be greater than zero!");
	RMLUI_ASSERT(total_memory > 0 && "must be greater than zero!");
	RMLUI_ASSERT(base_memory != size_t(-1) && "must be valid number!");
	RMLUI_ASSERT(total_memory != size_t(-1) && "must be valid number!");
	RMLUI_ASSERT(this->m_p_device && "must be valid!");
	RMLUI_ASSERT(p_impl && "must be valid!");
	RMLUI_ASSERT(this->m_p_command_allocator && "must be valid!");
	RMLUI_ASSERT(this->m_p_command_list && "must be valid!");
	RMLUI_ASSERT(this->m_p_allocator && "allocator must be valid!");

	if (this->m_p_allocator)
	{
		D3D12MA::ALLOCATION_DESC desc_allocation = {};
		desc_allocation.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		ID3D12Resource* p_resource{};
		D3D12MA::Allocation* p_allocation{};
		auto status = this->m_p_allocator->CreateResource(&desc_allocation, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, &p_allocation,
			IID_PPV_ARGS(&p_resource));

		RMLUI_DX_ASSERTMSG(status, "failed to CreateResource (D3D12MA)");

		if (p_impl)
		{
			p_impl->Set_Resource(p_allocation);
		}

		if (this->m_p_command_allocator)
		{
			auto status = this->m_p_command_allocator->Reset();
			RMLUI_DX_ASSERTMSG(status, "failed to Reset (command allocator)");
		}

		if (this->m_p_command_list)
		{
			auto status = this->m_p_command_list->Reset(this->m_p_command_allocator, nullptr);
			RMLUI_DX_ASSERTMSG(status, "failed to Reset (command list)");
		}

		this->Upload(true, p_impl, desc, p_data, p_resource);
	}
}

void RenderInterface_DX12::TextureMemoryManager::Alloc_As_Committed(size_t base_memory, size_t total_memory, D3D12_RESOURCE_DESC& desc,
	D3D12_RESOURCE_STATES initial_state, TextureHandleType* p_texture, Gfx::FramebufferData* p_impl)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Alloc_As_Committed");
	RMLUI_ASSERT(base_memory > 0, "must be greater than zero!");
	RMLUI_ASSERT(total_memory > 0, "must be greater than zero!");
	RMLUI_ASSERT(base_memory != size_t(-1), "must be valid number!");
	RMLUI_ASSERT(total_memory != size_t(-1), "must be valid number!");
	RMLUI_ASSERT(this->m_p_device, "must be valid!");
	RMLUI_ASSERT(p_impl, "must be valid!");
	RMLUI_ASSERT(this->m_p_command_allocator, "must be valid!");
	RMLUI_ASSERT(this->m_p_command_list, "must be valid!");
	RMLUI_ASSERT(this->m_p_allocator, "allocator must be valid!");
	RMLUI_ASSERT(p_texture && "must be valid!");

	if (this->m_p_allocator)
	{
		D3D12MA::ALLOCATION_DESC desc_allocation = {};
		desc_allocation.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_CLEAR_VALUE optimized_clear_value = {};

		if (p_impl->Is_RenderTarget())
		{
			optimized_clear_value.Format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;

			constexpr FLOAT color[] = {RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_RENDERTARGET_COLOR_VAlUE};

			optimized_clear_value.Color[0] = 0.0f;
			optimized_clear_value.Color[1] = 0.0f;
			optimized_clear_value.Color[2] = 0.0f;
			optimized_clear_value.Color[3] = 0.0f;
		}
		else
		{
			optimized_clear_value.Format = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
			optimized_clear_value.DepthStencil.Depth = RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_DEPTH_VALUE;
			optimized_clear_value.DepthStencil.Stencil = RMLUI_RENDER_BACKEND_FIELD_CLEAR_VALUE_DEPTHSTENCIL_STENCIL_VALUE;
		}
		
		ID3D12Resource* p_resource{};
		D3D12MA::Allocation* p_allocation{};
		auto status = this->m_p_allocator->CreateResource(&desc_allocation, &desc, initial_state, &optimized_clear_value, &p_allocation,
			IID_PPV_ARGS(&p_resource));

		RMLUI_DX_ASSERTMSG(status, "failed to CreateResource (D3D12MA) (RenderTargetTexture)");

		if (p_texture)
		{
			p_texture->Set_Resource(p_allocation);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC desc_srv{};
		desc_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		bool is_rt = desc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		bool is_ds = desc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		if (is_rt)
			desc_srv.Format = desc.Format;
		else if (is_ds)
			desc_srv.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

		if (desc.SampleDesc.Count > 1)
		{
			desc_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			desc_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		}

		desc_srv.Texture2D.MipLevels = desc.MipLevels;

		RMLUI_ASSERT(is_rt || is_ds && "this method for dsv or rtv resources");

		auto descriptor_allocation = this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav->allocate(this->m_size_srv_cbv_uav_descriptor);

		auto offset_pointer = SIZE_T(INT64(this->m_p_handle->ptr)) + INT64(descriptor_allocation.offset);
		D3D12_CPU_DESCRIPTOR_HANDLE cast_offset_pointer;
		cast_offset_pointer.ptr = offset_pointer;

		this->m_p_device->CreateShaderResourceView(p_resource, &desc_srv, cast_offset_pointer);
		p_texture->Set_Allocation_DescriptorHeap(descriptor_allocation);

		if (is_rt)
			p_impl->Set_DescriptorResourceView(this->Alloc_RenderTargetResourceView(p_resource, p_impl->Get_VirtualAllocation_Descriptor()));
		else if (is_ds)
			p_impl->Set_DescriptorResourceView(this->Alloc_DepthStencilResourceView(p_resource, p_impl->Get_VirtualAllocation_Descriptor()));
	}
}

void RenderInterface_DX12::TextureMemoryManager::Alloc_As_Placed(size_t base_memory, size_t total_memory, D3D12_RESOURCE_DESC& desc,
	TextureHandleType* p_impl, const Rml::byte* p_data)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Alloc_As_Placed");
	RMLUI_ASSERT(base_memory > 0, "must be greater than zero!");
	RMLUI_ASSERT(total_memory > 0, "must be greater than zero!");
	RMLUI_ASSERT(base_memory != size_t(-1), "must be valid number!");
	RMLUI_ASSERT(total_memory != size_t(-1), "must be valid number!");
	RMLUI_ASSERT(this->m_p_device, "must be valid!");
	RMLUI_ASSERT(p_impl, "must be valid!");
	RMLUI_ASSERT(this->m_p_command_allocator, "must be valid!");
	RMLUI_ASSERT(this->m_p_command_list, "must be valid!");

	D3D12_RESOURCE_ALLOCATION_INFO info_for_alloc{};

	if (this->CanBeSmallResource(base_memory))
	{
		desc.Alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
		info_for_alloc = this->m_p_device->GetResourceAllocationInfo(0, 1, &desc);

		RMLUI_ASSERT(info_for_alloc.Alignment == D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT && "wrong calculation! check CanBeSmallResource method!");
		RMLUI_ASSERT(total_memory == info_for_alloc.SizeInBytes, "must be equal! check calculate how you calculate total_memory variable!");
	}
	else
	{
		desc.Alignment = 0;
		info_for_alloc = this->m_p_device->GetResourceAllocationInfo(0, 1, &desc);

		RMLUI_ASSERT(info_for_alloc.Alignment != D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT && "wrong calculation! check CanBeSmallResource method!");
		RMLUI_ASSERT(total_memory == info_for_alloc.SizeInBytes, "must be equal! check calculation how you calculate total_memory variable!");
	}

	int heap_index{-1};
	auto* p_block = this->Get_AvailableBlock(info_for_alloc.SizeInBytes, &heap_index);

	RMLUI_ASSERT(heap_index != -1 && "something is wrong!");
	RMLUI_ASSERT(p_block && "something is wrong!");

	auto* p_heap = this->m_heaps_placed.at(heap_index);
	RMLUI_ASSERT(p_heap && "something is wrong!");

	D3D12_RESOURCE_BARRIER bar;

	D3D12MA::VIRTUAL_ALLOCATION_DESC desc_alloc{};
	desc_alloc.Size = info_for_alloc.SizeInBytes;
	desc_alloc.Alignment = info_for_alloc.Alignment;
	D3D12MA::VirtualAllocation alloc_virtual;
	UINT64 offset{};
	auto status = p_block->Allocate(&desc_alloc, &alloc_virtual, &offset);
	RMLUI_DX_ASSERTMSG(status, "can't allocate virtual alloc!");

	ID3D12Resource* p_resource{};
	status = this->m_p_device->CreatePlacedResource(p_heap, offset, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&p_resource));

	RMLUI_DX_ASSERTMSG(status, "failed to CreatePlacedResource!");
	RMLUI_ASSERT(p_resource && "must be valid pointer of ID3D12Resource*!");

	if (p_impl)
	{
		GraphicsAllocationInfo info_graphics;
		info_graphics.Set_Size(info_for_alloc.SizeInBytes);
		info_graphics.Set_BufferIndex(heap_index);
		info_graphics.Set_Offset(offset);
		info_graphics.Set_VirtualAllocation(alloc_virtual);

		p_impl->Set_Info(info_graphics);
		p_impl->Set_Resource(p_resource);
	}

	bar = CD3DX12_RESOURCE_BARRIER::Aliasing(nullptr, p_resource);

	if (this->m_p_command_allocator)
	{
		auto status = this->m_p_command_allocator->Reset();
		RMLUI_DX_ASSERTMSG(status, "failed to Reset (command allocator)");
	}

	if (this->m_p_command_list)
	{
		auto status = this->m_p_command_list->Reset(this->m_p_command_allocator, nullptr);
		RMLUI_DX_ASSERTMSG(status, "failed to Reset (command list)");

		this->m_p_command_list->ResourceBarrier(1, &bar);
	}

	this->Upload(false, p_impl, desc, p_data, p_resource);
}

void RenderInterface_DX12::TextureMemoryManager::Upload(bool is_committed, TextureHandleType* p_texture_handle, const D3D12_RESOURCE_DESC& desc,
	const Rml::byte* p_data, ID3D12Resource* p_resource)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Upload");
	RMLUI_ASSERT(this->m_p_device && "must be valid!");
	RMLUI_ASSERT(p_data && "must be valid!");
	RMLUI_ASSERT(p_resource && "must be valid!");
	RMLUI_ASSERT(this->m_p_descriptor_heap_srv && "must be valid!");
	RMLUI_ASSERT(this->m_p_allocator && "must be valid!");
	RMLUI_ASSERT(this->m_p_command_list && "must be valid!");
	RMLUI_ASSERT(this->m_p_handle && "must be valid!");
	RMLUI_ASSERT(this->m_p_copy_queue && "must be valid!");
	RMLUI_ASSERT(this->m_p_renderer && "must be valid!");
	RMLUI_ASSERT(this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav && "must be valid!");
	RMLUI_ASSERT(p_texture_handle && "must be valid!");
	RMLUI_ASSERT(this->m_p_fence && "must be valid!");
	RMLUI_ASSERT(this->m_p_fence_event && "must be valid!");

	auto upload_size = GetRequiredIntermediateSize(p_resource, 0, 1);

	D3D12MA::ALLOCATION_DESC desc_alloc{};
	desc_alloc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	D3D12MA::Allocation* p_allocation{};

	ID3D12Resource* p_upload_buffer{};
	auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(upload_size);
	auto status = this->m_p_allocator->CreateResource(&desc_alloc, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &p_allocation,
		IID_PPV_ARGS(&p_upload_buffer));

	RMLUI_DX_ASSERTMSG(status, "failed to CreateResource (upload buffer for texture)");

	D3D12_SUBRESOURCE_DATA desc_data{};
	desc_data.pData = p_data;
	desc_data.RowPitch = desc.Width * this->BytesPerPixel(desc.Format);
	desc_data.SlicePitch = desc_data.RowPitch * desc.Height;

	auto allocated_size = UpdateSubresources<1>(this->m_p_command_list, p_resource, p_upload_buffer, 0, 0, 1, &desc_data);

#ifdef RMLUI_DX_DEBUG
	constexpr const char* p_committed = "committed";
	constexpr const char* p_placed = "placed";

	const char* p_current_resource_type_name{};

	if (is_committed)
		p_current_resource_type_name = p_committed;
	else
		p_current_resource_type_name = p_placed;

	Rml::Log::Message(Rml::Log::Type::LT_DEBUG, "[DirectX 12] allocated %s resource to GPU with size: [%zu]", p_current_resource_type_name,
		allocated_size);
#endif

	D3D12_SHADER_RESOURCE_VIEW_DESC desc_srv{};
	desc_srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc_srv.Format = desc.Format;
	desc_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc_srv.Texture2D.MipLevels = desc.MipLevels;

	auto descriptor_allocation = this->m_p_offset_allocator_for_descriptor_heap_srv_cbv_uav->allocate(this->m_size_srv_cbv_uav_descriptor);

	auto offset_pointer = SIZE_T(INT64(this->m_p_handle->ptr) + INT64(descriptor_allocation.offset));
	D3D12_CPU_DESCRIPTOR_HANDLE cast_offset_pointer;
	cast_offset_pointer.ptr = offset_pointer;

	this->m_p_device->CreateShaderResourceView(p_resource, &desc_srv, cast_offset_pointer);
	p_texture_handle->Set_Allocation_DescriptorHeap(descriptor_allocation);

	status = this->m_p_command_list->Close();

	RMLUI_DX_ASSERTMSG(status, "failed to Close (command list for copy)");

	ID3D12CommandList* p_lists[] = {this->m_p_command_list};

	this->m_p_copy_queue->ExecuteCommandLists(1, p_lists);

	// wait queue for completion
	if (this->m_p_device)
	{
		++this->m_fence_value;

		RMLUI_DX_ASSERTMSG(this->m_p_copy_queue->Signal(this->m_p_fence, this->m_fence_value), "failed to signal copy queue's fence");

		if (this->m_p_fence->GetCompletedValue() < this->m_fence_value)
		{
			RMLUI_DX_ASSERTMSG(this->m_p_fence->SetEventOnCompletion(this->m_fence_value, this->m_p_fence_event), "failed to SetEventOnCompletion");

			WaitForSingleObject(this->m_p_fence_event, INFINITE);
		}
	}

	if (p_allocation)
	{
		if (p_allocation->GetResource())
		{
			p_allocation->GetResource()->Release();
		}

		auto ref_count = p_allocation->Release();
		RMLUI_ASSERT(ref_count == 0, "leak!");
	}
}

size_t RenderInterface_DX12::TextureMemoryManager::BytesPerPixel(DXGI_FORMAT format)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::BytesPerPixel");
	return this->BitsPerPixel(format) / static_cast<size_t>(8);
}

// TODO: need to add xbox's formats????
size_t RenderInterface_DX12::TextureMemoryManager::BitsPerPixel(DXGI_FORMAT format)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::BitsPerPixel");
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT: return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT: return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216: return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2: return 32;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016: return 24;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM: return 16;

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11: return 12;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8: return 8;

	case DXGI_FORMAT_R1_UNORM: return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM: return 4;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB: return 8;

	default: return 0;
	}
}

Rml::Pair<ID3D12Heap*, D3D12MA::VirtualBlock*> RenderInterface_DX12::TextureMemoryManager::Create_HeapPlaced(size_t size_for_creation)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Create_HeapPlaced");
	RMLUI_ASSERT(this->m_p_device, "must be valid!");

	D3D12MA::VIRTUAL_BLOCK_DESC desc_block{};

	desc_block.Size = this->m_size_for_placed_heap;
	D3D12MA::VirtualBlock* p_block{};
	auto status = D3D12MA::CreateVirtualBlock(&desc_block, &p_block);

	RMLUI_DX_ASSERTMSG(status, "failed to D3D12MA::CreateVirtualBlock");

	this->m_blocks.push_back(p_block);

	CD3DX12_HEAP_DESC desc_heap(this->m_size_for_placed_heap, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, 0,
		D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES);

	ID3D12Heap* p_heap{};
	status = this->m_p_device->CreateHeap(&desc_heap, IID_PPV_ARGS(&p_heap));

	RMLUI_DX_ASSERTMSG(status, "failed to CreateHeap!");

	this->m_heaps_placed.push_back(p_heap);

	return {p_heap, p_block};
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderInterface_DX12::TextureMemoryManager::Alloc_DepthStencilResourceView(ID3D12Resource* p_resource,
	D3D12MA::VirtualAllocation* p_alloc)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Alloc_DepthStecilResourceView");
	RMLUI_ASSERT(p_resource && "must be allocated!");
	RMLUI_ASSERT(this->m_p_device && "must be allocated!");
	RMLUI_ASSERT(this->m_p_virtual_block_for_depth_stencil_heap_allocations && "must be initialized");
	RMLUI_ASSERT(p_alloc && "must be valid!");
	RMLUI_ASSERT(this->m_p_descriptor_heap_dsv && "must be valid!");

	D3D12_CPU_DESCRIPTOR_HANDLE calculated_offset{};

	if (p_resource)
	{
		if (this->m_p_virtual_block_for_depth_stencil_heap_allocations)
		{
			if (this->m_p_device)
			{
				if (this->m_p_descriptor_heap_dsv)
				{
					D3D12MA::VIRTUAL_ALLOCATION_DESC desc_alloc = {};
					desc_alloc.Size = this->m_size_dsv_descriptor;
					UINT64 offset{};

					auto status = this->m_p_virtual_block_for_depth_stencil_heap_allocations->Allocate(&desc_alloc, p_alloc, &offset);

					RMLUI_DX_ASSERTMSG(status,
						"failed to allocate descriptor rtv, it means you need to resize and set higher size than previous, overflow (see "
						"RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV constant)");

					D3D12_DEPTH_STENCIL_VIEW_DESC desc_rtv = {};
					desc_rtv.Format = RMLUI_RENDER_BACKEND_FIELD_DEPTHSTENCIL_TEXTURE_FORMAT;
					desc_rtv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

					calculated_offset.ptr = (this->m_p_descriptor_heap_dsv->GetCPUDescriptorHandleForHeapStart().ptr + offset);

					this->m_p_device->CreateDepthStencilView(p_resource, &desc_rtv, calculated_offset);
				}
			}
		}
	}

	return calculated_offset;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderInterface_DX12::TextureMemoryManager::Alloc_RenderTargetResourceView(ID3D12Resource* p_resource,
	D3D12MA::VirtualAllocation* p_alloc)
{
	RMLUI_ZoneScopedN("DirectX 12 - TextureMemoryManager::Alloc_RenderTargetResourceView");
	RMLUI_ASSERT(p_resource && "must be allocated!");
	RMLUI_ASSERT(this->m_p_device && "must be allocated!");
	RMLUI_ASSERT(this->m_p_virtual_block_for_render_target_heap_allocations && "must be initialized");
	RMLUI_ASSERT(p_alloc && "must be valid!");
	RMLUI_ASSERT(this->m_p_descriptor_heap_rtv && "must be valid!");

	D3D12_CPU_DESCRIPTOR_HANDLE calculated_offset{};

	if (p_resource)
	{
		if (this->m_p_virtual_block_for_render_target_heap_allocations)
		{
			if (this->m_p_device)
			{
				if (this->m_p_descriptor_heap_rtv)
				{
					D3D12MA::VIRTUAL_ALLOCATION_DESC desc_alloc = {};
					desc_alloc.Size = this->m_size_rtv_descriptor;
					UINT64 offset{};

					auto status = this->m_p_virtual_block_for_render_target_heap_allocations->Allocate(&desc_alloc, p_alloc, &offset);

					RMLUI_DX_ASSERTMSG(status,
						"failed to allocate descriptor rtv, it means you need to resize and set higher size than previous, overflow (see "
						"RMLUI_RENDER_BACKEND_FIELD_DESCRIPTOR_HEAP_RTV constant)");

					D3D12_RENDER_TARGET_VIEW_DESC desc_rtv = {};
					desc_rtv.Format = RMLUI_RENDER_BACKEND_FIELD_COLOR_TEXTURE_FORMAT;
					desc_rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

					calculated_offset.ptr = (this->m_p_descriptor_heap_rtv->GetCPUDescriptorHandleForHeapStart().ptr + offset);

					this->m_p_device->CreateRenderTargetView(p_resource, &desc_rtv, calculated_offset);
				}
			}
		}
	}

	return calculated_offset;
}

RenderInterface_DX12::ConstantBufferType::ConstantBufferType() : m_is_free{true}, m_p_gpu_start_memory_for_binding_data{} {}

RenderInterface_DX12::ConstantBufferType::~ConstantBufferType() {}

const RenderInterface_DX12::GraphicsAllocationInfo& RenderInterface_DX12::ConstantBufferType::Get_AllocInfo(void) const noexcept
{
	return this->m_alloc_info;
}

void RenderInterface_DX12::ConstantBufferType::Set_AllocInfo(const GraphicsAllocationInfo& info) noexcept
{
	this->m_alloc_info = info;
}

void* RenderInterface_DX12::ConstantBufferType::Get_GPU_StartMemoryForBindingData(void)
{
	return this->m_p_gpu_start_memory_for_binding_data;
}

void RenderInterface_DX12::ConstantBufferType::Set_GPU_StartMemoryForBindingData(void* p_start_pointer)
{
	this->m_p_gpu_start_memory_for_binding_data = p_start_pointer;
}

RenderInterface_DX12::GraphicsAllocationInfo::GraphicsAllocationInfo() : m_buffer_index{-1}, m_offset{}, m_size{}, m_alloc_info{} {}

RenderInterface_DX12::GraphicsAllocationInfo::~GraphicsAllocationInfo() {}

const D3D12MA::VirtualAllocation& RenderInterface_DX12::GraphicsAllocationInfo::Get_VirtualAllocation(void) const noexcept
{
	return this->m_alloc_info;
}

void RenderInterface_DX12::GraphicsAllocationInfo::Set_VirtualAllocation(const D3D12MA::VirtualAllocation& info_alloc) noexcept
{
	this->m_alloc_info = info_alloc;
}

size_t RenderInterface_DX12::GraphicsAllocationInfo::Get_Offset(void) const noexcept
{
	return this->m_offset;
}

void RenderInterface_DX12::GraphicsAllocationInfo::Set_Offset(size_t value) noexcept
{
	this->m_offset = value;
}

size_t RenderInterface_DX12::GraphicsAllocationInfo::Get_Size(void) const noexcept
{
	return this->m_size;
}

void RenderInterface_DX12::GraphicsAllocationInfo::Set_Size(size_t value) noexcept
{
	this->m_size = value;
}

int RenderInterface_DX12::GraphicsAllocationInfo::Get_BufferIndex(void) const noexcept
{
	return this->m_buffer_index;
}

void RenderInterface_DX12::GraphicsAllocationInfo::Set_BufferIndex(int index) noexcept
{
	this->m_buffer_index = index;
}
