#include "RmlUi_Backend.h"
#include "RmlUi_Platform_Wayland.h"
#include "RmlUi_Renderer_GL3.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <EGL/egl.h>
#include <libdecor.h>
#include <wayland-egl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

#include <cursor-shape-v1-client-protocol.h>

static constexpr int MinimumWindowWidth = 1;
static constexpr int MinimumWindowHeight = 1;
using Clock = std::chrono::steady_clock;

#ifdef WL_KEYBOARD_KEY_STATE_REPEATED_SINCE_VERSION
static constexpr uint32_t MaxSeatVersion = WL_KEYBOARD_KEY_STATE_REPEATED_SINCE_VERSION;
#else
static constexpr uint32_t MaxSeatVersion = 9;
#endif

struct KeyboardRepeatState {
	bool active = false;
	xkb_keycode_t keycode = 0;
	int32_t rate = 0;
	int32_t delay = 0;
	Clock::time_point next_time;
	Clock::duration interval{};

	void Stop()
	{
		active = false;
		keycode = 0;
	}
};

struct BackendData {
	RmlWayland::Globals globals;
	Rml::UniquePtr<SystemInterface_Wayland> system_interface;
	RmlWayland::KeyboardState keyboard_state;
	KeyboardRepeatState repeat_state;
	Rml::UniquePtr<RenderInterface_GL3> render_interface;

	wl_display* display = nullptr;
	wl_registry* registry = nullptr;
	wl_surface* surface = nullptr;
	wl_surface* cursor_surface = nullptr;
	libdecor* decor_context = nullptr;
	libdecor_frame* decor_frame = nullptr;
	wl_egl_window* egl_window = nullptr;
	wl_pointer* pointer = nullptr;
	wl_keyboard* keyboard = nullptr;

	EGLDisplay egl_display = EGL_NO_DISPLAY;
	EGLConfig egl_config = nullptr;
	EGLSurface egl_surface = EGL_NO_SURFACE;
	EGLContext egl_context = EGL_NO_CONTEXT;

	Rml::Context* context = nullptr;
	KeyDownCallback key_down_callback = nullptr;

	int width = 0;
	int height = 0;
	bool configured = false;
	bool running = true;
	bool context_dimensions_dirty = true;
};

static Rml::UniquePtr<BackendData> data;

static void UpdateWindowSize(int width, int height)
{
	RMLUI_ASSERT(data);

	data->width = Rml::Math::Max(width, MinimumWindowWidth);
	data->height = Rml::Math::Max(height, MinimumWindowHeight);

	if (data->egl_window)
		wl_egl_window_resize(data->egl_window, data->width, data->height, 0, 0);

	if (data->render_interface)
		data->render_interface->SetViewport(data->width, data->height);

	data->context_dimensions_dirty = true;
}

static void RegistryHandleGlobal(void* user_data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
	auto* globals = static_cast<RmlWayland::Globals*>(user_data);

	if (std::strcmp(interface, wl_compositor_interface.name) == 0)
		globals->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
	else if (std::strcmp(interface, wl_shm_interface.name) == 0)
		globals->shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
	else if (std::strcmp(interface, wl_seat_interface.name) == 0)
		globals->seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, MaxSeatVersion)));
	else if (std::strcmp(interface, wp_cursor_shape_manager_v1_interface.name) == 0)
		globals->cursor_shape_manager =
			static_cast<wp_cursor_shape_manager_v1*>(wl_registry_bind(registry, name, &wp_cursor_shape_manager_v1_interface, 1));
}

static void RegistryHandleGlobalRemove(void*, wl_registry*, uint32_t) {}

static const wl_registry_listener registry_listener = {
	RegistryHandleGlobal,
	RegistryHandleGlobalRemove,
};

static void LibdecorHandleError(libdecor*, libdecor_error error, const char* message)
{
	Rml::Log::Message(Rml::Log::LT_ERROR, "libdecor error (%d): %s", int(error), message ? message : "Unknown error");
	data->running = false;
}

static libdecor_interface libdecor_listener = [] {
	libdecor_interface result {};
	result.error = LibdecorHandleError;
	return result;
}();

static void LibdecorFrameHandleConfigure(libdecor_frame* frame, libdecor_configuration* configuration, void*)
{
	int width = 0;
	int height = 0;
	if (libdecor_configuration_get_content_size(configuration, frame, &width, &height))
		UpdateWindowSize(width, height);

	libdecor_state* state = libdecor_state_new(data->width, data->height);
	libdecor_frame_commit(frame, state, configuration);
	libdecor_state_free(state);
	data->configured = true;
}

