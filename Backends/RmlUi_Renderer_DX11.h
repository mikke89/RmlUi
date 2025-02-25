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

#ifndef RMLUI_BACKENDS_RENDERER_DX11_H
#define RMLUI_BACKENDS_RENDERER_DX11_H

#include <RmlUi/Core/Platform.h>

#ifndef RMLUI_PLATFORM_WIN32
	#error "Cannot build DX11 renderer on non-Windows platforms"
#endif // RMLUI_PLATFORM_WIN32

#include <RmlUi/Core/RenderInterface.h>

#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3d11_1.h>

#ifdef RMLUI_DEBUG
    #define RMLUI_DX_ASSERTMSG(statement, msg) RMLUI_ASSERTMSG(SUCCEEDED(statement), msg)

    // Uncomment the following line to enable additional DirectX debugging.
    #define RMLUI_DX_DEBUG
#else
    #define RMLUI_DX_ASSERTMSG(statement, msg) static_cast<void>(statement)
#endif

// Helper for a common cleanup pattern in DX11
#define DX_CLEANUP_RESOURCE_IF_CREATED(resource)    \
        if (resource)                               \
        {                                           \
            resource->Release();                    \
            resource = nullptr;                     \
        }

// Allow the user to override the number of MSAA samples
#ifndef NUM_MSAA_SAMPLES
    #define NUM_MSAA_SAMPLES 2
#endif

// Use these typedefs to overload the default implementation with another image loader so that you can handle more than uncompressed TGA if you wish.
// Must define both the load and free functions. See RenderInterface_DX11::
typedef void (*pfnLoadTextureRaw)(const Rml::String& filename, int* pWidth, int* pHeight, uint8_t** pData, size_t* pDataSize);
typedef void (*pfnFreeTextureRaw)(uint8_t* pData);

enum class ProgramId;
class RenderLayerStack;
namespace Gfx {
struct ProgramData;
struct RenderTargetData;
} // namespace Gfx

class RenderInterface_DX11 : public Rml::RenderInterface {
public:
    RenderInterface_DX11();
    ~RenderInterface_DX11();
    
    // Resource initialisation and cleanup
    void Init(ID3D11Device* p_d3d_device);
    void Cleanup ();

    // Sets up DirectX11 states for taking rendering commands from RmlUi.
    void BeginFrame(ID3D11RenderTargetView* p_render_target_view);
    void EndFrame();

    void SetViewport(const int width, const int height);
    void Clear();

    // -- Inherited from Rml::RenderInterface --

    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override;
    void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture_handle) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;

    void EnableClipMask(bool enable) override;
    void RenderToClipMask(Rml::ClipMaskOperation mask_operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;

    void SetTransform(const Rml::Matrix4f* transform) override;

    Rml::LayerHandle PushLayer() override;
    void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
        Rml::Span<const Rml::CompiledFilterHandle> filters) override;
    void PopLayer() override;

    Rml::TextureHandle SaveLayerAsTexture() override;

    Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

    Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) override;
    void ReleaseFilter(Rml::CompiledFilterHandle filter) override;

    Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override;
    void RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation,
        Rml::TextureHandle texture) override;
    void ReleaseShader(Rml::CompiledShaderHandle effect_handle) override;

    // Can be passed to RenderGeometry() to enable texture rendering without changing the bound texture.
    static constexpr Rml::TextureHandle TextureEnableWithoutBinding = Rml::TextureHandle(-1);
    // Can be passed to RenderGeometry() to leave the bound texture and used program unchanged.
    static constexpr Rml::TextureHandle TexturePostprocess = Rml::TextureHandle(-2);
    // Can be passed to RenderGeometry() to not touch the cbuffers
    static constexpr Rml::TextureHandle TexturePostprocessNoBinding = Rml::TextureHandle(-3);

