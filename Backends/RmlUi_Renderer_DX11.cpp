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

    #include "d3dcompiler.h"

    #ifdef RMLUI_DX_DEBUG
        #include <dxgidebug.h>
    #endif

// Shader source code
constexpr const char pShaderSourceText_Color[] = R"(
struct sInputData
{
    float4 inputPos : SV_Position;
    float4 inputColor : COLOR;
    float2 inputUV : TEXCOORD;
};

float4 main(const sInputData inputArgs) : SV_TARGET 
{ 
    return inputArgs.inputColor; 
}
)";
constexpr const char pShaderSourceText_Vertex[] = R"(
struct sInputData 
{
    float2 inPosition : POSITION;
    float4 inColor : COLOR;
    float2 inTexCoord : TEXCOORD;
};

struct sOutputData
{
    float4 outPosition : SV_Position;
    float4 outColor : COLOR;
    float2 outUV : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 m_transform;
    float2 m_translate;
    float2 _padding;
};

sOutputData main(const sInputData inArgs)
{
    sOutputData result;

    float2 translatedPos = inArgs.inPosition + m_translate;
    float4 resPos = mul(m_transform, float4(translatedPos.x, translatedPos.y, 0.0, 1.0));

    result.outPosition = resPos;
    result.outColor = inArgs.inColor;
    result.outUV = inArgs.inTexCoord;

#if defined(RMLUI_PREMULTIPLIED_ALPHA)
    // Pre-multiply vertex colors with their alpha.
    result.outColor.rgb = result.outColor.rgb * result.outColor.a;
#endif

    return result;
};
)";

constexpr const char pShaderSourceText_Texture[] = R"(
struct sInputData
{
    float4 inputPos : SV_Position;
    float4 inputColor : COLOR;
    float2 inputUV : TEXCOORD;
};

Texture2D g_InputTexture : register(t0);
SamplerState g_SamplerLinear : register(s0);


float4 main(const sInputData inputArgs) : SV_TARGET 
{
    return inputArgs.inputColor * g_InputTexture.Sample(g_SamplerLinear, inputArgs.inputUV); 
}
)";