static void LibdecorFrameHandleClose(libdecor_frame*, void*)
{
	data->running = false;
}

static void LibdecorFrameHandleCommit(libdecor_frame*, void*)
{
	wl_surface_commit(data->surface);
}

static void LibdecorFrameHandleDismissPopup(libdecor_frame*, const char*, void*) {}

static libdecor_frame_interface libdecor_frame_listener = [] {
	libdecor_frame_interface result {};
	result.configure = LibdecorFrameHandleConfigure;
	result.close = LibdecorFrameHandleClose;
	result.commit = LibdecorFrameHandleCommit;
	result.dismiss_popup = LibdecorFrameHandleDismissPopup;
	return result;
}();

static void PointerHandleEnter(void*, wl_pointer*, uint32_t serial, wl_surface*, wl_fixed_t sx, wl_fixed_t sy)
{
	data->system_interface->SetPointerSerial(serial);
	data->system_interface->SetMouseCursor("arrow");
	if (data->context)
		data->context->ProcessMouseMove(wl_fixed_to_int(sx), wl_fixed_to_int(sy), data->keyboard_state.modifiers);
}

static void PointerHandleLeave(void*, wl_pointer*, uint32_t, wl_surface*)
{
	data->system_interface->ClearPointerSerial();
	if (data->context)
		data->context->ProcessMouseLeave();
}

static void PointerHandleMotion(void*, wl_pointer*, uint32_t, wl_fixed_t sx, wl_fixed_t sy)
{
	if (data->context)
		data->context->ProcessMouseMove(wl_fixed_to_int(sx), wl_fixed_to_int(sy), data->keyboard_state.modifiers);
}

static void PointerHandleButton(void*, wl_pointer*, uint32_t, uint32_t, uint32_t button, uint32_t state)
{
	if (!data->context)
		return;

	const int mouse_button = RmlWayland::ConvertMouseButton(button);
	if (mouse_button < 0)
		return;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED)
		data->context->ProcessMouseButtonDown(mouse_button, data->keyboard_state.modifiers);
	else
		data->context->ProcessMouseButtonUp(mouse_button, data->keyboard_state.modifiers);
}

static void PointerHandleAxis(void*, wl_pointer*, uint32_t, uint32_t axis, wl_fixed_t value)
{
	if (data->context && axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
		data->context->ProcessMouseWheel(float(wl_fixed_to_double(value)) / 10.f, data->keyboard_state.modifiers);
}

static void PointerHandleFrame(void*, wl_pointer*) {}
static void PointerHandleAxisSource(void*, wl_pointer*, uint32_t) {}
static void PointerHandleAxisStop(void*, wl_pointer*, uint32_t, uint32_t) {}
static void PointerHandleAxisDiscrete(void*, wl_pointer*, uint32_t, int32_t) {}
static void PointerHandleAxisValue120(void*, wl_pointer*, uint32_t, int32_t) {}
static void PointerHandleAxisRelativeDirection(void*, wl_pointer*, uint32_t, uint32_t) {}

static const wl_pointer_listener pointer_listener = {
	PointerHandleEnter,
	PointerHandleLeave,
	PointerHandleMotion,
	PointerHandleButton,
	PointerHandleAxis,
	PointerHandleFrame,
	PointerHandleAxisSource,
	PointerHandleAxisStop,
	PointerHandleAxisDiscrete,
	PointerHandleAxisValue120,
	PointerHandleAxisRelativeDirection,
};

static void KeyboardHandleKeymap(void*, wl_keyboard*, uint32_t format, int32_t fd, uint32_t size)
{
	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	{
		close(fd);
		return;
	}

	void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (mapped == MAP_FAILED)
		return;

	data->keyboard_state.SetKeymapFromString(static_cast<const char*>(mapped));
	data->repeat_state.Stop();
	munmap(mapped, size);
}

static void KeyboardHandleEnter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {}

static void KeyboardHandleLeave(void*, wl_keyboard*, uint32_t, wl_surface*)
{
	data->repeat_state.Stop();
	data->keyboard_state.Reset();
}

static bool IsTextInputCodepoint(uint32_t codepoint)
{
	return codepoint >= 0x20 && !(codepoint >= 0x7f && codepoint <= 0x9f);
}

static Clock::duration GetKeyRepeatInterval(int32_t rate)
{
	RMLUI_ASSERT(rate > 0);
	const Clock::duration interval = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / double(rate)));
	return std::max(Clock::duration{1}, interval);
}

