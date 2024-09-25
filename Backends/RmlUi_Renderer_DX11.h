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

    #if defined RMLUI_PLATFORM_WIN32
        #include <RmlUi/Core/RenderInterface.h>

        #include <d3d11.h>
        #include <dxgi1_3.h>

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
#ifndef MSAA_SAMPLES
#define MSAA_SAMPLES 2
#endif

// Use these typedefs to overload the default implementation with another image loader so that you can handle more than uncompressed TGA if you wish.
// Must define both the load and free functions. See RenderInterface_DX11::
typedef void (*pfnLoadTextureRaw)(const Rml::String& filename, int* pWidth, int* pHeight, uint8_t** pData, size_t* pDataSize);
typedef void (*pfnFreeTextureRaw)(uint8_t* pData);

class RenderInterface_DX11 : public Rml::RenderInterface {
public:
    RenderInterface_DX11();
    
    // Resource initialisation and cleanup
    void Init(ID3D11Device* p_d3d_device, ID3D11DeviceContext* p_d3d_device_context);
    void Cleanup ();

    // Sets up DirectX11 states for taking rendering commands from RmlUi.
    void BeginFrame(IDXGISwapChain* p_swapchain, ID3D11RenderTargetView* p_render_target_view);
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
    
    void SetTransform(const Rml::Matrix4f* transform) override;

public:
    /// Called by the renderer when it wants to load a texture from disk.
    /// @param[in] pFilename A Rml string containing the file name.
    /// @param[out] pWidth The width of the texture read from disk.
    /// @param[out] pHeight The height of the texture read from disk.
    /// @param[out] pData A pointer to an RGBA byte array of texture data, which will then be used to generate a texture. Set to NULL to indicate that the file failed to load for whatever reason.
    /// @param[out] pDataSize A pointer to a size_t storing how many bytes is in pData.
    pfnLoadTextureRaw LoadTextureFromFileRaw = nullptr;
    /// Called by the renderer when it wants to free a texture from disk. Always called after a successful load from LoadTextureFromFileRaw.
    /// @param[in] pData A pointer to an RGBA byte array of texture data to free.
    pfnFreeTextureRaw FreeTextureFromFileRaw = nullptr;

private:
    // Changes blend state if necessary
    void SetBlendState(ID3D11BlendState* blendState);
    void UpdateConstantBuffer();

private:
    UINT m_default_shader_flags;

    // D3D11 core resources
    ID3D11Device* m_d3d_device = nullptr;
    ID3D11DeviceContext* m_d3d_context = nullptr;
    IDXGISwapChain* m_bound_swapchain = nullptr;
    ID3D11RenderTargetView* m_bound_render_target = nullptr;
    ID3D11RasterizerState* m_rasterizer_state_scissor_enabled = nullptr;
    ID3D11RasterizerState* m_rasterizer_state_scissor_disabled = nullptr;
    ID3D11DepthStencilState* m_depth_stencil_state = nullptr;

    // Shaders
    ID3D11Buffer* m_shader_buffer = nullptr;
    bool m_cbuffer_dirty = true;
    ID3D11SamplerState* m_samplerState = nullptr;
    ID3D11VertexShader* m_shader_vertex_common = nullptr;
    ID3D11PixelShader* m_shader_pixel_color = nullptr;
    ID3D11PixelShader* m_shader_pixel_texture = nullptr;

    // Viewport dimensions
    int m_viewport_width = 0;
    int m_viewport_height = 0;

    Rml::Matrix4f m_transform = Rml::Matrix4f::Identity();
    Rml::Matrix4f m_projection = Rml::Matrix4f::Identity();
    Rml::Vector2f m_translation = {};

    ID3D11InputLayout* m_vertex_layout = nullptr;
    ID3D11BlendState* m_blend_state = nullptr;
    D3D11_RECT m_rect_scissor = {};
    bool m_scissor_region_enabled = false;

    struct DX11_GeometryData {
    public:
        ID3D11Buffer* vertex_buffer = nullptr;
        ID3D11Buffer* index_buffer = nullptr;
        uint32_t index_count = 0;
    };

    std::unordered_map<uintptr_t, DX11_GeometryData> m_geometry_cache;

    // D3D11 state
    ID3D11BlendState* m_current_blend_state = nullptr;
    
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
    struct ShaderCbuffer {
        Rml::Matrix4f transform;
        Rml::Vector2f translation;
        float _padding[2];
    };
    #pragma pack()
};

#endif // RMLUI_PLATFORM_WIN32

#endif // RMLUI_BACKENDS_RENDERER_DX11_H
