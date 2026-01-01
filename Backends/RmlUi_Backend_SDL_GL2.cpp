#include "RmlUi_Backend.h"
#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_GL2.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>

#if SDL_MAJOR_VERSION >= 3
	#include <SDL3_image/SDL_image.h>
#else
	#include <SDL_image.h>
#endif

#if SDL_MAJOR_VERSION == 2 && !(SDL_VIDEO_RENDER_OGL)
	#error "Only the OpenGL SDL backend is supported."
#endif

/**
    Custom render interface example for the SDL/GL2 backend.

    Overloads the OpenGL2 render interface to load textures through SDL_image's built-in texture loading functionality.
 */
class RenderInterface_GL2_SDL : public RenderInterface_GL2 {
public:
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

#if SDL_MAJOR_VERSION >= 3
		auto CreateSurface = [&]() { return IMG_LoadTyped_IO(SDL_IOFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str()); };
		auto GetSurfaceFormat = [](SDL_Surface* surface) { return surface->format; };
		auto ConvertSurface = [](SDL_Surface* surface, SDL_PixelFormat format) { return SDL_ConvertSurface(surface, format); };
		auto DestroySurface = [](SDL_Surface* surface) { SDL_DestroySurface(surface); };
#else
		auto CreateSurface = [&]() { return IMG_LoadTyped_RW(SDL_RWFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str()); };
		auto GetSurfaceFormat = [](SDL_Surface* surface) { return surface->format->format; };
		auto ConvertSurface = [](SDL_Surface* surface, Uint32 format) { return SDL_ConvertSurfaceFormat(surface, format, 0); };
		auto DestroySurface = [](SDL_Surface* surface) { SDL_FreeSurface(surface); };
#endif

		SDL_Surface* surface = CreateSurface();
		if (!surface)
			return {};

		texture_dimensions = {surface->w, surface->h};

		if (GetSurfaceFormat(surface) != SDL_PIXELFORMAT_RGBA32)
		{
			// Ensure correct format for premultiplied alpha conversion and GenerateTexture below.
			SDL_Surface* converted_surface = ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
			DestroySurface(surface);
			if (!converted_surface)
				return {};

			surface = converted_surface;
		}

		// Convert colors to premultiplied alpha, which is necessary for correct alpha compositing.
		const size_t pixels_byte_size = surface->w * surface->h * 4;
		byte* pixels = static_cast<byte*>(surface->pixels);
		for (size_t i = 0; i < pixels_byte_size; i += 4)
		{
			const byte alpha = pixels[i + 3];
			for (size_t j = 0; j < 3; ++j)
				pixels[i + j] = byte(int(pixels[i + j]) * int(alpha) / 255);
		}

		Rml::TextureHandle texture_handle = RenderInterface_GL2::GenerateTexture({pixels, pixels_byte_size}, texture_dimensions);

		DestroySurface(surface);

		return texture_handle;
	}
};

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_SDL system_interface;
	RenderInterface_GL2_SDL render_interface;

	SDL_Window* window = nullptr;
	SDL_GLContext glcontext = nullptr;

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

	// Request stencil buffer of at least 8-bit size to supporting clipping on transformed elements.
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable MSAA for better-looking visuals, especially when transforms are applied.
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

#if SDL_MAJOR_VERSION >= 3
	auto CreateWindow = [&]() {
		const float window_size_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, window_name);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, int(width * window_size_scale));
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, int(height * window_size_scale));
		SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
		SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, allow_resize);
		SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
		SDL_Window* window = SDL_CreateWindowWithProperties(props);
		SDL_DestroyProperties(props);
		return window;
	};
#else
	auto CreateWindow = [&]() {
		const Uint32 window_flags = (SDL_WINDOW_OPENGL | (allow_resize ? SDL_WINDOW_RESIZABLE : 0));
		SDL_Window* window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
		// SDL2 implicitly activates text input on window creation. Turn it off for now, it will be activated again e.g. when focusing a text input
		// field.
		SDL_StopTextInput();
		return window;
	};
	// Enable linear filtering, SDL 3 already defaults to it.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
#endif

	SDL_Window* window = CreateWindow();
	if (!window)
	{
		// Try again on low-quality settings.
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		window = CreateWindow();
		if (!window)
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "SDL error on create window: %s", SDL_GetError());
			return false;
		}
	}

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glcontext);
	SDL_GL_SetSwapInterval(1);

	data = Rml::MakeUnique<BackendData>();

	data->window = window;
	data->glcontext = glcontext;

	data->system_interface.SetWindow(window);
	data->render_interface.SetViewport(width, height);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

#if SDL_MAJOR_VERSION >= 3
	SDL_GL_DestroyContext(data->glcontext);
#else
	SDL_GL_DeleteContext(data->glcontext);
#endif

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
	#define RMLSDL_WINDOW_EVENTS_BEGIN
	#define RMLSDL_WINDOW_EVENTS_END
	auto GetKey = [](const SDL_Event& event) { return event.key.key; };
	auto GetDisplayScale = []() { return SDL_GetWindowDisplayScale(data->window); };
	constexpr auto event_quit = SDL_EVENT_QUIT;
	constexpr auto event_key_down = SDL_EVENT_KEY_DOWN;
	constexpr auto event_window_size_changed = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
	bool has_event = false;
#else
	#define RMLSDL_WINDOW_EVENTS_BEGIN \
	case SDL_WINDOWEVENT:              \
	{                                  \
		switch (ev.window.event)       \
		{
	#define RMLSDL_WINDOW_EVENTS_END \
		}                            \
		}                            \
		break;
	auto GetKey = [](const SDL_Event& event) { return event.key.keysym.sym; };
	auto GetDisplayScale = []() { return 1.f; };
	constexpr auto event_quit = SDL_QUIT;
	constexpr auto event_key_down = SDL_KEYDOWN;
	constexpr auto event_window_size_changed = SDL_WINDOWEVENT_SIZE_CHANGED;
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

			RMLSDL_WINDOW_EVENTS_BEGIN

		case event_window_size_changed:
		{
			Rml::Vector2i dimensions = {ev.window.data1, ev.window.data2};
			data->render_interface.SetViewport(dimensions.x, dimensions.y);
		}
		break;

			RMLSDL_WINDOW_EVENTS_END

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

	data->render_interface.Clear();
	data->render_interface.BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);

	data->render_interface.EndFrame();
	SDL_GL_SwapWindow(data->window);
}
