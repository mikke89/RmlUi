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
#include "RmlUi_Renderer_SDL_GPU.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	BackendData(SDL_GPUDevice* device, SDL_Window* window) : render_interface(device, window) {}

	SystemInterface_SDL system_interface;
	RenderInterface_SDL_GPU render_interface;

	SDL_Window* window = nullptr;
	SDL_GPUDevice* device = nullptr;
	SDL_GPUCommandBuffer* command_buffer = nullptr;

	bool running = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	if (!SDL_Init(SDL_INIT_VIDEO))
		return false;

	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

	const float window_size_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, window_name);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, int(width * window_size_scale));
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, int(height * window_size_scale));
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, allow_resize);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
	SDL_Window* window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);

	if (!window)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "SDL error on create window: %s\n", SDL_GetError());
		return false;
	}

	props = SDL_CreateProperties();
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
#ifdef RMLUI_DEBUG
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
#endif
	SDL_GPUDevice* device = SDL_CreateGPUDeviceWithProperties(props);
	SDL_DestroyProperties(props);

	if (!device)
		return false;

	if (!SDL_ClaimWindowForGPUDevice(device, window))
		return false;

	data = Rml::MakeUnique<BackendData>(device, window);
	data->window = window;
	data->device = device;
	data->system_interface.SetWindow(window);

	const char* renderer_name = SDL_GetGPUDeviceDriver(device);
	data->system_interface.LogMessage(Rml::Log::LT_INFO, Rml::CreateString("Using SDL device driver: %s", renderer_name));

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	data->render_interface.Shutdown();

	SDL_ReleaseWindowFromGPUDevice(data->device, data->window);
	SDL_DestroyGPUDevice(data->device);
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

	auto GetKey = [](const SDL_Event& event) { return event.key.key; };
	auto GetDisplayScale = []() { return SDL_GetWindowDisplayScale(data->window); };
	constexpr auto event_quit = SDL_EVENT_QUIT;
	constexpr auto event_key_down = SDL_EVENT_KEY_DOWN;
	bool has_event = false;

	bool result = data->running;
	data->running = true;

	SDL_Event ev;
	if (power_save)
		has_event = SDL_WaitEventTimeout(&ev, static_cast<int>(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0) * 1000));
	else
		has_event = SDL_PollEvent(&ev);

	while (has_event)
	{
		bool propagate_event = true;
		switch (ev.type)
		{
		case event_quit:
		{
			propagate_event = false;
			result = false;
		}
		break;
		case event_key_down:
		{
			propagate_event = false;
			const Rml::Input::KeyIdentifier key = RmlSDL::ConvertKey(GetKey(ev));
			const int key_modifier = RmlSDL::GetKeyModifierState();
			const float native_dp_ratio = GetDisplayScale();

			// See if we have any global shortcuts that take priority over the context.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, true))
				break;
			// Otherwise, hand the event over to the context by calling the input handler as normal.
			if (!RmlSDL::InputEventHandler(context, data->window, ev))
				break;
			// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, native_dp_ratio, false))
				break;
		}
		break;
		default: break;
		}

		if (propagate_event)
			RmlSDL::InputEventHandler(context, data->window, ev);

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

    data->command_buffer = SDL_AcquireGPUCommandBuffer(data->device);
    if (!data->command_buffer)
    {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

	SDL_GPUTexture* swapchain_texture;
	uint32_t width;
	uint32_t height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(data->command_buffer, data->window, &swapchain_texture, &width, &height))
    {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to acquire swapchain texture: %s", SDL_GetError());
        return;
    }

    if (!swapchain_texture || !width || !height)
    {
        // Not an error. Happens on minimize
        SDL_CancelGPUCommandBuffer(data->command_buffer);
        return;
    }

	// Do your normal draw operations (make sure you clear the swapchain texture)
    SDL_GPUColorTargetInfo color_info{};
    color_info.texture = swapchain_texture;
	color_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(data->command_buffer, &color_info, 1, nullptr);
	SDL_EndGPURenderPass(render_pass);

	data->render_interface.BeginFrame(data->command_buffer, swapchain_texture, width, height);
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.EndFrame();

    SDL_SubmitGPUCommandBuffer(data->command_buffer);
    data->command_buffer = nullptr;
}