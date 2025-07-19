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
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Types.h>

#include <SDL3_image/SDL_image.h>

RenderInterface_SDL_GPU::RenderInterface_SDL_GPU(SDL_GPUDevice* device) : device(device)
{

}

void RenderInterface_SDL_GPU::BeginFrame()
{

}

void RenderInterface_SDL_GPU::EndFrame()
{

}

Rml::CompiledGeometryHandle RenderInterface_SDL_GPU::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	return 0;
}

void RenderInterface_SDL_GPU::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{

}

void RenderInterface_SDL_GPU::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture)
{

}

void RenderInterface_SDL_GPU::EnableScissorRegion(bool enable)
{

}

void RenderInterface_SDL_GPU::SetScissorRegion(Rml::Rectanglei region)
{

}

Rml::TextureHandle RenderInterface_SDL_GPU::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
    return 0;
}

Rml::TextureHandle RenderInterface_SDL_GPU::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
    return 0;
}

void RenderInterface_SDL_GPU::ReleaseTexture(Rml::TextureHandle texture_handle)
{

}