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

#include "RmlUi_Renderer_SDL.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL.h>
#include <SDL_image.h>

RenderInterface_SDL::RenderInterface_SDL(SDL_Renderer* renderer) : renderer(renderer)
{
	// RmlUi serves vertex colors and textures with premultiplied alpha, set the blend mode accordingly.
	// Equivalent to glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
	blend_mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE,
		SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
}

void RenderInterface_SDL::BeginFrame()
{
	SDL_RenderSetViewport(renderer, nullptr);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawBlendMode(renderer, blend_mode);
}

void RenderInterface_SDL::EndFrame() {}

Rml::CompiledGeometryHandle RenderInterface_SDL::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	GeometryView* data = new GeometryView{vertices, indices};
	return reinterpret_cast<Rml::CompiledGeometryHandle>(data);
}

void RenderInterface_SDL::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	delete reinterpret_cast<GeometryView*>(geometry);
}

void RenderInterface_SDL::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	const GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);
	const Rml::Vertex* vertices = geometry->vertices.data();
	const size_t num_vertices = geometry->vertices.size();
	const int* indices = geometry->indices.data();
	const size_t num_indices = geometry->indices.size();

	SDL_FPoint* positions = new SDL_FPoint[num_vertices];

	for (size_t i = 0; i < num_vertices; i++)
	{
		positions[i].x = vertices[i].position.x + translation.x;
		positions[i].y = vertices[i].position.y + translation.y;
	}

	SDL_Texture* sdl_texture = (SDL_Texture*)texture;

	SDL_RenderGeometryRaw(renderer, sdl_texture, &positions[0].x, sizeof(SDL_FPoint), (const SDL_Color*)&vertices->colour, sizeof(Rml::Vertex),
		&vertices->tex_coord.x, sizeof(Rml::Vertex), (int)num_vertices, indices, (int)num_indices, 4);

	delete[] positions;
}

void RenderInterface_SDL::EnableScissorRegion(bool enable)
{
	if (enable)
		SDL_RenderSetClipRect(renderer, &rect_scissor);
	else
		SDL_RenderSetClipRect(renderer, nullptr);

	scissor_region_enabled = enable;
}

void RenderInterface_SDL::SetScissorRegion(Rml::Rectanglei region)
{
	rect_scissor.x = region.Left();
	rect_scissor.y = region.Top();
	rect_scissor.w = region.Width();
	rect_scissor.h = region.Height();

	if (scissor_region_enabled)
		SDL_RenderSetClipRect(renderer, &rect_scissor);
}

Rml::TextureHandle RenderInterface_SDL::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
		return {};

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	using Rml::byte;
	Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	const size_t i_ext = source.rfind('.');
	Rml::String extension = (i_ext == Rml::String::npos ? Rml::String() : source.substr(i_ext + 1));

	SDL_Surface* surface = IMG_LoadTyped_RW(SDL_RWFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str());
	if (!surface)
		return {};

	if (surface->format->format != SDL_PIXELFORMAT_RGBA32 && surface->format->format != SDL_PIXELFORMAT_BGRA32)
	{
		SDL_Surface* converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(surface);

		if (!converted_surface)
			return {};

		surface = converted_surface;
	}

	// Convert colors to premultiplied alpha, which is necessary for correct alpha compositing.
	byte* pixels = static_cast<byte*>(surface->pixels);
	for (int i = 0; i < surface->w * surface->h * 4; i += 4)
	{
		const byte alpha = pixels[i + 3];
		for (int j = 0; j < 3; ++j)
			pixels[i + j] = byte(int(pixels[i + j]) * int(alpha) / 255);
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	texture_dimensions = Rml::Vector2i(surface->w, surface->h);
	SDL_FreeSurface(surface);

	if (texture)
		SDL_SetTextureBlendMode(texture, blend_mode);

	return (Rml::TextureHandle)texture;
}

Rml::TextureHandle RenderInterface_SDL::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom((void*)source.data(), source_dimensions.x, source_dimensions.y, 32,
		source_dimensions.x * 4, SDL_PIXELFORMAT_RGBA32);

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_SetTextureBlendMode(texture, blend_mode);

	SDL_FreeSurface(surface);
	return (Rml::TextureHandle)texture;
}

void RenderInterface_SDL::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	SDL_DestroyTexture((SDL_Texture*)texture_handle);
}
