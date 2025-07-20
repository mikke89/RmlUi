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

#include "RmlUi_Renderer_SDL_GPU.h"
#include "RmlUi_SDL_GPU/ShadersCompiledSPV.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Types.h>

#include <SDL3_image/SDL_image.h>

using namespace Rml;

enum ShaderType
{
    ShaderTypeColor,
    ShaderTypeTexture,
    ShaderTypeVert,
    ShaderTypeCount,
};

enum ShaderFormat
{
    ShaderFormatSPIRV,
    ShaderFormatMSL,
    ShaderFormatDXIL,
    ShaderFormatCount,
};

struct Shader
{
    Span<const byte> data[ShaderFormatCount];
    int uniforms;
    int samplers;
    SDL_GPUShaderStage stage;
};

#undef X
#define X(name) Span<const byte>{name, sizeof(name)}

static const Shader shaders[ShaderTypeCount] =
{{
    {
        X(shader_frag_color_spirv),
        X(shader_frag_color_msl),
        X(shader_frag_color_dxil)
    },
    0, 0,
    SDL_GPU_SHADERSTAGE_FRAGMENT
},
{
    {
        X(shader_frag_texture_spirv),
        X(shader_frag_texture_msl),
        X(shader_frag_texture_dxil)
    },
    0, 1,
    SDL_GPU_SHADERSTAGE_FRAGMENT
},
{
    {
        X(shader_vert_spirv),
        X(shader_vert_msl),
        X(shader_vert_dxil)
    },
    2, 0,
    SDL_GPU_SHADERSTAGE_VERTEX
}};

#undef X

static SDL_GPUShader* CreateShaderFromMemory(SDL_GPUDevice* device, ShaderType type)
{
    SDL_GPUShaderFormat sdlShaderFormat = SDL_GetGPUShaderFormats(device);
    ShaderFormat format = ShaderFormatCount;
    const char* entrypoint = nullptr;
    if (sdlShaderFormat & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        sdlShaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
        format = ShaderFormatSPIRV;
        entrypoint = "main";
    }
    else if (sdlShaderFormat & SDL_GPU_SHADERFORMAT_DXIL)
    {
        sdlShaderFormat = SDL_GPU_SHADERFORMAT_DXIL;
        format = ShaderFormatDXIL;
        entrypoint = "main";
    }
    else if (sdlShaderFormat & SDL_GPU_SHADERFORMAT_MSL)
    {
        sdlShaderFormat = SDL_GPU_SHADERFORMAT_MSL;
        format = ShaderFormatMSL;
        entrypoint = "main0";
    }
    else
    {
        RMLUI_ERRORMSG("Invalid shader format");
    }
    const Shader& shader = shaders[type];
    SDL_GPUShaderCreateInfo info{};
    info.code = static_cast<const Uint8*>(shader.data[format].data());
    info.code_size = shader.data[format].size();
    info.entrypoint = entrypoint;
    info.format = sdlShaderFormat;
    info.stage = shader.stage;
    info.num_samplers = shader.samplers;
    info.num_uniform_buffers = shader.uniforms;
    SDL_GPUShader* sdlShader = SDL_CreateGPUShader(device, &info);
    if (!sdlShader)
    {
        Log::Message(Log::LT_ERROR, "Failed to create shader: %s", SDL_GetError());
        RMLUI_ERROR;
    }
    return sdlShader;
}
 