RenderInterface_DX11::RenderInterface_DX11() {
    #ifdef RMLUI_DX_DEBUG
    m_default_shader_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
    m_default_shader_flags = 0;
    #endif
}

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
        D3D11_BLEND_DESC blendDesc{};
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
        RMLUI_DX_ASSERTMSG(result, "failed to CreateBlendState");
    #ifdef RMLUI_DX_DEBUG
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::LT_ERROR, "ID3D11Device::CreateBlendState (%d)", result);
            return;
        }
    #endif
    }

    // Scissor regions require a rasterizer state. Cache one for scissor on and off
    {
        D3D11_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.FrontCounterClockwise = false;
        rasterizerDesc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
        rasterizerDesc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.DepthClipEnable = false;
        rasterizerDesc.ScissorEnable = true;
        rasterizerDesc.MultisampleEnable = MSAA_SAMPLES > 1;
        rasterizerDesc.AntialiasedLineEnable = MSAA_SAMPLES > 1;

        HRESULT result = m_d3d_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizer_state_scissor_enabled);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateRasterizerState");
    #ifdef RMLUI_DX_DEBUG
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::LT_ERROR, "ID3D11Device::CreateRasterizerState (scissor: enabled) (%d)", result);
            return;
        }
    #endif

        rasterizerDesc.ScissorEnable = false;

        result = m_d3d_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizer_state_scissor_disabled);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateRasterizerState");
    #ifdef RMLUI_DX_DEBUG
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::LT_ERROR, "ID3D11Device::CreateRasterizerState (scissor: disabled) (%d)", result);
            return;
        }
    #endif
    }

    // Compile and load shaders
    
    // Buffer shall be cleared up later, as it's required to create the input layout
    ID3DBlob* p_shader_vertex_common{};

    {
        const D3D_SHADER_MACRO macros[] = {"RMLUI_PREMULTIPLIED_ALPHA", nullptr, nullptr, nullptr};
        ID3DBlob* p_error_buff{};

        // Common vertex shader
        HRESULT result = D3DCompile(pShaderSourceText_Vertex, sizeof(pShaderSourceText_Vertex), nullptr, macros, nullptr, "main", "vs_5_0",
            this->m_default_shader_flags, 0, &p_shader_vertex_common, &p_error_buff);
        RMLUI_DX_ASSERTMSG(result, "failed to D3DCompile");
    #ifdef RMLUI_DX_DEBUG
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
        }
    #endif

        DX_CLEANUP_RESOURCE_IF_CREATED(p_error_buff);

        // Color fragment shader
        ID3DBlob* p_shader_color_pixel{};

        result = D3DCompile(pShaderSourceText_Color, sizeof(pShaderSourceText_Color), nullptr, macros, nullptr, "main", "ps_5_0",
            this->m_default_shader_flags, 0, &p_shader_color_pixel, &p_error_buff);
        RMLUI_DX_ASSERTMSG(result, "failed to D3DCompile");
    #ifdef RMLUI_DX_DEBUG
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
        }
    #endif

        DX_CLEANUP_RESOURCE_IF_CREATED(p_error_buff);

        // Texture fragment shader
        ID3DBlob* p_shader_color_texture{};

        result = D3DCompile(pShaderSourceText_Texture, sizeof(pShaderSourceText_Texture), nullptr, macros, nullptr, "main", "ps_5_0",
            this->m_default_shader_flags, 0, &p_shader_color_texture, &p_error_buff);
        RMLUI_DX_ASSERTMSG(result, "failed to D3DCompile");
    #ifdef RMLUI_DX_DEBUG
        if (FAILED(result))
        {
            Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to compile shader: %s", (char*)p_error_buff->GetBufferPointer());
        }
    #endif

        DX_CLEANUP_RESOURCE_IF_CREATED(p_error_buff);

        // Create the shader objects
        result = m_d3d_device->CreateVertexShader(p_shader_vertex_common->GetBufferPointer(), p_shader_vertex_common->GetBufferSize(), nullptr,
            &m_shader_vertex_common);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateVertexShader");
        if (FAILED(result))
        {
    #ifdef RMLUI_DX_DEBUG
            Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to create vertex shader: %d", result);
    #endif
            DX_CLEANUP_RESOURCE_IF_CREATED(p_shader_color_pixel);
            DX_CLEANUP_RESOURCE_IF_CREATED(p_shader_color_texture);
            goto cleanup;
            return;
        }

        result = m_d3d_device->CreatePixelShader(p_shader_color_pixel->GetBufferPointer(), p_shader_color_pixel->GetBufferSize(), nullptr,
            &m_shader_pixel_color);
        RMLUI_DX_ASSERTMSG(result, "failed to CreatePixelShader");
        if (FAILED(result))
        {
    #ifdef RMLUI_DX_DEBUG
            Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to create pixel shader: %d", result);
    #endif
            DX_CLEANUP_RESOURCE_IF_CREATED(p_shader_color_pixel);
            DX_CLEANUP_RESOURCE_IF_CREATED(p_shader_color_texture);
            goto cleanup;
            return;
        }

        result = m_d3d_device->CreatePixelShader(p_shader_color_texture->GetBufferPointer(), p_shader_color_texture->GetBufferSize(), nullptr,
            &m_shader_pixel_texture);
        RMLUI_DX_ASSERTMSG(result, "failed to CreatePixelShader");
        if (FAILED(result))
        {
    #ifdef RMLUI_DX_DEBUG
            Rml::Log::Message(Rml::Log::Type::LT_ERROR, "failed to create pixel shader: %d", result);
    #endif
            DX_CLEANUP_RESOURCE_IF_CREATED(p_shader_color_pixel);
            DX_CLEANUP_RESOURCE_IF_CREATED(p_shader_color_texture);
            goto cleanup;
            return;
        }
    }

    // Create vertex layout. This will be constant to avoid copying to an intermediate struct.

    {
        D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        uint32_t numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

        HRESULT result = m_d3d_device->CreateInputLayout(polygonLayout, numElements, p_shader_vertex_common->GetBufferPointer(),
            p_shader_vertex_common->GetBufferSize(), &m_vertex_layout);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateInputLayout");
        if (FAILED(result))
        {
            goto cleanup;
            return;
        }
    }

    // Create constant buffers. This is so that we can bind uniforms such as translation and color to the shaders.

    {
        D3D11_BUFFER_DESC cbufferDesc{};
        cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbufferDesc.ByteWidth = sizeof(ShaderCbuffer);
        cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbufferDesc.MiscFlags = 0;
        cbufferDesc.StructureByteStride = 0;

        HRESULT result = m_d3d_device->CreateBuffer(&cbufferDesc, nullptr, &m_shader_buffer);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateBuffer");
        if (FAILED(result))
        {
            goto cleanup;
            return;
        }
    }

    // Create sampler state for textures
    {
        // Create a texture sampler state description.
        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.BorderColor[0] = 0;
        samplerDesc.BorderColor[1] = 0;
        samplerDesc.BorderColor[2] = 0;
        samplerDesc.BorderColor[3] = 0;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        // Create the texture sampler state.
        HRESULT result = m_d3d_device->CreateSamplerState(&samplerDesc, &m_samplerState);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateSamplerState");
        if (FAILED(result))
        {
            goto cleanup;
            return;
        }

    }