static void SubmitKeyDown(xkb_keycode_t keycode)
{
	if (!data->context || !data->keyboard_state.state)
		return;

	const xkb_keysym_t sym = xkb_state_key_get_one_sym(data->keyboard_state.state, keycode);
	const Rml::Input::KeyIdentifier rml_key = RmlWayland::ConvertKeySym(sym);
	const int modifiers = data->keyboard_state.modifiers;
	const float native_dp_ratio = 1.f;

	if (data->key_down_callback && !data->key_down_callback(data->context, rml_key, modifiers, native_dp_ratio, true))
		return;

	bool propagates = true;
	if (rml_key != Rml::Input::KI_UNKNOWN)
		propagates = data->context->ProcessKeyDown(rml_key, modifiers);

	const uint32_t codepoint = xkb_state_key_get_utf32(data->keyboard_state.state, keycode);
	if (rml_key == Rml::Input::KI_RETURN || rml_key == Rml::Input::KI_NUMPADENTER)
		propagates &= data->context->ProcessTextInput('\n');
	else if (IsTextInputCodepoint(codepoint) && !(modifiers & Rml::Input::KM_CTRL))
		propagates &= data->context->ProcessTextInput(Rml::Character(codepoint));

	if (propagates && data->key_down_callback)
		data->key_down_callback(data->context, rml_key, modifiers, native_dp_ratio, false);
}

static void StartKeyRepeat(xkb_keycode_t keycode)
{
	KeyboardRepeatState& repeat = data->repeat_state;
	repeat.Stop();

	// Positive repeat rates ask the client to schedule repeats. Version 10 compositor repeats arrive with rate zero instead.
	if (repeat.rate <= 0 || !data->keyboard_state.keymap || !xkb_keymap_key_repeats(data->keyboard_state.keymap, keycode))
		return;

	repeat.active = true;
	repeat.keycode = keycode;
	repeat.interval = GetKeyRepeatInterval(repeat.rate);
	repeat.next_time = Clock::now() + std::chrono::milliseconds(std::max(0, repeat.delay));
}

static void ProcessKeyRepeats()
{
	KeyboardRepeatState& repeat = data->repeat_state;
	if (!repeat.active)
		return;

	const Clock::time_point now = Clock::now();
	if (now < repeat.next_time)
		return;

	int repeated_keys = 0;
	do
	{
		SubmitKeyDown(repeat.keycode);
		repeat.next_time += repeat.interval;
		repeated_keys += 1;
	} while (repeat.active && repeat.next_time <= now && repeated_keys < 32);

	if (repeated_keys == 32)
		repeat.next_time = now + repeat.interval;
}

static double GetKeyRepeatTimeout(double timeout_seconds)
{
	const KeyboardRepeatState& repeat = data->repeat_state;
	if (!repeat.active)
		return timeout_seconds;

	const Clock::time_point now = Clock::now();
	if (now >= repeat.next_time)
		return 0.0;

	const double repeat_timeout = std::chrono::duration<double>(repeat.next_time - now).count();
	return Rml::Math::Min(timeout_seconds, repeat_timeout);
}

static void KeyboardHandleKey(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state)
{
	if (!data->context || !data->keyboard_state.state)
		return;

	const xkb_keycode_t keycode = key + 8;

	switch (state)
	{
	case WL_KEYBOARD_KEY_STATE_PRESSED:
		SubmitKeyDown(keycode);
		StartKeyRepeat(keycode);
		break;
#ifdef WL_KEYBOARD_KEY_STATE_REPEATED_SINCE_VERSION
	case WL_KEYBOARD_KEY_STATE_REPEATED:
		// Version 10 compositors may send repeats directly when repeat_info disables client-side repetition.
		SubmitKeyDown(keycode);
		break;
#endif
	case WL_KEYBOARD_KEY_STATE_RELEASED:
	{
		if (data->repeat_state.active && data->repeat_state.keycode == keycode)
			data->repeat_state.Stop();

		const xkb_keysym_t sym = xkb_state_key_get_one_sym(data->keyboard_state.state, keycode);
		const Rml::Input::KeyIdentifier rml_key = RmlWayland::ConvertKeySym(sym);
		const int modifiers = data->keyboard_state.modifiers;
		data->context->ProcessKeyUp(rml_key, modifiers);
		break;
	}
	default: break;
	}
}

