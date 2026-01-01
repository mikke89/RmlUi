#include "RmlUi_Platform_SDL.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/SystemInterface.h>

static Rml::TouchList TouchEventToTouchList(SDL_Event& ev, Rml::Context* context, SDL_FingerID finger_id)
{
	const Rml::Vector2f position = Rml::Vector2f{ev.tfinger.x, ev.tfinger.y} * Rml::Vector2f{context->GetDimensions()};
	return {Rml::Touch{static_cast<Rml::TouchId>(finger_id), position}};
}

SystemInterface_SDL::SystemInterface_SDL()
{
#if SDL_MAJOR_VERSION >= 3
	cursor_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	cursor_move = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
	cursor_pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
	cursor_resize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
	cursor_cross = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	cursor_text = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
	cursor_unavailable = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);
#else
	cursor_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	cursor_move = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	cursor_pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	cursor_resize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	cursor_cross = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	cursor_text = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	cursor_unavailable = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
#endif
}

SystemInterface_SDL::~SystemInterface_SDL()
{
#if SDL_MAJOR_VERSION >= 3
	auto DestroyCursor = [](SDL_Cursor* cursor) { SDL_DestroyCursor(cursor); };
#else
	auto DestroyCursor = [](SDL_Cursor* cursor) { SDL_FreeCursor(cursor); };
#endif

	DestroyCursor(cursor_default);
	DestroyCursor(cursor_move);
	DestroyCursor(cursor_pointer);
	DestroyCursor(cursor_resize);
	DestroyCursor(cursor_cross);
	DestroyCursor(cursor_text);
	DestroyCursor(cursor_unavailable);
}

void SystemInterface_SDL::SetWindow(SDL_Window* in_window)
{
	window = in_window;
}

double SystemInterface_SDL::GetElapsedTime()
{
	static const Uint64 start = SDL_GetPerformanceCounter();
	static const double frequency = double(SDL_GetPerformanceFrequency());
	return double(SDL_GetPerformanceCounter() - start) / frequency;
}

void SystemInterface_SDL::SetMouseCursor(const Rml::String& cursor_name)
{
	SDL_Cursor* cursor = nullptr;

	if (cursor_name.empty() || cursor_name == "arrow")
		cursor = cursor_default;
	else if (cursor_name == "move")
		cursor = cursor_move;
	else if (cursor_name == "pointer")
		cursor = cursor_pointer;
	else if (cursor_name == "resize")
		cursor = cursor_resize;
	else if (cursor_name == "cross")
		cursor = cursor_cross;
	else if (cursor_name == "text")
		cursor = cursor_text;
	else if (cursor_name == "unavailable")
		cursor = cursor_unavailable;
	else if (Rml::StringUtilities::StartsWith(cursor_name, "rmlui-scroll"))
		cursor = cursor_move;

	if (cursor)
		SDL_SetCursor(cursor);
}

void SystemInterface_SDL::SetClipboardText(const Rml::String& text)
{
	SDL_SetClipboardText(text.c_str());
}

void SystemInterface_SDL::GetClipboardText(Rml::String& text)
{
	char* raw_text = SDL_GetClipboardText();
	text = Rml::String(raw_text);
	SDL_free(raw_text);
}

void SystemInterface_SDL::ActivateKeyboard(Rml::Vector2f caret_position, float line_height)
{
	if (window)
	{
#if SDL_MAJOR_VERSION >= 3
		const SDL_Rect rect = {int(caret_position.x), int(caret_position.y), 1, int(line_height)};
		SDL_SetTextInputArea(window, &rect, 0);
		SDL_StartTextInput(window);
#else
		(void)caret_position;
		(void)line_height;
		SDL_StartTextInput();
#endif
	}
}

void SystemInterface_SDL::DeactivateKeyboard()
{
	if (window)
	{
#if SDL_MAJOR_VERSION >= 3
		SDL_StopTextInput(window);
#else
		SDL_StopTextInput();
#endif
	}
}

