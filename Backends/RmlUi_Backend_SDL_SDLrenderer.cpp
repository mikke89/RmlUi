#include "RmlUi_Backend.h"
#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_SDL.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	BackendData(SDL_Renderer* renderer) : render_interface(renderer) {}

	SystemInterface_SDL system_interface;
	RenderInterface_SDL render_interface;

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	bool running = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

#if SDL_MAJOR_VERSION >= 3
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
		return false;
#else
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
		return false;
#endif

	// Submit click events when focusing the window.
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	// Touch events are handled natively, no need to generate synthetic mouse events for touch devices.
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

#if defined RMLUI_BACKEND_SIMULATE_TOUCH
	// Simulate touch events from mouse events for testing touch behavior on a desktop machine.
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
#endif

#if SDL_MAJOR_VERSION >= 3
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
#else
	const Uint32 window_flags = (allow_resize ? SDL_WINDOW_RESIZABLE : 0);
	SDL_Window* window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
	// SDL2 implicitly activates text input on window creation. Turn it off for now, it will be activated again e.g. when focusing a text input field.
	SDL_StopTextInput();
#endif

	if (!window)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "SDL error on create window: %s\n", SDL_GetError());
		return false;
	}

	// Force a specific SDL renderer
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d12");
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");

#if SDL_MAJOR_VERSION >= 3
	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
#else
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif

	if (!renderer)
		return false;

	data = Rml::MakeUnique<BackendData>(renderer);
	data->window = window;
	data->renderer = renderer;
	data->system_interface.SetWindow(window);

	const char* renderer_name = nullptr;
	Rml::String available_renderers;

#if SDL_MAJOR_VERSION >= 3
	SDL_SetRenderVSync(renderer, 1);
	renderer_name = SDL_GetRendererName(renderer);
	for (int i = 0; i < SDL_GetNumRenderDrivers(); i++)
	{
		if (i > 0)
			available_renderers += ", ";
		available_renderers += SDL_GetRenderDriver(i);
	}
#else
	SDL_RendererInfo renderer_info;
	if (SDL_GetRendererInfo(renderer, &renderer_info) == 0)
		renderer_name = renderer_info.name;
#endif

	if (!available_renderers.empty())
		data->system_interface.LogMessage(Rml::Log::LT_INFO, Rml::CreateString("Available SDL renderers: %s", available_renderers.c_str()));
	if (renderer_name)
		data->system_interface.LogMessage(Rml::Log::LT_INFO, Rml::CreateString("Using SDL renderer: %s", renderer_name));

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	SDL_DestroyRenderer(data->renderer);
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

#if SDL_MAJOR_VERSION >= 3
	auto GetKey = [](const SDL_Event& event) { return event.key.key; };
	auto GetDisplayScale = []() { return SDL_GetWindowDisplayScale(data->window); };
	constexpr auto event_quit = SDL_EVENT_QUIT;
	constexpr auto event_key_down = SDL_EVENT_KEY_DOWN;
	bool has_event = false;
#else
	auto GetKey = [](const SDL_Event& event) { return event.key.keysym.sym; };
	auto GetDisplayScale = []() { return 1.f; };
	constexpr auto event_quit = SDL_QUIT;
	constexpr auto event_key_down = SDL_KEYDOWN;
	int has_event = 0;
#endif

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
	data->render_interface.BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.EndFrame();
	SDL_RenderPresent(data->renderer);
}
