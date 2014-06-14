/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 Nuno Silva
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

#include <Rocket/Core/Core.h>
#include <SDL_image.h>

#include "RenderInterfaceSDL2.h"

#if !(SDL_VIDEO_RENDER_OGL)
    #error "Only the opengl sdl backend is supported. To add support for others, see http://mdqinc.com/blog/2013/01/integrating-librocket-with-sdl-2/"
#endif

RocketSDL2Renderer::RocketSDL2Renderer(SDL_Renderer* renderer, SDL_Window* screen)
{
    mRenderer = renderer;
    mScreen = screen;
}

// Called by Rocket when it wants to render geometry that it does not wish to optimise.
void RocketSDL2Renderer::RenderGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation)
{
    // SDL uses shaders that we need to disable here  
    glUseProgramObjectARB(0);
    glPushMatrix();
    glTranslatef(translation.x, translation.y, 0);
 
    std::vector<Rocket::Core::Vector2f> Positions(num_vertices);
    std::vector<Rocket::Core::Colourb> Colors(num_vertices);
    std::vector<Rocket::Core::Vector2f> TexCoords(num_vertices);
    float texw, texh;
 
    SDL_Texture* sdl_texture = NULL;
    if(texture)
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        sdl_texture = (SDL_Texture *) texture;
        SDL_GL_BindTexture(sdl_texture, &texw, &texh);
    }
 
    for(int  i = 0; i < num_vertices; i++) {
        Positions[i] = vertices[i].position;
        Colors[i] = vertices[i].colour;
        if (sdl_texture) {
            TexCoords[i].x = vertices[i].tex_coord.x * texw;
            TexCoords[i].y = vertices[i].tex_coord.y * texh;
        }
        else TexCoords[i] = vertices[i].tex_coord;
    };
 
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &Positions[0]);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, &Colors[0]);
    glTexCoordPointer(2, GL_FLOAT, 0, &TexCoords[0]);
 
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, indices);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
 
    if (sdl_texture) {
        SDL_GL_UnbindTexture(sdl_texture);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
 
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glPopMatrix();
    /* Reset blending and draw a fake point just outside the screen to let SDL know that it needs to reset its state in case it wants to render a texture */
    glDisable(GL_BLEND);
    SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_NONE);
    SDL_RenderDrawPoint(mRenderer, -1, -1);
}


// Called by Rocket when it wants to enable or disable scissoring to clip content.		
void RocketSDL2Renderer::EnableScissorRegion(bool enable)
{
    if (enable)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
}

// Called by Rocket when it wants to change the scissor region.		
void RocketSDL2Renderer::SetScissorRegion(int x, int y, int width, int height)
{
    int w_width, w_height;
    SDL_GetWindowSize(mScreen, &w_width, &w_height);
    glScissor(x, w_height - (y + height), width, height);
}

// Called by Rocket when a texture is required by the library.		
bool RocketSDL2Renderer::LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source)
{

    Rocket::Core::FileInterface* file_interface = Rocket::Core::GetFileInterface();
    Rocket::Core::FileHandle file_handle = file_interface->Open(source);
    if (!file_handle)
        return false;

    file_interface->Seek(file_handle, 0, SEEK_END);
    size_t buffer_size = file_interface->Tell(file_handle);
    file_interface->Seek(file_handle, 0, SEEK_SET);

    char* buffer = new char[buffer_size];
    file_interface->Read(buffer, buffer_size, file_handle);
    file_interface->Close(file_handle);

    size_t i;
    for(i = source.Length() - 1; i > 0; i--)
    {
        if(source[i] == '.')
            break;
    }

    Rocket::Core::String extension = source.Substring(i+1, source.Length()-i);

    SDL_Surface* surface = IMG_LoadTyped_RW(SDL_RWFromMem(buffer, buffer_size), 1, extension.CString());

    if (surface) {
        SDL_Texture *texture = SDL_CreateTextureFromSurface(mRenderer, surface);

        if (texture) {
            texture_handle = (Rocket::Core::TextureHandle) texture;
            texture_dimensions = Rocket::Core::Vector2i(surface->w, surface->h);
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

// Called by Rocket when a texture is required to be built from an internally-generated sequence of pixels.
bool RocketSDL2Renderer::GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& source_dimensions)
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
    texture_handle = (Rocket::Core::TextureHandle) texture;
    return true;
}

// Called by Rocket when a loaded texture is no longer required.		
void RocketSDL2Renderer::ReleaseTexture(Rocket::Core::TextureHandle texture_handle)
{
    SDL_DestroyTexture((SDL_Texture*) texture_handle);
}