bool RmlSDL::InputEventHandler(Rml::Context* context, SDL_Window* window, SDL_Event& ev)
{
#if SDL_MAJOR_VERSION >= 3
	#define RMLSDL_WINDOW_EVENTS_BEGIN
	#define RMLSDL_WINDOW_EVENTS_END
	auto GetKey = [](const SDL_Event& event) { return event.key.key; };
	auto GetPixelDensity = [](SDL_Window* target_window) { return SDL_GetWindowPixelDensity(target_window); };
	auto GetFingerId = [](const SDL_Event& event) { return event.tfinger.fingerID; };
	constexpr auto event_mouse_motion = SDL_EVENT_MOUSE_MOTION;
	constexpr auto event_mouse_down = SDL_EVENT_MOUSE_BUTTON_DOWN;
	constexpr auto event_mouse_up = SDL_EVENT_MOUSE_BUTTON_UP;
	constexpr auto event_mouse_wheel = SDL_EVENT_MOUSE_WHEEL;
	constexpr auto event_key_down = SDL_EVENT_KEY_DOWN;
	constexpr auto event_key_up = SDL_EVENT_KEY_UP;
	constexpr auto event_text_input = SDL_EVENT_TEXT_INPUT;
	constexpr auto event_window_size_changed = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
	constexpr auto event_window_leave = SDL_EVENT_WINDOW_MOUSE_LEAVE;
	constexpr auto event_finger_down = SDL_EVENT_FINGER_DOWN;
	constexpr auto event_finger_up = SDL_EVENT_FINGER_UP;
	constexpr auto event_finger_motion = SDL_EVENT_FINGER_MOTION;

	constexpr auto rmlsdl_true = true;
	constexpr auto rmlsdl_false = false;
#else
	(void)window;
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
	auto GetPixelDensity = [](SDL_Window* /*target_window*/) { return 1.f; };
	auto GetFingerId = [](const SDL_Event& event) { return event.tfinger.fingerId; };
	constexpr auto event_mouse_motion = SDL_MOUSEMOTION;
	constexpr auto event_mouse_down = SDL_MOUSEBUTTONDOWN;
	constexpr auto event_mouse_up = SDL_MOUSEBUTTONUP;
	constexpr auto event_mouse_wheel = SDL_MOUSEWHEEL;
	constexpr auto event_key_down = SDL_KEYDOWN;
	constexpr auto event_key_up = SDL_KEYUP;
	constexpr auto event_text_input = SDL_TEXTINPUT;
	constexpr auto event_window_size_changed = SDL_WINDOWEVENT_SIZE_CHANGED;
	constexpr auto event_window_leave = SDL_WINDOWEVENT_LEAVE;
	constexpr auto event_finger_down = SDL_FINGERDOWN;
	constexpr auto event_finger_up = SDL_FINGERUP;
	constexpr auto event_finger_motion = SDL_FINGERMOTION;

	constexpr auto rmlsdl_true = SDL_TRUE;
	constexpr auto rmlsdl_false = SDL_FALSE;
#endif

	bool result = true;

	switch (ev.type)
	{
#ifndef RMLUI_BACKEND_SIMULATE_TOUCH
	case event_mouse_motion:
	{
		const float pixel_density = GetPixelDensity(window);
		result = context->ProcessMouseMove(int(ev.motion.x * pixel_density), int(ev.motion.y * pixel_density), GetKeyModifierState());
	}
	break;
	case event_mouse_down:
	{
		result = context->ProcessMouseButtonDown(ConvertMouseButton(ev.button.button), GetKeyModifierState());
		SDL_CaptureMouse(rmlsdl_true);
	}
	break;
	case event_mouse_up:
	{
		SDL_CaptureMouse(rmlsdl_false);
		result = context->ProcessMouseButtonUp(ConvertMouseButton(ev.button.button), GetKeyModifierState());
	}
	break;
#endif

	case event_mouse_wheel:
	{
		result = context->ProcessMouseWheel(float(-ev.wheel.y), GetKeyModifierState());
	}
	break;
	case event_key_down:
	{
		result = context->ProcessKeyDown(ConvertKey(GetKey(ev)), GetKeyModifierState());
		if (GetKey(ev) == SDLK_RETURN || GetKey(ev) == SDLK_KP_ENTER)
			result &= context->ProcessTextInput('\n');
	}
	break;
	case event_key_up:
	{
		result = context->ProcessKeyUp(ConvertKey(GetKey(ev)), GetKeyModifierState());
	}
	break;
	case event_text_input:
	{
		result = context->ProcessTextInput(Rml::String(&ev.text.text[0]));
	}
	break;
	case event_finger_down:
	{
		const Rml::TouchList touches = TouchEventToTouchList(ev, context, GetFingerId(ev));
		result = context->ProcessTouchStart(touches, GetKeyModifierState());
	}
	break;
	case event_finger_motion:
	{
		const Rml::TouchList touches = TouchEventToTouchList(ev, context, GetFingerId(ev));
		result = context->ProcessTouchMove(touches, GetKeyModifierState());
	}
	break;
	case event_finger_up:
	{
		const Rml::TouchList touches = TouchEventToTouchList(ev, context, GetFingerId(ev));
		result = context->ProcessTouchEnd(touches, GetKeyModifierState());
	}
	break;

		RMLSDL_WINDOW_EVENTS_BEGIN

	case event_window_size_changed:
	{
		Rml::Vector2i dimensions(ev.window.data1, ev.window.data2);
		context->SetDimensions(dimensions);
	}
	break;
	case event_window_leave:
	{
		context->ProcessMouseLeave();
	}
	break;

#if SDL_MAJOR_VERSION >= 3
	case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
	{
		const float display_scale = SDL_GetWindowDisplayScale(window);
		context->SetDensityIndependentPixelRatio(display_scale);
	}
	break;
#endif

		RMLSDL_WINDOW_EVENTS_END

	default: break;
	}

	return result;
}