private:
    // Changes blend state if necessary
    void SetBlendState(ID3D11BlendState* blend_state);
    void DisableBlend();
    void UpdateConstantBuffer();
    void UseProgram(ProgramId program_id);

    // Mirrors the behaviour of glBlitFramebuffer. It supports sampling from a rect from source, writing to a rect at dest, and does MSAA resolve.
    // Coordinates are in pixels.
    void BlitRenderTarget(const Gfx::RenderTargetData& source, const Gfx::RenderTargetData& dest, int srcX0, int srcY0, int srcX1, int srcY1,
        int dstX0, int dstY0, int dstX1, int dstY1);
    void BlitLayerToPostprocessPrimary(Rml::LayerHandle layer_handle);
    void RenderFilters(Rml::Span<const Rml::CompiledFilterHandle> filter_handles);

    void SetScissor(Rml::Rectanglei region, bool vertically_flip = false);

    void DrawFullscreenQuad();
    void DrawFullscreenQuad(Rml::Vector2f uv_offset, Rml::Vector2f uv_scaling = Rml::Vector2f(1.f));

    void RenderBlur(float sigma, const Gfx::RenderTargetData& source_destination, const Gfx::RenderTargetData& temp, Rml::Rectanglei window_flipped);

private:
    // D3D11 core resources
    ID3D11Device* m_d3d_device = nullptr;
    ID3D11Device1* m_d3d_device_1 = nullptr;
    ID3D11DeviceContext1* m_d3d_context = nullptr;
    ID3D11RenderTargetView* m_bound_render_target = nullptr;
    bool m_scissor_enabled = false;
    ID3D11RasterizerState* m_rasterizer_state_scissor_enabled = nullptr;
    ID3D11RasterizerState* m_rasterizer_state_scissor_disabled = nullptr;

    // Depth stencil states
    bool m_is_stencil_enabled = false;
    bool m_stencil_dirty = false;
    uint32_t m_stencil_ref_value = 0;
    ID3D11DepthStencilState* m_depth_stencil_state_disable = nullptr; // Stencil Off
    ID3D11DepthStencilState* m_depth_stencil_state_stencil_set = nullptr; // Clipmask Set/Inverse
    ID3D11DepthStencilState* m_depth_stencil_state_stencil_intersect = nullptr; // Clipmask Intersect. Can have several test values
    ID3D11DepthStencilState* m_depth_stencil_state_stencil_test = nullptr; // Clipmask test

    // Shaders
    ID3D11Buffer* m_shader_buffer = nullptr;
    ID3D11Buffer* m_color_matrix_cbuffer = nullptr;
    ID3D11Buffer* m_blur_cbuffer = nullptr;
    ID3D11Buffer* m_drop_shadow_cbuffer = nullptr;
    bool m_cbuffer_dirty = true;
    ID3D11SamplerState* m_sampler_state = nullptr;

    // Viewport dimensions
    int m_viewport_width = 0;
    int m_viewport_height = 0;

    Rml::Matrix4f m_transform = Rml::Matrix4f::Identity();
    Rml::Matrix4f m_projection = Rml::Matrix4f::Identity();
    Rml::Vector2f m_translation = {};

    ID3D11InputLayout* m_vertex_layout = nullptr;
    ID3D11BlendState* m_blend_state_enable = nullptr;
    ID3D11BlendState* m_blend_state_disable_color = nullptr;
    ID3D11BlendState* m_blend_state_color_filter = nullptr;

    ProgramId active_program = {};
    Rml::Rectanglei m_scissor_state = Rml::Rectanglei::MakeInvalid();
    Rml::UniquePtr<const Gfx::ProgramData> program_data;

    struct DX11_GeometryData {
    public:
        ID3D11Buffer* vertex_buffer = nullptr;
        ID3D11Buffer* index_buffer = nullptr;
        size_t index_count = 0;
    };

    // D3D11 state for RmlUi rendering
    bool m_current_blend_state_enabled = false;
    ID3D11BlendState* m_current_blend_state = nullptr;
    
    // Backup of prior D3D11 state
    struct D3D11State {
        UINT scissor_rects_count, viewports_count;
        D3D11_RECT scissor_rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ID3D11RasterizerState* rastizer_state;
        ID3D11BlendState* blend_state;
        FLOAT blend_factor[4];
        UINT sample_mask;
        UINT stencil_ref;
        ID3D11DepthStencilState* depth_stencil_state;
        ID3D11ShaderResourceView* pixel_shader_shader_resource;
        ID3D11SamplerState* pixel_shader_sampler;
        ID3D11PixelShader* pixel_shader;
        ID3D11VertexShader* vertex_shader;
        ID3D11GeometryShader* geometry_shader;
        UINT pixel_shader_instances_count, vertex_shader_instances_count, geometry_shader_instances_count;
        // 256 is max according to PSSetShader documentation
        ID3D11ClassInstance *pixel_shader_instances[256], *vertex_shader_instances[256], *geometry_shader_instances[256];
        D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
        ID3D11Buffer *index_buffer, *vertex_buffer, *vertex_shader_constant_buffer;
        UINT index_buffer_offset, vertex_buffer_stride, vertex_buffer_offset;
        DXGI_FORMAT index_buffer_format;
        ID3D11InputLayout* input_layout;
    };

    D3D11State m_previous_d3d_state{};

    #pragma pack(4)
    union ShaderCbuffer {
        struct {
            Rml::Matrix4f transform;
            Rml::Vector2f translation;
        } common;
        struct Gradient {
            int _padding[18];
            int func;
            int num_stops;
            Rml::Vector2f p;
            Rml::Vector2f v;
            Rml::Vector4f stop_colors[16];
            float stop_positions[16];
        } gradient;
        struct Blur {
            int _padding[18];
            Rml::Vector2f texel_offset;
            Rml::Vector4f weights;
            Rml::Vector2f texcoord_min;
            Rml::Vector2f texcoord_max;
        } blur;
        struct DropShadow {
            Rml::Vector2f texcoord_min;
            Rml::Vector2f texcoord_max;
            Rml::Vector4f color;
        } drop_shadow;
        struct Creation {
            int _padding[18];
            Rml::Vector2f dimensions;
            float value;
        } creation;
    };
    struct ColorMatrixCbuffer {
        Rml::Matrix4f color_matrix;
    };
    #pragma pack()

    Rml::CompiledGeometryHandle m_fullscreen_quad_geometry = 0;

    /*
        Manages render targets, including the layer stack and postprocessing render targets.

        Layers can be pushed and popped, creating new render targets as needed. Typically, geometry is rendered to the top
        layer. The layer render targets may have MSAA enabled.

        Postprocessing render targets are separate from the layers, and are commonly used to apply texture-wide effects
        such as filters. They are used both as input and output during rendering, and do not use MSAA.
    */
    class RenderLayerStack {
    public:
        RenderLayerStack();
        ~RenderLayerStack();

        void SetD3DResources(ID3D11Device* device);

        // Push a new layer. All references to previously retrieved layers are invalidated.
        Rml::LayerHandle PushLayer();

        // Pop the top layer. All references to previously retrieved layers are invalidated.
        void PopLayer();

        const Gfx::RenderTargetData& GetLayer(Rml::LayerHandle layer) const;
        const Gfx::RenderTargetData& GetTopLayer() const;
        Rml::LayerHandle GetTopLayerHandle() const;

        const Gfx::RenderTargetData& GetPostprocessPrimary() { return EnsureRenderTargetPostprocess(0); }
        const Gfx::RenderTargetData& GetPostprocessSecondary() { return EnsureRenderTargetPostprocess(1); }
        const Gfx::RenderTargetData& GetPostprocessTertiary() { return EnsureRenderTargetPostprocess(2); }
        const Gfx::RenderTargetData& GetBlendMask() { return EnsureRenderTargetPostprocess(3); }
        const Gfx::RenderTargetData& GetTemporary() { return EnsureRenderTargetPostprocess(4); }

        void SwapPostprocessPrimarySecondary();

        void BeginFrame(int new_width, int new_height);
        void EndFrame();

    private:
        void DestroyRenderTargets();
        const Gfx::RenderTargetData& EnsureRenderTargetPostprocess(int index);

        int m_width = 0, m_height = 0;

        // The number of active layers is manually tracked since we re-use the render targets stored in the fb_layers stack.
        int m_layers_size = 0;

        Rml::Vector<Gfx::RenderTargetData> m_rt_layers;
        Rml::Vector<Gfx::RenderTargetData> m_rt_postprocess;
        
        ID3D11Device* m_d3d_device = nullptr;
    };

    RenderLayerStack m_render_layers;
};

#endif // RMLUI_BACKENDS_RENDERER_DX11_H