void RenderInterface_SDL_GPU::CreatePipelines()
{
    SDL_GPUShader* color_shader = CreateShaderFromMemory(device, ShaderTypeColor);
    SDL_GPUShader* texture_shader = CreateShaderFromMemory(device, ShaderTypeTexture);
    SDL_GPUShader* vert_shader = CreateShaderFromMemory(device, ShaderTypeVert);

    SDL_GPUColorTargetDescription target{};
    target.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    target.blend_state.enable_blend = true;
    target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

    SDL_GPUVertexAttribute attrib[3]{};
    attrib[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrib[0].location = 0;
    attrib[0].offset = offsetof(Vertex, position);
    attrib[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    attrib[1].location = 1;
    attrib[1].offset = offsetof(Vertex, colour);
    attrib[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrib[2].location = 2;
    attrib[2].offset = offsetof(Vertex, tex_coord);

    SDL_GPUVertexBufferDescription buffer{};
    buffer.pitch = sizeof(Vertex);

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vert_shader;

    info.target_info.num_color_targets = 1;
    info.target_info.color_target_descriptions = &target;

    info.vertex_input_state.num_vertex_attributes = 3;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attrib;
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;

    info.fragment_shader = color_shader;
    color_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!color_pipeline)
    {
        Log::Message(Log::LT_ERROR, "Failed to create color pipeline: %s", SDL_GetError());
        RMLUI_ERROR;
    }

    info.fragment_shader = texture_shader;
    texture_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!texture_pipeline)
    {
        Log::Message(Log::LT_ERROR, "Failed to create texture pipeline: %s", SDL_GetError());
        RMLUI_ERROR;
    }

    SDL_ReleaseGPUShader(device, color_shader);
    SDL_ReleaseGPUShader(device, texture_shader);
    SDL_ReleaseGPUShader(device, vert_shader);
}

RenderInterface_SDL_GPU::RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* window) : device(device), window(window)
{
    CreatePipelines();

    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    linear_sampler = SDL_CreateGPUSampler(device, &info);
    if (!linear_sampler)
    {
        Log::Message(Log::LT_ERROR, "Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
}

void RenderInterface_SDL_GPU::Shutdown()
{
    SDL_ReleaseGPUGraphicsPipeline(device, color_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, texture_pipeline);
}

void RenderInterface_SDL_GPU::BeginFrame()
{
    SDL_WaitForGPUSwapchain(device, window);

    command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer)
    {
        Log::Message(Log::LT_ERROR, "Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &swapchain_width, &swapchain_height))
    {
        Log::Message(Log::LT_ERROR, "Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        return;
    }

    if (!swapchain_texture || !swapchain_width || !swapchain_height)
    {
        // Not an error. Happens on minimize
        SDL_CancelGPUCommandBuffer(command_buffer);
        return;
    }

    SDL_GPUColorTargetInfo color_info{};
    color_info.texture = swapchain_texture;
    color_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_info, 1, nullptr);
    SDL_EndGPURenderPass(render_pass);
}

void RenderInterface_SDL_GPU::EndFrame()
{
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

struct GeometryView
{
    GeometryView(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, Span<const Vertex> vertices, Span<const int> indices)
    {
        // TODO: On error, there's a few leaks. Not sure it's a big deal though since it never should happen anyways

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        if (!copy_pass)
        {
            Log::Message(Log::LT_ERROR, "Failed to begin copy pass: %s", SDL_GetError());
            return;
        }

        SDL_GPUTransferBuffer* vertex_transfer_buffer;
        SDL_GPUTransferBuffer* index_transfer_buffer;

        {
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = static_cast<Uint32>(vertices.size() * sizeof(Vertex));
            vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
            info.size = static_cast<Uint32>(indices.size() * sizeof(int));
            index_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!vertex_transfer_buffer || !index_transfer_buffer)
            {
                Log::Message(Log::LT_ERROR, "Failed to create transfer buffers(s): %s", SDL_GetError());
                return;
            }
        }

        Vertex* vertex_data = static_cast<Vertex*>(SDL_MapGPUTransferBuffer(device, vertex_transfer_buffer, false));
        int* index_data = static_cast<int*>(SDL_MapGPUTransferBuffer(device, index_transfer_buffer, false));
        if (!vertex_data || !index_data)
        {
            Log::Message(Log::LT_ERROR, "Failed to map transfer buffers(s): %s", SDL_GetError());
            return;
        }

        std::memcpy(vertex_data, vertices.data(), vertices.size() * sizeof(Vertex));
        std::memcpy(index_data, indices.data(), indices.size() * sizeof(int));

        SDL_UnmapGPUTransferBuffer(device, vertex_transfer_buffer);
        SDL_UnmapGPUTransferBuffer(device, index_transfer_buffer);

        {
            SDL_GPUBufferCreateInfo info{};
            info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            info.size = static_cast<Uint32>(vertices.size() * sizeof(Vertex));
            vertex_buffer = SDL_CreateGPUBuffer(device, &info);
            info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            info.size = static_cast<Uint32>(indices.size() * sizeof(int));
            index_buffer = SDL_CreateGPUBuffer(device, &info);
            if (!vertex_buffer || !index_buffer)
            {
                Log::Message(Log::LT_ERROR, "Failed to create buffers(s): %s", SDL_GetError());
                return;
            }
        }

        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};

        location.transfer_buffer = vertex_transfer_buffer;
        region.buffer = vertex_buffer;
        region.size = static_cast<Uint32>(vertices.size() * sizeof(Vertex));
        SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
        SDL_ReleaseGPUTransferBuffer(device, vertex_transfer_buffer);

        location.transfer_buffer = index_transfer_buffer;
        region.buffer = index_buffer;
        region.size = static_cast<Uint32>(indices.size() * sizeof(int));
        SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
        SDL_ReleaseGPUTransferBuffer(device, index_transfer_buffer);

        SDL_EndGPUCopyPass(copy_pass);

        num_indices = static_cast<int>(indices.size());
    }

    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
    int num_indices;
};

CompiledGeometryHandle RenderInterface_SDL_GPU::CompileGeometry(Span<const Vertex> vertices, Span<const int> indices)
{
	GeometryView* data = new GeometryView{device, command_buffer, vertices, indices};
	return reinterpret_cast<Rml::CompiledGeometryHandle>(data);
}

void RenderInterface_SDL_GPU::ReleaseGeometry(CompiledGeometryHandle handle)
{
    GeometryView* data = reinterpret_cast<GeometryView*>(handle);

    SDL_ReleaseGPUBuffer(device, data->vertex_buffer);
    SDL_ReleaseGPUBuffer(device, data->index_buffer);

	delete reinterpret_cast<GeometryView*>(handle);
}

void RenderInterface_SDL_GPU::RenderGeometry(CompiledGeometryHandle handle, Vector2f translation, TextureHandle texture)
{
    GeometryView* data = reinterpret_cast<GeometryView*>(handle);

    SDL_GPUColorTargetInfo color_info{};
    color_info.texture = swapchain_texture;
    color_info.load_op = SDL_GPU_LOADOP_LOAD;
    color_info.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_info, 1, nullptr);
    if (!render_pass)
    {
        Log::Message(Log::LT_ERROR, "Failed to begin render pass: %s", SDL_GetError());
        return;
    }

    if (texture != 0)
    {
        SDL_BindGPUGraphicsPipeline(render_pass, texture_pipeline);

        SDL_GPUTextureSamplerBinding texture_binding{};
        texture_binding.texture = reinterpret_cast<SDL_GPUTexture*>(texture);
        texture_binding.sampler = linear_sampler;
        SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);
    }
    else
    {
        SDL_BindGPUGraphicsPipeline(render_pass, color_pipeline);
    }

    SDL_GPUBufferBinding vertex_buffer_binding{};
    SDL_GPUBufferBinding index_buffer_binding{};
    vertex_buffer_binding.buffer = data->vertex_buffer;
    index_buffer_binding.buffer = data->index_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
    SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    Matrix4f identity_matrix = Matrix4f::Identity();
    SDL_PushGPUVertexUniformData(command_buffer, 0, &identity_matrix, sizeof(identity_matrix));
    SDL_PushGPUVertexUniformData(command_buffer, 1, &translation, sizeof(translation));

    SDL_DrawGPUIndexedPrimitives(render_pass, data->num_indices, 1, 0, 0, 0);
    SDL_EndGPURenderPass(render_pass);
}