Rml::Input::KeyIdentifier RmlSDL::ConvertKey(int sdlkey)
{
#if SDL_MAJOR_VERSION >= 3
	constexpr auto key_a = SDLK_A;
	constexpr auto key_b = SDLK_B;
	constexpr auto key_c = SDLK_C;
	constexpr auto key_d = SDLK_D;
	constexpr auto key_e = SDLK_E;
	constexpr auto key_f = SDLK_F;
	constexpr auto key_g = SDLK_G;
	constexpr auto key_h = SDLK_H;
	constexpr auto key_i = SDLK_I;
	constexpr auto key_j = SDLK_J;
	constexpr auto key_k = SDLK_K;
	constexpr auto key_l = SDLK_L;
	constexpr auto key_m = SDLK_M;
	constexpr auto key_n = SDLK_N;
	constexpr auto key_o = SDLK_O;
	constexpr auto key_p = SDLK_P;
	constexpr auto key_q = SDLK_Q;
	constexpr auto key_r = SDLK_R;
	constexpr auto key_s = SDLK_S;
	constexpr auto key_t = SDLK_T;
	constexpr auto key_u = SDLK_U;
	constexpr auto key_v = SDLK_V;
	constexpr auto key_w = SDLK_W;
	constexpr auto key_x = SDLK_X;
	constexpr auto key_y = SDLK_Y;
	constexpr auto key_z = SDLK_Z;
	constexpr auto key_grave = SDLK_GRAVE;
	constexpr auto key_dblapostrophe = SDLK_DBLAPOSTROPHE;
#else
	constexpr auto key_a = SDLK_a;
	constexpr auto key_b = SDLK_b;
	constexpr auto key_c = SDLK_c;
	constexpr auto key_d = SDLK_d;
	constexpr auto key_e = SDLK_e;
	constexpr auto key_f = SDLK_f;
	constexpr auto key_g = SDLK_g;
	constexpr auto key_h = SDLK_h;
	constexpr auto key_i = SDLK_i;
	constexpr auto key_j = SDLK_j;
	constexpr auto key_k = SDLK_k;
	constexpr auto key_l = SDLK_l;
	constexpr auto key_m = SDLK_m;
	constexpr auto key_n = SDLK_n;
	constexpr auto key_o = SDLK_o;
	constexpr auto key_p = SDLK_p;
	constexpr auto key_q = SDLK_q;
	constexpr auto key_r = SDLK_r;
	constexpr auto key_s = SDLK_s;
	constexpr auto key_t = SDLK_t;
	constexpr auto key_u = SDLK_u;
	constexpr auto key_v = SDLK_v;
	constexpr auto key_w = SDLK_w;
	constexpr auto key_x = SDLK_x;
	constexpr auto key_y = SDLK_y;
	constexpr auto key_z = SDLK_z;
	constexpr auto key_grave = SDLK_BACKQUOTE;
	constexpr auto key_dblapostrophe = SDLK_QUOTEDBL;
#endif

	// clang-format off
	switch (sdlkey)
	{
	case SDLK_UNKNOWN:      return Rml::Input::KI_UNKNOWN;
	case SDLK_ESCAPE:       return Rml::Input::KI_ESCAPE;
	case SDLK_SPACE:        return Rml::Input::KI_SPACE;
	case SDLK_0:            return Rml::Input::KI_0;
	case SDLK_1:            return Rml::Input::KI_1;
	case SDLK_2:            return Rml::Input::KI_2;
	case SDLK_3:            return Rml::Input::KI_3;
	case SDLK_4:            return Rml::Input::KI_4;
	case SDLK_5:            return Rml::Input::KI_5;
	case SDLK_6:            return Rml::Input::KI_6;
	case SDLK_7:            return Rml::Input::KI_7;
	case SDLK_8:            return Rml::Input::KI_8;
	case SDLK_9:            return Rml::Input::KI_9;
	case key_a:             return Rml::Input::KI_A;
	case key_b:             return Rml::Input::KI_B;
	case key_c:             return Rml::Input::KI_C;
	case key_d:             return Rml::Input::KI_D;
	case key_e:             return Rml::Input::KI_E;
	case key_f:             return Rml::Input::KI_F;
	case key_g:             return Rml::Input::KI_G;
	case key_h:             return Rml::Input::KI_H;
	case key_i:             return Rml::Input::KI_I;
	case key_j:             return Rml::Input::KI_J;
	case key_k:             return Rml::Input::KI_K;
	case key_l:             return Rml::Input::KI_L;
	case key_m:             return Rml::Input::KI_M;
	case key_n:             return Rml::Input::KI_N;
	case key_o:             return Rml::Input::KI_O;
	case key_p:             return Rml::Input::KI_P;
	case key_q:             return Rml::Input::KI_Q;
	case key_r:             return Rml::Input::KI_R;
	case key_s:             return Rml::Input::KI_S;
	case key_t:             return Rml::Input::KI_T;
	case key_u:             return Rml::Input::KI_U;
	case key_v:             return Rml::Input::KI_V;
	case key_w:             return Rml::Input::KI_W;
	case key_x:             return Rml::Input::KI_X;
	case key_y:             return Rml::Input::KI_Y;
	case key_z:             return Rml::Input::KI_Z;
	case SDLK_SEMICOLON:    return Rml::Input::KI_OEM_1;
	case SDLK_PLUS:         return Rml::Input::KI_OEM_PLUS;
	case SDLK_COMMA:        return Rml::Input::KI_OEM_COMMA;
	case SDLK_MINUS:        return Rml::Input::KI_OEM_MINUS;
	case SDLK_PERIOD:       return Rml::Input::KI_OEM_PERIOD;
	case SDLK_SLASH:        return Rml::Input::KI_OEM_2;
	case key_grave:         return Rml::Input::KI_OEM_3;
	case SDLK_LEFTBRACKET:  return Rml::Input::KI_OEM_4;
	case SDLK_BACKSLASH:    return Rml::Input::KI_OEM_5;
	case SDLK_RIGHTBRACKET: return Rml::Input::KI_OEM_6;
	case key_dblapostrophe: return Rml::Input::KI_OEM_7;
	case SDLK_KP_0:         return Rml::Input::KI_NUMPAD0;
	case SDLK_KP_1:         return Rml::Input::KI_NUMPAD1;
	case SDLK_KP_2:         return Rml::Input::KI_NUMPAD2;
	case SDLK_KP_3:         return Rml::Input::KI_NUMPAD3;
	case SDLK_KP_4:         return Rml::Input::KI_NUMPAD4;
	case SDLK_KP_5:         return Rml::Input::KI_NUMPAD5;
	case SDLK_KP_6:         return Rml::Input::KI_NUMPAD6;
	case SDLK_KP_7:         return Rml::Input::KI_NUMPAD7;
	case SDLK_KP_8:         return Rml::Input::KI_NUMPAD8;
	case SDLK_KP_9:         return Rml::Input::KI_NUMPAD9;
	case SDLK_KP_ENTER:     return Rml::Input::KI_NUMPADENTER;
	case SDLK_KP_MULTIPLY:  return Rml::Input::KI_MULTIPLY;
	case SDLK_KP_PLUS:      return Rml::Input::KI_ADD;
	case SDLK_KP_MINUS:     return Rml::Input::KI_SUBTRACT;
	case SDLK_KP_PERIOD:    return Rml::Input::KI_DECIMAL;
	case SDLK_KP_DIVIDE:    return Rml::Input::KI_DIVIDE;
	case SDLK_KP_EQUALS:    return Rml::Input::KI_OEM_NEC_EQUAL;
	case SDLK_BACKSPACE:    return Rml::Input::KI_BACK;
	case SDLK_TAB:          return Rml::Input::KI_TAB;
	case SDLK_CLEAR:        return Rml::Input::KI_CLEAR;
	case SDLK_RETURN:       return Rml::Input::KI_RETURN;
	case SDLK_PAUSE:        return Rml::Input::KI_PAUSE;
	case SDLK_CAPSLOCK:     return Rml::Input::KI_CAPITAL;
	case SDLK_PAGEUP:       return Rml::Input::KI_PRIOR;
	case SDLK_PAGEDOWN:     return Rml::Input::KI_NEXT;
	case SDLK_END:          return Rml::Input::KI_END;
	case SDLK_HOME:         return Rml::Input::KI_HOME;
	case SDLK_LEFT:         return Rml::Input::KI_LEFT;
	case SDLK_UP:           return Rml::Input::KI_UP;
	case SDLK_RIGHT:        return Rml::Input::KI_RIGHT;
	case SDLK_DOWN:         return Rml::Input::KI_DOWN;
	case SDLK_INSERT:       return Rml::Input::KI_INSERT;
	case SDLK_DELETE:       return Rml::Input::KI_DELETE;
	case SDLK_HELP:         return Rml::Input::KI_HELP;
	case SDLK_F1:           return Rml::Input::KI_F1;
	case SDLK_F2:           return Rml::Input::KI_F2;
	case SDLK_F3:           return Rml::Input::KI_F3;
	case SDLK_F4:           return Rml::Input::KI_F4;
	case SDLK_F5:           return Rml::Input::KI_F5;
	case SDLK_F6:           return Rml::Input::KI_F6;
	case SDLK_F7:           return Rml::Input::KI_F7;
	case SDLK_F8:           return Rml::Input::KI_F8;
	case SDLK_F9:           return Rml::Input::KI_F9;
	case SDLK_F10:          return Rml::Input::KI_F10;
	case SDLK_F11:          return Rml::Input::KI_F11;
	case SDLK_F12:          return Rml::Input::KI_F12;
	case SDLK_F13:          return Rml::Input::KI_F13;
	case SDLK_F14:          return Rml::Input::KI_F14;
	case SDLK_F15:          return Rml::Input::KI_F15;
	case SDLK_NUMLOCKCLEAR: return Rml::Input::KI_NUMLOCK;
	case SDLK_SCROLLLOCK:   return Rml::Input::KI_SCROLL;
	case SDLK_LSHIFT:       return Rml::Input::KI_LSHIFT;
	case SDLK_RSHIFT:       return Rml::Input::KI_RSHIFT;
	case SDLK_LCTRL:        return Rml::Input::KI_LCONTROL;
	case SDLK_RCTRL:        return Rml::Input::KI_RCONTROL;
	case SDLK_LALT:         return Rml::Input::KI_LMENU;
	case SDLK_RALT:         return Rml::Input::KI_RMENU;
	case SDLK_LGUI:         return Rml::Input::KI_LMETA;
	case SDLK_RGUI:         return Rml::Input::KI_RMETA;
	/*
	case SDLK_LSUPER:       return Rml::Input::KI_LWIN;
	case SDLK_RSUPER:       return Rml::Input::KI_RWIN;
	*/
	default: break;
	}
	// clang-format on

	return Rml::Input::KI_UNKNOWN;
}