cleanup:

    // Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
    DX_CLEANUP_RESOURCE_IF_CREATED(p_shader_vertex_common);
}

void RenderInterface_DX11::Cleanup() {
    // Cleans up all resources
    DX_CLEANUP_RESOURCE_IF_CREATED(m_rasterizer_state_scissor_disabled);
    DX_CLEANUP_RESOURCE_IF_CREATED(m_rasterizer_state_scissor_enabled);
    DX_CLEANUP_RESOURCE_IF_CREATED(m_blend_state);
    DX_CLEANUP_RESOURCE_IF_CREATED(m_shader_vertex_common);
    DX_CLEANUP_RESOURCE_IF_CREATED(m_shader_pixel_color);
    DX_CLEANUP_RESOURCE_IF_CREATED(m_shader_pixel_texture);
    DX_CLEANUP_RESOURCE_IF_CREATED(m_shader_buffer);
    DX_CLEANUP_RESOURCE_IF_CREATED(m_samplerState);
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
    d3dviewport.Width = m_viewport_width;
    d3dviewport.Height = m_viewport_height;
    d3dviewport.MinDepth = 0.0f;
    d3dviewport.MaxDepth = 1.0f;
    m_d3d_context->RSSetViewports(1, &d3dviewport);
    m_d3d_context->RSSetState(m_rasterizer_state_scissor_disabled); // Disable scissor
    Clear();
    SetBlendState(m_blend_state);
    m_d3d_context->OMSetRenderTargets(1, &p_render_target_view, nullptr);
    SetTransform(nullptr); // Set no transform on new frame
}

void RenderInterface_DX11::EndFrame() {
    // @TODO: Compositing
    // @TODO: Layer stack
    m_bound_swapchain = nullptr;
    m_bound_render_target = nullptr;
    // Reset blend state
    m_current_blend_state = nullptr;
}


Rml::CompiledGeometryHandle RenderInterface_DX11::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    
    ID3D11Buffer* vertex_buffer = nullptr;
    ID3D11Buffer* index_buffer = nullptr;

    // Bind vertex buffer
    {
        D3D11_BUFFER_DESC vertexBufferDesc{};
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(Rml::Vertex) * vertices.size();
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;
        vertexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vertexData{};
        vertexData.pSysMem = vertices.data();
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        HRESULT result = m_d3d_device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertex_buffer);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateBuffer");
        if (FAILED(result))
        {
            return Rml::CompiledGeometryHandle(0);
        }
    }

    // Bind index buffer
    {

        D3D11_BUFFER_DESC indexBufferDesc{};
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(int) * indices.size();
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;
        indexBufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA indexData{};
        indexData.pSysMem = indices.data();
        indexData.SysMemPitch = 0;
        indexData.SysMemSlicePitch = 0;

        HRESULT result = m_d3d_device->CreateBuffer(&indexBufferDesc, &indexData, &index_buffer);
        RMLUI_DX_ASSERTMSG(result, "failed to CreateBuffer");
        if (FAILED(result))
        {
            DX_CLEANUP_RESOURCE_IF_CREATED(vertex_buffer);
            return Rml::CompiledGeometryHandle(0);
        }
    }
    
    DX11_GeometryData* geometryData = new DX11_GeometryData;

    geometryData->vertex_buffer = vertex_buffer;
    geometryData->index_buffer = index_buffer;
    geometryData->index_count = indices.size();

    return Rml::CompiledGeometryHandle(geometryData);
}

void RenderInterface_DX11::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
    DX11_GeometryData* geometryData = (DX11_GeometryData*)geometry;
    
    DX_CLEANUP_RESOURCE_IF_CREATED(geometryData->vertex_buffer);
    DX_CLEANUP_RESOURCE_IF_CREATED(geometryData->index_buffer);

    delete reinterpret_cast<DX11_GeometryData*>(geometry);
}