void RenderInterface_SDL_GPU::EnableScissorRegion(bool enable)
{
    (void) enable;
}

void RenderInterface_SDL_GPU::SetScissorRegion(Rectanglei region)
{
    (void) region;
}

TextureHandle RenderInterface_SDL_GPU::LoadTexture(Vector2i& texture_dimensions, const String& source)
{
    SDL_Surface* surface = IMG_Load(source.data());
    if (!surface)
    {
        Log::Message(Log::LT_ERROR, "Failed to load surface: %s, %s", source.data(), SDL_GetError());
        return 0;
    }

    if (surface->format != SDL_PIXELFORMAT_RGBA32)
    {
        SDL_Surface* converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (!surface)
        {
            Log::Message(Log::LT_ERROR, "Failed to convert surface: %s", SDL_GetError());
            SDL_DestroySurface(surface);
            return 0;
        }

        SDL_DestroySurface(surface);
        surface = converted_surface;
    }

    Span<const byte> data{static_cast<const byte*>(surface->pixels), static_cast<size_t>(surface->pitch * surface->h)};
    texture_dimensions = {surface->w, surface->h};

    TextureHandle handle = GenerateTexture(data, texture_dimensions);

    SDL_DestroySurface(surface);

    return handle;
}

TextureHandle RenderInterface_SDL_GPU::GenerateTexture(Span<const byte> source, Vector2i source_dimensions)
{
    // TODO: On error, there's a few leaks. Not sure it's a big deal though since it never should happen anyways

    SDL_GPUTransferBuffer* transfer_buffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = source_dimensions.x * source_dimensions.y * 4;
        transfer_buffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_buffer)
        {
            Log::Message(Log::LT_ERROR, "Failed to create transfer buffer: %s", SDL_GetError());
            return 0;
        }
    }

    void* dst = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!dst)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return 0;
    }

    std::memcpy(dst, source.data(), source_dimensions.x * source_dimensions.y * 4);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUTexture* texture;
    {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.width = source_dimensions.x;
        info.height = source_dimensions.y;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        texture = SDL_CreateGPUTexture(device, &info);
        if (!texture)
        {
            Log::Message(Log::LT_ERROR, "Failed to create texture: %s", SDL_GetError());
            return 0;
        }
    }

    SDL_GPUTextureTransferInfo transfer_info{};
    SDL_GPUTextureRegion region{};
    transfer_info.transfer_buffer = transfer_buffer;
    region.texture = texture;
    region.w = source_dimensions.x;
    region.h = source_dimensions.y;
    region.d = 1;

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        Log::Message(Log::LT_ERROR, "Failed to begin copy pass: %s", SDL_GetError());
        return 0;
    }

    SDL_UploadToGPUTexture(copy_pass, &transfer_info, &region, false);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    SDL_EndGPUCopyPass(copy_pass);

    return reinterpret_cast<TextureHandle>(texture);
}

void RenderInterface_SDL_GPU::ReleaseTexture(TextureHandle texture_handle)
{
    SDL_GPUTexture* texture = reinterpret_cast<SDL_GPUTexture*>(texture_handle);
    SDL_ReleaseGPUTexture(device, texture);
}