int RmlSDL::ConvertMouseButton(int button)
{
	switch (button)
	{
	case SDL_BUTTON_LEFT: return 0;
	case SDL_BUTTON_RIGHT: return 1;
	case SDL_BUTTON_MIDDLE: return 2;
	default: return 3;
	}
}

int RmlSDL::GetKeyModifierState()
{
	SDL_Keymod sdl_mods = SDL_GetModState();

#if SDL_MAJOR_VERSION >= 3
	constexpr auto mod_ctrl = SDL_KMOD_CTRL;
	constexpr auto mod_shift = SDL_KMOD_SHIFT;
	constexpr auto mod_alt = SDL_KMOD_ALT;
	constexpr auto mod_num = SDL_KMOD_NUM;
	constexpr auto mod_caps = SDL_KMOD_CAPS;
#else
	constexpr auto mod_ctrl = KMOD_CTRL;
	constexpr auto mod_shift = KMOD_SHIFT;
	constexpr auto mod_alt = KMOD_ALT;
	constexpr auto mod_num = KMOD_NUM;
	constexpr auto mod_caps = KMOD_CAPS;
#endif

	int retval = 0;

	if (sdl_mods & mod_ctrl)
		retval |= Rml::Input::KM_CTRL;

	if (sdl_mods & mod_shift)
		retval |= Rml::Input::KM_SHIFT;

	if (sdl_mods & mod_alt)
		retval |= Rml::Input::KM_ALT;

	if (sdl_mods & mod_num)
		retval |= Rml::Input::KM_NUMLOCK;

	if (sdl_mods & mod_caps)
		retval |= Rml::Input::KM_CAPSLOCK;

	return retval;
}
