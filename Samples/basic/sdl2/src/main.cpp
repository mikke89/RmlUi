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

#include <Rocket/Core.h>
#include <Rocket/Core/Input.h>
#include <Rocket/Debugger/Debugger.h>
#include <ShellFileInterface.h>

#include "SystemInterfaceSDL2.h"
#include "RenderInterfaceSDL2.h"

#include <SDL.h>

#include <GL/glew.h>

int main(int argc, char **argv)
{
#ifdef ROCKET_PLATFORM_LINUX
#define APP_PATH "../Samples/basic/sdl2/"
#else
#ifdef ROCKET_PLATFORM_MACOSX
#define APP_PATH "../../../../../Samples/basic/sdl2/"
#else
#define APP_PATH "../../Samples/basic/sdl2/"
#endif
#endif

#ifdef ROCKET_PLATFORM_WIN32
        DoAllocConsole();
#endif

        int window_width = 1024;
        int window_height = 768;

    SDL_Init( SDL_INIT_VIDEO );
    SDL_Window * screen = SDL_CreateWindow("LibRocket SDL2 test", 20, 20, window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(screen);
    int oglIdx = -1;
    int nRD = SDL_GetNumRenderDrivers();
    for(int i=0; i<nRD; i++)
    {
        SDL_RendererInfo info;
        if(!SDL_GetRenderDriverInfo(i, &info))
        {
            if(!strcmp(info.name, "opengl"))
            {
                oglIdx = i;
            }
        }
    }
    SDL_Renderer * renderer = SDL_CreateRenderer(screen, oglIdx, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    GLenum err = glewInit();

    if(err != GLEW_OK)
        fprintf(stderr, "GLEW ERROR: %s\n", glewGetErrorString(err));

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    glMatrixMode(GL_PROJECTION|GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, window_width, window_height, 0, 0, 1);
 
	RocketSDL2Renderer Renderer(renderer, screen);
	RocketSDL2SystemInterface SystemInterface;
	ShellFileInterface FileInterface("../Samples/assets/");

	Rocket::Core::SetFileInterface(&FileInterface);
	Rocket::Core::SetRenderInterface(&Renderer);
    Rocket::Core::SetSystemInterface(&SystemInterface);

	if(!Rocket::Core::Initialise())
		return 1;

	Rocket::Core::FontDatabase::LoadFontFace("Delicious-Bold.otf");
	Rocket::Core::FontDatabase::LoadFontFace("Delicious-BoldItalic.otf");
	Rocket::Core::FontDatabase::LoadFontFace("Delicious-Italic.otf");
	Rocket::Core::FontDatabase::LoadFontFace("Delicious-Roman.otf");

	Rocket::Core::Context *Context = Rocket::Core::CreateContext("default",
		Rocket::Core::Vector2i(window_width, window_height));

	Rocket::Debugger::Initialise(Context);

	Rocket::Core::ElementDocument *Document = Context->LoadDocument("demo.rml");

	if(Document)
	{
		Document->Show();
		Document->RemoveReference();
		fprintf(stdout, "\nDocument loaded");
	}
	else
	{
		fprintf(stdout, "\nDocument is NULL");
	}

    bool done = false;

	while(!done)
	{
        SDL_Event event;

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderClear(renderer);

		Context->Render();
        SDL_RenderPresent(renderer);

        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    done = true;
                    break;

                case SDL_MOUSEMOTION:
                    Context->ProcessMouseMove(event.motion.x, event.motion.y, SystemInterface.GetKeyModifiers());
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    Context->ProcessMouseButtonDown(SystemInterface.TranslateMouseButton(event.button.button), SystemInterface.GetKeyModifiers());
                    break;

                case SDL_MOUSEBUTTONUP:
                    Context->ProcessMouseButtonUp(SystemInterface.TranslateMouseButton(event.button.button), SystemInterface.GetKeyModifiers());
                    break;

                case SDL_MOUSEWHEEL:
                    Context->ProcessMouseWheel(event.wheel.y, SystemInterface.GetKeyModifiers());
                    break;

                case SDL_KEYDOWN:
                {
                    // Intercept SHIFT + ~ key stroke to toggle libRocket's 
                    // visual debugger tool
                    if( event.key.keysym.sym == SDLK_BACKQUOTE && 
                        event.key.keysym.mod == KMOD_LSHIFT )
                    {
                        Rocket::Debugger::SetVisible( ! Rocket::Debugger::IsVisible() );
                        break;
                    }
                    
                    Context->ProcessKeyDown(SystemInterface.TranslateKey(event.key.keysym.sym), SystemInterface.GetKeyModifiers());
                    break;
                }
                
                default:
                    break;
            }
        }
		Context->Update();
	}

    Context->RemoveReference();
    Rocket::Core::Shutdown();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();

	return 0;
};
