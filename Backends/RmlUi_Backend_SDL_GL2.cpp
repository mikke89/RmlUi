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

#include "RmlUi_Backend.h"
#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_GL2.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_image.h>

#if !(SDL_VIDEO_RENDER_OGL)
	#error "Only the OpenGL SDL backend is supported."
#endif

/**
    Custom render interface example for the SDL/GL2 backend.

    Overloads the OpenGL2 render interface to load textures through SDL_image's built-in texture loading functionality.
 */
class RenderInterface_GL2_SDL : public RenderInterface_GL2 {
private:
	SDL_Renderer* renderer;

public:
	RenderInterface_GL2_SDL(SDL_Renderer* renderer) : renderer(renderer) {}

	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override
	{
		SDL_Texture* sdl_texture = (SDL_Texture*)texture;
		if (sdl_texture)
		{
			SDL_GL_BindTexture(sdl_texture, nullptr, nullptr);
			texture = RenderInterface_GL2::TextureEnableWithoutBinding;
		}

		RenderInterface_GL2::RenderGeometry(handle, translation, texture);

		if (sdl_texture)
			SDL_GL_UnbindTexture(sdl_texture);
	}

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override
	{
		Rml::FileInterface* file_interface = Rml::GetFileInterface();
		Rml::FileHandle file_handle = file_interface->Open(source);
		if (!file_handle)
			return {};

		file_interface->Seek(file_handle, 0, SEEK_END);
		const size_t buffer_size = file_interface->Tell(file_handle);
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

		texture_dimensions = {surface->w, surface->h};

		if (surface->format->format != SDL_PIXELFORMAT_RGBA32 && surface->format->format != SDL_PIXELFORMAT_BGRA32)
		{
			// Ensure correct format for premultiplied alpha conversion below. Additionally, fix rendering images with
			// no alpha channel, see https://github.com/mikke89/RmlUi/issues/239
			SDL_Surface* converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA32, 0);
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
		SDL_FreeSurface(surface);

		return (Rml::TextureHandle)texture;
	}

	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override
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

		SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)source.data(), source_dimensions.x, source_dimensions.y, 32, source_dimensions.x * 4,
			rmask, gmask, bmask, amask);
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_FreeSurface(surface);
		return (Rml::TextureHandle)texture;
	}

	void ReleaseTexture(Rml::TextureHandle texture_handle) override { SDL_DestroyTexture((SDL_Texture*)texture_handle); }
};

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	BackendData(SDL_Renderer* renderer) : render_interface(renderer) {}

	SystemInterface_SDL system_interface;
	RenderInterface_GL2_SDL render_interface;

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_GLContext glcontext = nullptr;

	bool running = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
		return false;

	// Submit click events when focusing the window.
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

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
			Rml::Log::Message(Rml::Log::LT_ERROR, "SDL error on create window: %s", SDL_GetError());
			return false;
		}
	}

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	int opengl_renderer_index = -1;
	int num_render_drivers = SDL_GetNumRenderDrivers();
	for (int i = 0; i < num_render_drivers; i++)
	{
		SDL_RendererInfo info;
		if (SDL_GetRenderDriverInfo(i, &info) == 0)
		{
			if (strcmp(info.name, "opengl") == 0)
				opengl_renderer_index = i;
		}
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, opengl_renderer_index, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer)
		return false;

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "GLEW error: %s", glewGetErrorString(err));
		return false;
	}

	data = Rml::MakeUnique<BackendData>(renderer);

	data->window = window;
	data->glcontext = glcontext;
	data->renderer = renderer;

	data->system_interface.SetWindow(window);
	data->render_interface.SetViewport(width, height);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	SDL_DestroyRenderer(data->renderer);
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

	bool result = data->running;
	data->running = true;

	SDL_Event ev;
	int has_event = 0;
	if (power_save)
		has_event = SDL_WaitEventTimeout(&ev, static_cast<int>(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0) * 1000));
	else
		has_event = SDL_PollEvent(&ev);
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

	SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 0);
	SDL_RenderClear(data->renderer);

	// SDL uses shaders that we need to disable here.
	glUseProgramObjectARB(0);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	data->render_interface.BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);

	data->render_interface.EndFrame();

	// Draw a fake point just outside the screen to let SDL know that it needs to reset its state in case it wants to render a texture next frame.
	SDL_SetRenderDrawBlendMode(data->renderer, SDL_BLENDMODE_NONE);
	SDL_RenderDrawPoint(data->renderer, -1, -1);

	SDL_RenderPresent(data->renderer);
}
