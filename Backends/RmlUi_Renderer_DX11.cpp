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

#include <RmlUi/Core/Platform.h>

#if defined RMLUI_PLATFORM_WIN32

    #include "RmlUi_Renderer_DX11.h"
    #include <RmlUi/Core/Core.h>
    #include <RmlUi/Core/FileInterface.h>
    #include <RmlUi/Core/Log.h>

    #include "RmlUi_Include_Windows.h"

RenderInterface_DX11::RenderInterface_DX11() {}

{
void RenderInterface_DX11::Init(ID3D11Device* p_d3d_device, ID3D11DeviceContext* p_d3d_device_context)
{
    RMLUI_ASSERTMSG(p_d3d_device, "p_d3d_device cannot be nullptr!");
    RMLUI_ASSERTMSG(p_d3d_device_context, "p_d3d_device_context cannot be nullptr!");

    // Assign D3D resources
    m_d3d_device = p_d3d_device;
    m_d3d_context = p_d3d_device_context;

    // RmlUi serves vertex colors and textures with premultiplied alpha, set the blend mode accordingly.
    // Equivalent to glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
    if (!m_blend_state)
    {
        D3D11_BLEND_DESC blendDesc;
        ZeroMemory(&blendDesc, sizeof(blendDesc));
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HRESULT result = m_d3d_device->CreateBlendState(&blendDesc, &m_blend_state);
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::LT_ERROR, "ID3D11Device::CreateBlendState (%d)", result);
            return;
        }
    }

    // Scissor regions require a rasterizer state. Cache one for scissor on and off
    {
        D3D11_RASTERIZER_DESC rasterizerDesc;
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_BACK;
        rasterizerDesc.FrontCounterClockwise = false;
        rasterizerDesc.DepthBias = 0;
        rasterizerDesc.SlopeScaledDepthBias = 0.0f;
        rasterizerDesc.DepthBiasClamp = 0.0f;
        rasterizerDesc.DepthClipEnable = true;
        rasterizerDesc.ScissorEnable = true;
        rasterizerDesc.MultisampleEnable = MSAA_SAMPLES > 1;
        rasterizerDesc.AntialiasedLineEnable = MSAA_SAMPLES > 1;

        HRESULT result = m_d3d_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizer_state_scissor_enabled);
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::LT_ERROR, "ID3D11Device::CreateRasterizerState (scissor: enabled) (%d)", result);
            return;
        }

        rasterizerDesc.ScissorEnable = false;

        result = m_d3d_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizer_state_scissor_disabled);
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::LT_ERROR, "ID3D11Device::CreateRasterizerState (scissor: disabled) (%d)", result);
            return;
        }
    }

    // Load shaders

    // @TODO:

    // Create vertex layout. This will be constant to avoid copying to an intermediate struct.

    // @TODO:

    // Create constant buffers. This is so that we can bind uniforms such as translation and color to the shaders.

    // @TODO:
}

void RenderInterface_DX11::Cleanup() {
    m_blendState->Release();
}

void RenderInterface_DX11::BeginFrame(IDXGISwapChain* p_swapchain, ID3D11RenderTargetView* p_render_target_view)
{
    RMLUI_ASSERTMSG(p_swapchain, "p_swapchain cannot be nullptr!");
    RMLUI_ASSERTMSG(p_render_target_view, "p_render_target_view cannot be nullptr!");
    RMLUI_ASSERTMSG(m_d3d_context, "d3d_context cannot be nullptr!");
    RMLUI_ASSERTMSG(m_d3d_device, "d3d_device cannot be nullptr!");

    m_bound_render_target = p_render_target_view;
    m_bound_swapchain = p_swapchain;

    D3D11_VIEWPORT d3dviewport;
    d3dviewport.TopLeftX = 0;
    d3dviewport.TopLeftY = 0;
    d3dviewport.Width = m_width;
    d3dviewport.Height = m_height;
    d3dviewport.MinDepth = 0.0f;
    d3dviewport.MaxDepth = 1.0f;
    m_d3d_context->RSSetViewports(1, &d3dviewport);
    Clear();
    SetBlendState(m_blend_state);
}

void RenderInterface_DX11::EndFrame() {
    // @TODO: Compositing
    // @TODO: Layer stack
    m_bound_render_target = nullptr;
}


Rml::CompiledGeometryHandle RenderInterface_DX11::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    return Rml::CompiledGeometryHandle(0);
}

void RenderInterface_DX11::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {

}

void RenderInterface_DX11::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) {

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


Rml::TextureHandle RenderInterface_DX11::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
    // Use the user provided image loading function if it's provided, else fallback to the included TGA one
    if (LoadTextureFromFileRaw != nullptr && FreeTextureFromFileRaw != nullptr)
    {
        int texture_width = 0, texture_height = 0;
        size_t image_size_bytes = 0;
        uint8_t* texture_data = nullptr;
        LoadTextureFromFileRaw(source, &texture_width, &texture_height, &texture_data, &image_size_bytes);

        if (texture_data != nullptr)
        {
            texture_dimensions.x = texture_width;
            texture_dimensions.y = texture_height;

            Rml::TextureHandle handle = GenerateTexture({texture_data, image_size_bytes}, texture_dimensions);

            FreeTextureFromFileRaw(texture_data);
            return handle;
        }

        // Image must be invalid if the file failed to load. Return invalid handle
        return Rml::TextureHandle(0);
    }

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

Rml::TextureHandle RenderInterface_DX11::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
    return Rml::TextureHandle(0);
}

void RenderInterface_DX11::ReleaseTexture(Rml::TextureHandle texture_handle)
{

}

void RenderInterface_DX11::EnableScissorRegion(bool enable) {

    if (enable != m_scissor_region_enabled)
    {
        if (enable)
        {
            m_d3d_context->RSSetState(m_rasterizer_state_scissor_enabled);
        }
        else
        {
            m_d3d_context->RSSetState(m_rasterizer_state_scissor_disabled);
        }
        m_scissor_region_enabled = enable;
    }
}

void RenderInterface_DX11::SetScissorRegion(Rml::Rectanglei region)
{
    m_rect_scissor.left		= region.Left();
    m_rect_scissor.top		= region.Top();
    m_rect_scissor.bottom	= region.Bottom();
    m_rect_scissor.right	= region.Right();

    if (m_scissor_region_enabled)
    {
        m_d3d_context->RSSetScissorRects(1, &m_rect_scissor);
    }
}

void RenderInterface_DX11::SetViewport(const int width, const int height)
{
    m_width = width;
    m_height = height;
}

void RenderInterface_DX11::Clear()
{
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_d3d_context->ClearRenderTargetView(m_bound_render_target, clearColor);
}

void RenderInterface_DX11::SetBlendState(ID3D11BlendState* blendState) {
    if (blendState != m_current_blend_state)
    {
        m_d3d_context->OMSetBlendState(blendState, 0, 0xFFFFFFFF);
        m_current_blend_state = blendState;
    }
}

#endif // RMLUI_PLATFORM_WIN32