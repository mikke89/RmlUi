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
#include "RmlUi_Renderer_VK.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#if !SDL_VIDEO_VULKAN
	#error "Only the Vulkan SDL backend is supported."
#endif

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_SDL system_interface;
	RenderInterface_VK render_interface;

	SDL_Window* window = nullptr;

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

	const Uint32 window_flags = (SDL_WINDOW_VULKAN | (allow_resize ? SDL_WINDOW_RESIZABLE : 0));

	SDL_Window* window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
	if (!window)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "SDL error on create window: %s", SDL_GetError());
		return false;
	}

	data = Rml::MakeUnique<BackendData>();
	data->window = window;

	Rml::Vector<const char*> extensions;
	{
		unsigned int count;
		if (!SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr))
		{
			data.reset();
			Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to get required vulkan extensions");
			return false;
		}
		extensions.resize(count);
		if (!SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data()))
		{
			data.reset();
			Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to get required vulkan extensions");
			return false;
		}
	}

	if (!data->render_interface.Initialize(std::move(extensions),
			[](VkInstance instance, VkSurfaceKHR* out_surface) { return (bool)SDL_Vulkan_CreateSurface(data->window, instance, out_surface); }))
	{
		data.reset();
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to initialize Vulkan render interface");
		return false;
	}

	data->system_interface.SetWindow(window);
	data->render_interface.SetViewport(width, height);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	data->render_interface.Shutdown();

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

static bool WaitForValidSwapchain()
{
	bool result = true;

	// In some situations the swapchain may become invalid, such as when the window is minimized. In this state the renderer cannot accept any render
	// calls. Since we don't have full control over the main loop here we may risk calls to Context::Render if we were to return. Instead, we keep the
	// application inside this loop until we are able to recreate the swapchain and render again.
	while (!data->render_interface.IsSwapchainValid())
	{
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_QUIT)
			{
				// Restore the window so that we can recreate the swapchain, and then properly release all resource and shutdown cleanly.
				SDL_RestoreWindow(data->window);
				result = false;
			}
		}
		SDL_Delay(10);
		data->render_interface.RecreateSwapchain();
	}

	return result;
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

	bool result = data->running;
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
			if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				Rml::Vector2i dimensions(ev.window.data1, ev.window.data2);
				context->SetDimensions(dimensions);
				data->render_interface.SetViewport(dimensions.x, dimensions.y);
				break;
			}
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

	if (!WaitForValidSwapchain())
		result = false;

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
	data->render_interface.BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.EndFrame();
}