static void KeyboardHandleModifiers(void*, wl_keyboard*, uint32_t, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
{
	data->keyboard_state.UpdateModifiers(depressed, latched, locked, group);
}

static void KeyboardHandleRepeatInfo(void*, wl_keyboard*, int32_t rate, int32_t delay)
{
	KeyboardRepeatState& repeat = data->repeat_state;
	repeat.rate = std::max(0, rate);
	repeat.delay = std::max(0, delay);

	if (repeat.rate == 0)
	{
		repeat.Stop();
		repeat.interval = {};
		return;
	}

	repeat.interval = GetKeyRepeatInterval(repeat.rate);
	if (repeat.active)
		repeat.next_time = Clock::now() + std::chrono::milliseconds(repeat.delay);
}

static const wl_keyboard_listener keyboard_listener = {
	KeyboardHandleKeymap,
	KeyboardHandleEnter,
	KeyboardHandleLeave,
	KeyboardHandleKey,
	KeyboardHandleModifiers,
	KeyboardHandleRepeatInfo,
};

static void SeatHandleCapabilities(void*, wl_seat* seat, uint32_t capabilities)
{
	if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !data->pointer)
	{
		data->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(data->pointer, &pointer_listener, nullptr);
		data->system_interface->SetPointer(data->pointer);
	}
	else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && data->pointer)
	{
		data->system_interface->SetPointer(nullptr);
		wl_pointer_destroy(data->pointer);
		data->pointer = nullptr;
	}

	if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !data->keyboard)
	{
		data->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(data->keyboard, &keyboard_listener, nullptr);
	}
	else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && data->keyboard)
	{
		data->repeat_state.Stop();
		data->keyboard_state.Reset();
		wl_keyboard_destroy(data->keyboard);
		data->keyboard = nullptr;
	}
}

static void SeatHandleName(void*, wl_seat*, const char*) {}

static const wl_seat_listener seat_listener = {
	SeatHandleCapabilities,
	SeatHandleName,
};

static bool InitializeWayland(const char* window_name, int width, int height, bool allow_resize)
{
	wl_display* display = wl_display_connect(nullptr);
	if (!display)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to connect to Wayland display.");
		return false;
	}

	auto new_data = Rml::MakeUnique<BackendData>();
	new_data->display = display;
	new_data->registry = wl_display_get_registry(display);
	wl_registry_add_listener(new_data->registry, &registry_listener, &new_data->globals);

	data = std::move(new_data);
	wl_display_roundtrip(display);

	if (!data->globals.compositor || !data->globals.shm)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Required Wayland globals are not available.");
		return false;
	}

	data->system_interface = Rml::MakeUnique<SystemInterface_Wayland>(display, data->globals.shm, data->globals.cursor_shape_manager);

	if (data->globals.seat)
		wl_seat_add_listener(data->globals.seat, &seat_listener, nullptr);

	data->surface = wl_compositor_create_surface(data->globals.compositor);
	data->cursor_surface = wl_compositor_create_surface(data->globals.compositor);
	data->system_interface->SetCursorSurface(data->cursor_surface);

	data->decor_context = libdecor_new(display, &libdecor_listener);
	if (!data->decor_context)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create libdecor context.");
		return false;
	}

	data->decor_frame = libdecor_decorate(data->decor_context, data->surface, &libdecor_frame_listener, nullptr);
	if (!data->decor_frame)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create libdecor frame.");
		return false;
	}

	libdecor_frame_set_title(data->decor_frame, window_name);
	if (!allow_resize)
	{
		libdecor_frame_unset_capabilities(data->decor_frame, LIBDECOR_ACTION_RESIZE);
		libdecor_frame_set_min_content_size(data->decor_frame, width, height);
		libdecor_frame_set_max_content_size(data->decor_frame, width, height);
	}

	UpdateWindowSize(width, height);
	libdecor_frame_map(data->decor_frame);

	while (!data->configured && data->running && libdecor_dispatch(data->decor_context, -1) >= 0) {}

	return data->configured && data->running;
}

static bool InitializeEGL()
{
	data->egl_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(data->display));
	if (data->egl_display == EGL_NO_DISPLAY)
		return false;

	if (!eglInitialize(data->egl_display, nullptr, nullptr))
		return false;

	if (!eglBindAPI(EGL_OPENGL_API))
		return false;

	const EGLint config_attributes[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE,
	};

	EGLint config_count = 0;
	if (!eglChooseConfig(data->egl_display, config_attributes, &data->egl_config, 1, &config_count) || config_count == 0)
		return false;

	const EGLint context_attributes[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 3,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
		EGL_NONE,
	};

	data->egl_context = eglCreateContext(data->egl_display, data->egl_config, EGL_NO_CONTEXT, context_attributes);
	if (data->egl_context == EGL_NO_CONTEXT)
		return false;

	data->egl_window = wl_egl_window_create(data->surface, data->width, data->height);
	if (!data->egl_window)
		return false;

	data->egl_surface = eglCreateWindowSurface(data->egl_display, data->egl_config, reinterpret_cast<EGLNativeWindowType>(data->egl_window), nullptr);
	if (data->egl_surface == EGL_NO_SURFACE)
		return false;

	if (!eglMakeCurrent(data->egl_display, data->egl_surface, data->egl_surface, data->egl_context))
		return false;

	eglSwapInterval(data->egl_display, 1);
	return true;
}