void RenderInterface_DX11::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture)
{
    DX11_GeometryData* geometryData = (DX11_GeometryData*)handle;

    if (m_translation != translation)
    {
        m_translation = translation;
        m_cbuffer_dirty = true;
    }

    if (texture)
    {
        // Texture available
        DX11_TextureData* texData = (DX11_TextureData*)texture;
        m_d3d_context->VSSetShader(this->m_shader_vertex_common, nullptr, 0);
        m_d3d_context->PSSetShader(this->m_shader_pixel_texture, nullptr, 0);
        m_d3d_context->PSSetShaderResources(0, 1, &texData->texture_view);
        m_d3d_context->PSSetSamplers(0, 1, &m_samplerState);
    }
    else
    {
        // No texture, use color
        m_d3d_context->VSSetShader(this->m_shader_vertex_common, nullptr, 0);
        m_d3d_context->PSSetShader(this->m_shader_pixel_color, nullptr, 0);
    }

    UpdateConstantBuffer();

    m_d3d_context->IASetInputLayout(m_vertex_layout);
    m_d3d_context->VSSetConstantBuffers(0, 1, &m_shader_buffer);

    // Bind vertex and index buffers, issue draw call
    uint32_t stride = sizeof(Rml::Vertex);
    uint32_t offset = 0;
    m_d3d_context->IASetVertexBuffers(0, 1, &geometryData->vertex_buffer, &stride, &offset);
    m_d3d_context->IASetIndexBuffer(geometryData->index_buffer, DXGI_FORMAT_R32_UINT, 0);
    m_d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_d3d_context->DrawIndexed(geometryData->index_count, 0, 0);
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

        // Image must be invalid if the file failed to load. Fallback to the default loader.
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
    ID3D11Texture2D* gpu_texture = nullptr;
    ID3D11ShaderResourceView* gpu_texture_view = nullptr;

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = source_dimensions.x;
    textureDesc.Height = source_dimensions.y;
    textureDesc.MipLevels = 0;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HRESULT result = m_d3d_device->CreateTexture2D(&textureDesc, nullptr, &gpu_texture);
    RMLUI_DX_ASSERTMSG(result, "failed to CreateTexture2D");
    if (FAILED(result))
    {
        return Rml::TextureHandle(0);
    }

    // Set the row pitch of the raw image data
    uint32_t rowPitch = (source_dimensions.x * 4) * sizeof(unsigned char);

    // Copy the raw image data into the texture
    m_d3d_context->UpdateSubresource(gpu_texture, 0, nullptr, source.data(), rowPitch, 0);

    // Setup the shader resource view description.
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    // Create the shader resource view for the texture.
    result = m_d3d_device->CreateShaderResourceView(gpu_texture, &srvDesc, &gpu_texture_view);
    RMLUI_DX_ASSERTMSG(result, "failed to CreateShaderResourceView");
    if (FAILED(result))
    {
        DX_CLEANUP_RESOURCE_IF_CREATED(gpu_texture);
        return Rml::TextureHandle(0);
    }

    DX11_TextureData* texData = new DX11_TextureData;
    texData->texture = gpu_texture;
    texData->texture_view = gpu_texture_view;

    // Generate mipmaps for this texture.
    m_d3d_context->GenerateMips(texData->texture_view);

    return Rml::TextureHandle(texData);
}

void RenderInterface_DX11::ReleaseTexture(Rml::TextureHandle texture_handle)
{
    DX11_TextureData* texData = (DX11_TextureData*)texture_handle;
    DX_CLEANUP_RESOURCE_IF_CREATED(texData->texture_view);
    DX_CLEANUP_RESOURCE_IF_CREATED(texData->texture);

    delete reinterpret_cast<DX11_TextureData*>(texData);
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
    m_viewport_width = width;
    m_viewport_height = height;
    m_projection = Rml::Matrix4f::ProjectOrtho(0, (float)m_viewport_width, (float)m_viewport_height, 0, -10000, 10000);
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

void RenderInterface_DX11::SetTransform(const Rml::Matrix4f* new_transform)
{
    m_transform = (new_transform ? (m_projection * (*new_transform)) : m_projection);
    m_cbuffer_dirty = true;
}

void RenderInterface_DX11::UpdateConstantBuffer() {

    if (m_cbuffer_dirty)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource{};
        // Lock the constant buffer so it can be written to.
        HRESULT result = m_d3d_context->Map(m_shader_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result))
        {
            return;
        }

        // Get a pointer to the data in the constant buffer.
        ShaderCbuffer* dataPtr = (ShaderCbuffer*)mappedResource.pData;

        // Copy the data to the GPU
        dataPtr->transform = m_transform;
        dataPtr->translation = m_translation;

        // Upload to the GPU.
        m_d3d_context->Unmap(m_shader_buffer, 0);
        m_cbuffer_dirty = false;
    }
}

#endif // RMLUI_PLATFORM_WIN32