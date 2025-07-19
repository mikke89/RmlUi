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
    1, 0,
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

RenderInterface_SDL_GPU::RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* window) : device(device), window(window)
{
    SDL_GPUShader* colorShader = CreateShaderFromMemory(device, ShaderTypeColor);
    SDL_GPUShader* textureShader = CreateShaderFromMemory(device, ShaderTypeTexture);
    SDL_GPUShader* vertShader = CreateShaderFromMemory(device, ShaderTypeVert);

    SDL_ReleaseGPUShader(device, colorShader);
    SDL_ReleaseGPUShader(device, textureShader);
    SDL_ReleaseGPUShader(device, vertShader);
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
}

void RenderInterface_SDL_GPU::EndFrame()
{
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

CompiledGeometryHandle RenderInterface_SDL_GPU::CompileGeometry(Span<const Vertex> vertices, Span<const int> indices)
{
    (void) vertices;
    (void) indices;
	return 0;
}

void RenderInterface_SDL_GPU::ReleaseGeometry(CompiledGeometryHandle geometry)
{
    (void) geometry;
}

void RenderInterface_SDL_GPU::RenderGeometry(CompiledGeometryHandle handle, Vector2f translation, TextureHandle texture)
{
    (void) handle;
    (void) translation;
    (void) texture;
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
    (void) texture_dimensions;
    (void) source;
    return 0;
}

TextureHandle RenderInterface_SDL_GPU::GenerateTexture(Span<const byte> source, Vector2i source_dimensions)
{
    (void) source;
    (void) source_dimensions;
    return 0;
}

void RenderInterface_SDL_GPU::ReleaseTexture(TextureHandle texture_handle)
{
    (void) texture_handle;
}