static bool DispatchWaylandEvents(double timeout_seconds)
{
	const int timeout_ms = int(std::ceil(timeout_seconds * 1000.0));
	if (libdecor_dispatch(data->decor_context, timeout_ms) < 0)
		return false;

	while (wl_display_dispatch_pending(data->display) > 0) {}
	return wl_display_get_error(data->display) == 0;
}

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	const auto shutdown_on_failure = [] {
		if (data)
			Backend::Shutdown();
		return false;
	};

	if (!InitializeWayland(window_name, width, height, allow_resize))
		return shutdown_on_failure();

	if (!InitializeEGL())
		return shutdown_on_failure();

	Rml::String renderer_message;
	if (!RmlGL3::Initialize(&renderer_message))
		return shutdown_on_failure();

	data->render_interface = Rml::MakeUnique<RenderInterface_GL3>();
	if (!data->render_interface || !*data->render_interface)
		return shutdown_on_failure();

	data->render_interface->SetViewport(data->width, data->height);
	data->system_interface->LogMessage(Rml::Log::LT_INFO, renderer_message);
	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	data->render_interface.reset();
	RmlGL3::Shutdown();
	data->system_interface.reset();

	if (data->egl_display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(data->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (data->egl_surface != EGL_NO_SURFACE)
			eglDestroySurface(data->egl_display, data->egl_surface);
		if (data->egl_context != EGL_NO_CONTEXT)
			eglDestroyContext(data->egl_display, data->egl_context);
		eglTerminate(data->egl_display);
	}

	if (data->egl_window)
		wl_egl_window_destroy(data->egl_window);
	if (data->keyboard)
		wl_keyboard_destroy(data->keyboard);
	if (data->pointer)
		wl_pointer_destroy(data->pointer);
	if (data->decor_frame)
		libdecor_frame_unref(data->decor_frame);
	if (data->decor_context)
		libdecor_unref(data->decor_context);
	if (data->cursor_surface)
		wl_surface_destroy(data->cursor_surface);
	if (data->surface)
		wl_surface_destroy(data->surface);
	if (data->globals.seat)
		wl_seat_destroy(data->globals.seat);
	if (data->globals.shm)
		wl_shm_destroy(data->globals.shm);
	if (data->globals.cursor_shape_manager)
		wp_cursor_shape_manager_v1_destroy(data->globals.cursor_shape_manager);
	if (data->globals.compositor)
		wl_compositor_destroy(data->globals.compositor);
	if (data->registry)
		wl_registry_destroy(data->registry);
	if (data->display)
		wl_display_disconnect(data->display);

	data.reset();
}

Rml::SystemInterface* Backend::GetSystemInterface()
{
	RMLUI_ASSERT(data);
	return data->system_interface.get();
}

Rml::RenderInterface* Backend::GetRenderInterface()
{
	RMLUI_ASSERT(data);
	return data->render_interface.get();
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

	if (data->context_dimensions_dirty)
	{
		data->context_dimensions_dirty = false;
		context->SetDimensions({data->width, data->height});
		context->SetDensityIndependentPixelRatio(1.f);
	}

	data->context = context;
	data->key_down_callback = key_down_callback;

	double timeout_seconds = power_save ? Rml::Math::Min(context->GetNextUpdateDelay(), 10.0) : 0.0;
	timeout_seconds = GetKeyRepeatTimeout(timeout_seconds);
	if (!DispatchWaylandEvents(timeout_seconds))
		data->running = false;

	ProcessKeyRepeats();

	data->context = nullptr;
	data->key_down_callback = nullptr;

	return data->running;
}

void Backend::RequestExit()
{
	RMLUI_ASSERT(data);
	data->running = false;
}

void Backend::BeginFrame()
{
	RMLUI_ASSERT(data && data->render_interface);
	data->render_interface->Clear();
	data->render_interface->BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data && data->render_interface);
	data->render_interface->EndFrame();
	eglSwapBuffers(data->egl_display, data->egl_surface);
}
