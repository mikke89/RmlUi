/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "RmlUi_Backend.h"
#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_GL3.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <SDL.h>
#include <SDL_image.h>

#if defined RMLUI_PLATFORM_EMSCRIPTEN
	#include <emscripten.h>
#else
	#if !(SDL_VIDEO_RENDER_OGL)
		#error "Only the OpenGL SDL backend is supported."
	#endif
#endif

/**
    Custom render interface example for the SDL/GL3 backend.

    Overloads the OpenGL3 render interface to load textures through SDL_image's built-in texture loading functionality.
 */
class RenderInterface_GL3_SDL : public RenderInterface_GL3 {
public:
	RenderInterface_GL3_SDL() {}

	bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source) override
	{
		Rml::FileInterface* file_interface = Rml::GetFileInterface();
		Rml::FileHandle file_handle = file_interface->Open(source);
		if (!file_handle)
			return false;

		file_interface->Seek(file_handle, 0, SEEK_END);
		const size_t buffer_size = file_interface->Tell(file_handle);
		file_interface->Seek(file_handle, 0, SEEK_SET);

		using Rml::byte;
		Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
		file_interface->Read(buffer.get(), buffer_size, file_handle);
		file_interface->Close(file_handle);

		const size_t i = source.rfind('.');
		Rml::String extension = (i == Rml::String::npos ? Rml::String() : source.substr(i + 1));

		SDL_Surface* surface = IMG_LoadTyped_RW(SDL_RWFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str());

		bool success = false;
		if (surface)
		{
			texture_dimensions.x = surface->w;
			texture_dimensions.y = surface->h;

			if (surface->format->format != SDL_PIXELFORMAT_RGBA32)
			{
				SDL_SetSurfaceAlphaMod(surface, SDL_ALPHA_OPAQUE);
				SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

				SDL_Surface* new_surface = SDL_CreateRGBSurfaceWithFormat(0, surface->w, surface->h, 32, SDL_PIXELFORMAT_RGBA32);
				if (!new_surface)
					return false;

				if (SDL_BlitSurface(surface, 0, new_surface, 0) != 0)
					return false;

				SDL_FreeSurface(surface);
				surface = new_surface;
			}

			success = RenderInterface_GL3::GenerateTexture(texture_handle, (const Rml::byte*)surface->pixels, texture_dimensions);
			SDL_FreeSurface(surface);
		}

		return success;
	}
};

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_SDL system_interface;
	RenderInterface_GL3_SDL render_interface;

	SDL_Window* window = nullptr;
	SDL_GLContext glcontext = nullptr;

	bool running = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		return false;

#if defined RMLUI_PLATFORM_EMSCRIPTEN
	// GLES 3.0 (WebGL 2.0)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	// GL 3.3 Core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

	// Request stencil buffer of at least 8-bit size to supporting clipping on transformed elements.
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable linear filtering and MSAA for better-looking visuals, especially when transforms are applied.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

	const Uint32 window_flags = (SDL_WINDOW_OPENGL | (allow_resize ? SDL_WINDOW_RESIZABLE : 0));

	SDL_Window* window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
	if (!window)
	{
		// Try again on low-quality settings.
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
		if (!window)
		{
			fprintf(stderr, "SDL error on create window: %s\n", SDL_GetError());
			return false;
		}
	}

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glcontext);
	SDL_GL_SetSwapInterval(1);

	if (!RmlGL3::Initialize())
	{
		fprintf(stderr, "Could not initialize OpenGL");
		return false;
	}

	data = Rml::MakeUnique<BackendData>();

	if (!data->render_interface)
	{
		data.reset();
		fprintf(stderr, "Could not initialize OpenGL3 render interface.");
		return false;
	}

	data->window = window;
	data->glcontext = glcontext;

	data->system_interface.SetWindow(window);
	data->render_interface.SetViewport(width, height);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	SDL_GL_DeleteContext(data->glcontext);
	SDL_DestroyWindow(data->window);

	data.reset();

	SDL_Quit();
}

Rml::SystemInterface* Backend::GetSystemInterface()
{
	RMLUI_ASSERT(data);
	return &data->system_interface;
}

Rml::RenderInterface* Backend::GetRenderInterface()
{
	RMLUI_ASSERT(data);
	return &data->render_interface;
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

#if defined RMLUI_PLATFORM_EMSCRIPTEN

	// Ideally we would hand over control of the main loop to emscripten:
	//
	//  // Hand over control of the main loop to the WebAssembly runtime.
	//  emscripten_set_main_loop_arg(EventLoopIteration, (void*)user_data_handle, 0, true);
	//
	// The above is the recommended approach. However, as we don't control the main loop here we have to make due with another approach. Instead, use
	// Asyncify to yield by sleeping.
	// Important: Must be linked with option -sASYNCIFY
	emscripten_sleep(1);

#endif

	bool result = data->running;
	data->running = true;

	SDL_Event ev;
	int has_event = 0;
	if(power_save)
		has_event = SDL_WaitEventTimeout(&ev, static_cast<int>(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0)*1000));
	else has_event = SDL_PollEvent(&ev);
	while (has_event)
	{
		switch (ev.type)
		{
		case SDL_QUIT:
		{
			result = false;
		}
		break;
		case SDL_KEYDOWN:
		{
			const Rml::Input::KeyIdentifier key = RmlSDL::ConvertKey(ev.key.keysym.sym);
			const int key_modifier = RmlSDL::GetKeyModifierState();
			const float native_dp_ratio = 1.f;

			// See if we have any global shortcuts that take priority over the context.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, true))
				break;
			// Otherwise, hand the event over to the context by calling the input handler as normal.
			if (!RmlSDL::InputEventHandler(context, ev))
				break;
			// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, false))
				break;
		}
		break;
		case SDL_WINDOWEVENT:
		{
			switch (ev.window.event)
			{
			case SDL_WINDOWEVENT_SIZE_CHANGED:
			{
				Rml::Vector2i dimensions(ev.window.data1, ev.window.data2);
				data->render_interface.SetViewport(dimensions.x, dimensions.y);
			}
			break;
			}
			RmlSDL::InputEventHandler(context, ev);
		}
		break;
		default:
		{
			RmlSDL::InputEventHandler(context, ev);
		}
		break;
		}
		has_event = SDL_PollEvent(&ev);
	}

	return result;
}

void Backend::RequestExit()
{
	RMLUI_ASSERT(data);

	data->running = false;
}

void Backend::BeginFrame()
{
	RMLUI_ASSERT(data);

	data->render_interface.Clear();
	data->render_interface.BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);

	data->render_interface.EndFrame();
	SDL_GL_SwapWindow(data->window);
}
