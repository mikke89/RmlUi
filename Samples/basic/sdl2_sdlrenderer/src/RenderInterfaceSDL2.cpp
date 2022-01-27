/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 Nuno Silva
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

#include <RmlUi/Core.h>
#include <SDL_image.h>

#include "RenderInterfaceSDL2.h"

RmlUiSDL2Renderer::RmlUiSDL2Renderer(SDL_Renderer* renderer, SDL_Window* screen)
{
    mRenderer = renderer;
    mScreen = screen;

    SDL_GetRendererOutputSize(renderer, &mRenderer_w, &mRenderer_h);

    mRectScisor.x = 0;
    mRectScisor.y = 0;
    mRectScisor.w = mRenderer_w;
    mRectScisor.h = mRenderer_h;
}

// Called by RmlUi when it wants to render geometry that it does not wish to optimise.
void RmlUiSDL2Renderer::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
    SDL_Texture *sdl_texture = (SDL_Texture *) texture;

    SDL_Rect r;
    r.x = (int)translation.x;
    r.y = (int)translation.y;
    r.w = mRenderer_w - r.x;
    r.h = mRenderer_h - r.y;
    
    SDL_RenderSetViewport(mRenderer, &r);

    if (sdl_texture) {
        SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_BLEND);
    }

    int sz = sizeof(vertices[0]);
    int off1 = offsetof(Rml::Vertex, position);
    int off2 = offsetof(Rml::Vertex, colour);
    int off3 = offsetof(Rml::Vertex, tex_coord);
    SDL_RenderGeometryRaw(mRenderer, sdl_texture, 
            (float *)((Uint8 *) vertices + off1), sz,
            (SDL_Color *)((Uint8 *) vertices + off2), sz,
            (float *)((Uint8 *) vertices + off3), sz,
            num_vertices, indices, num_indices, 4);

}

// Called by RmlUi when it wants to enable or disable scissoring to clip content.		
void RmlUiSDL2Renderer::EnableScissorRegion(bool enable)
{
    if (enable) {
        SDL_RenderSetClipRect(mRenderer, &mRectScisor);
    } else {
        SDL_RenderSetClipRect(mRenderer, NULL);
    }
}

// Called by RmlUi when it wants to change the scissor region.		
void RmlUiSDL2Renderer::SetScissorRegion(int x, int y, int width, int height)
{
    int w_width, w_height;
    SDL_GetWindowSize(mScreen, &w_width, &w_height);
    mRectScisor.x = x;
    mRectScisor.y = w_height - (y + height);
    mRectScisor.w = width;
    mRectScisor.h = height;
}

// Called by RmlUi when a texture is required by the library.		
bool RmlUiSDL2Renderer::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source)
{

    Rml::FileInterface* file_interface = Rml::GetFileInterface();
    Rml::FileHandle file_handle = file_interface->Open(source);
    if (!file_handle)
        return false;

    file_interface->Seek(file_handle, 0, SEEK_END);
    size_t buffer_size = file_interface->Tell(file_handle);
    file_interface->Seek(file_handle, 0, SEEK_SET);

    char* buffer = new char[buffer_size];
    file_interface->Read(buffer, buffer_size, file_handle);
    file_interface->Close(file_handle);

    size_t i;
    for(i = source.length() - 1; i > 0; i--)
    {
        if(source[i] == '.')
            break;
    }

    Rml::String extension = source.substr(i+1, source.length()-i);

    SDL_Surface* surface = IMG_LoadTyped_RW(SDL_RWFromMem(buffer, int(buffer_size)), 1, extension.c_str());

    if (surface) {
        SDL_Texture *texture = SDL_CreateTextureFromSurface(mRenderer, surface);

        if (texture) {
            texture_handle = (Rml::TextureHandle) texture;
            texture_dimensions = Rml::Vector2i(surface->w, surface->h);
            SDL_FreeSurface(surface);
        }
        else
        {
            return false;
        }

        return true;
    }

    return false;
}

// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
bool RmlUiSDL2Renderer::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        Uint32 rmask = 0xff000000;
        Uint32 gmask = 0x00ff0000;
        Uint32 bmask = 0x0000ff00;
        Uint32 amask = 0x000000ff;
    #else
        Uint32 rmask = 0x000000ff;
        Uint32 gmask = 0x0000ff00;
        Uint32 bmask = 0x00ff0000;
        Uint32 amask = 0xff000000;
    #endif

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom ((void*) source, source_dimensions.x, source_dimensions.y, 32, source_dimensions.x*4, rmask, gmask, bmask, amask);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(mRenderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surface);
    texture_handle = (Rml::TextureHandle) texture;
    return true;
}

// Called by RmlUi when a loaded texture is no longer required.		
void RmlUiSDL2Renderer::ReleaseTexture(Rml::TextureHandle texture_handle)
{
    SDL_DestroyTexture((SDL_Texture*) texture_handle);
}
