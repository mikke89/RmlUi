#include "RmlUi_Platform_X11.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Types.h>
#include <X11/cursorfont.h>
#include <X11/extensions/xf86vmode.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef HAS_X11XKBLIB
	#include <X11/XKBlib.h>
#endif // HAS_X11XKBLIB
#include <X11/keysym.h>

static Rml::Character GetCharacterCode(Rml::Input::KeyIdentifier key_identifier, int key_modifier_state);

SystemInterface_X11::SystemInterface_X11(Display* display) : display(display)
{
	// Create cursors
	cursor_default = XCreateFontCursor(display, XC_left_ptr);
	cursor_move = XCreateFontCursor(display, XC_fleur);
	cursor_pointer = XCreateFontCursor(display, XC_hand1);
	cursor_resize = XCreateFontCursor(display, XC_sizing);
	cursor_cross = XCreateFontCursor(display, XC_crosshair);
	cursor_text = XCreateFontCursor(display, XC_xterm);
	cursor_unavailable = XCreateFontCursor(display, XC_X_cursor);

	// For copy & paste functions
	UTF8_atom = XInternAtom(display, "UTF8_STRING", 1);
	XSEL_DATA_atom = XInternAtom(display, "XSEL_DATA", 0);
	CLIPBOARD_atom = XInternAtom(display, "CLIPBOARD", 0);
	TARGETS_atom = XInternAtom(display, "TARGETS", 0);
	TEXT_atom = XInternAtom(display, "TEXT", 0);

	gettimeofday(&start_time, nullptr);
}

void SystemInterface_X11::SetWindow(Window in_window)
{
	window = in_window;
}

bool SystemInterface_X11::HandleSelectionRequest(const XEvent& ev)
{
	if (ev.type == SelectionRequest && XGetSelectionOwner(display, CLIPBOARD_atom) == window && ev.xselectionrequest.selection == CLIPBOARD_atom)
	{
		XCopy(clipboard_text, ev);
		return false;
	}
	return true;
}

double SystemInterface_X11::GetElapsedTime()
{
	struct timeval now;

	gettimeofday(&now, nullptr);

	double sec = now.tv_sec - start_time.tv_sec;
	double usec = now.tv_usec - start_time.tv_usec;
	double result = sec + (usec / 1000000.0);

	return result;
}

void SystemInterface_X11::SetMouseCursor(const Rml::String& cursor_name)
{
	if (display && window)
	{
		Cursor cursor_handle = 0;
		if (cursor_name.empty() || cursor_name == "arrow")
			cursor_handle = cursor_default;
		else if (cursor_name == "move")
			cursor_handle = cursor_move;
		else if (cursor_name == "pointer")
			cursor_handle = cursor_pointer;
		else if (cursor_name == "resize")
			cursor_handle = cursor_resize;
		else if (cursor_name == "cross")
			cursor_handle = cursor_cross;
		else if (cursor_name == "text")
			cursor_handle = cursor_text;
		else if (cursor_name == "rmlui-scroll-idle")
			cursor_handle = cursor_move;
		else if (cursor_name == "rmlui-scroll-up")
			cursor_handle = cursor_move;
		else if (cursor_name == "rmlui-scroll-down")
			cursor_handle = cursor_move;
		else if (cursor_name == "unavailable")
			cursor_handle = cursor_unavailable;

		if (cursor_handle)
		{
			XDefineCursor(display, window, cursor_handle);
		}
	}
}

void SystemInterface_X11::SetClipboardText(const Rml::String& text)
{
	clipboard_text = text;
	XSetSelectionOwner(display, CLIPBOARD_atom, window, 0);
}

void SystemInterface_X11::GetClipboardText(Rml::String& text)
{
	if (XGetSelectionOwner(display, CLIPBOARD_atom) != window)
	{
		if (!UTF8_atom || !XPaste(UTF8_atom, text))
		{
			// fallback
			XPaste(XA_STRING_atom, text);
		}
	}
	else
	{
		text = clipboard_text;
	}
}

void SystemInterface_X11::XCopy(const Rml::String& clipboard_data, const XEvent& event)
{
	Atom format = (UTF8_atom ? UTF8_atom : XA_STRING_atom);

	XSelectionEvent ev = {
		SelectionNotify, // the event type that will be sent to the requestor
		0,               // serial
		0,               // send_event
		event.xselectionrequest.display, event.xselectionrequest.requestor, event.xselectionrequest.selection, event.xselectionrequest.target,
		event.xselectionrequest.property,
		0 // time
	};

	int retval = 0;
	if (ev.target == TARGETS_atom)
	{
		retval = XChangeProperty(ev.display, ev.requestor, ev.property, XA_atom, 32, PropModeReplace, (unsigned char*)&format, 1);
	}
	else if (ev.target == XA_STRING_atom || ev.target == TEXT_atom)
	{
		retval = XChangeProperty(ev.display, ev.requestor, ev.property, XA_STRING_atom, 8, PropModeReplace, (unsigned char*)clipboard_data.c_str(),
			clipboard_data.size());
	}
	else if (ev.target == UTF8_atom)
	{
		retval = XChangeProperty(ev.display, ev.requestor, ev.property, UTF8_atom, 8, PropModeReplace, (unsigned char*)clipboard_data.c_str(),
			clipboard_data.size());
	}
	else
	{
		ev.property = 0;
	}

	if ((retval & 2) == 0)
	{
		// Notify the requestor that clipboard data is available
		XSendEvent(display, ev.requestor, 0, 0, (XEvent*)&ev);
	}
}

bool SystemInterface_X11::XPaste(Atom target_atom, Rml::String& clipboard_data)
{
	XEvent ev;

	// A SelectionRequest event will be sent to the clipboard owner, which should respond with SelectionNotify
	XConvertSelection(display, CLIPBOARD_atom, target_atom, XSEL_DATA_atom, window, CurrentTime);
	XSync(display, 0);
	XNextEvent(display, &ev);

	if (ev.type == SelectionNotify)
	{
		if (ev.xselection.property == 0)
		{
			// If no owner for the specified selection exists, the X server generates
			// a SelectionNotify event with property None (0).
			return false;
		}
		if (ev.xselection.selection == CLIPBOARD_atom)
		{
			int actual_format;
			unsigned long bytes_after, nitems;
			char* prop = nullptr;
			Atom actual_type;
			XGetWindowProperty(ev.xselection.display, ev.xselection.requestor, ev.xselection.property,
				0L,    // offset
				(~0L), // length
				0,     // delete?
				AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, (unsigned char**)&prop);

			if (actual_type == UTF8_atom || actual_type == XA_STRING_atom)
			{
				clipboard_data = Rml::String(prop, prop + nitems);
				XFree(prop);
			}

			XDeleteProperty(ev.xselection.display, ev.xselection.requestor, ev.xselection.property);
			return true;
		}
	}

	return false;
}

bool RmlX11::HandleInputEvent(Rml::Context* context, Display* display, const XEvent& ev)
{
	switch (ev.type)
	{
	case ButtonPress:
	{
		switch (ev.xbutton.button)
		{
		case Button1:
		case Button2:
		case Button3: return context->ProcessMouseButtonDown(ConvertMouseButton(ev.xbutton.button), RmlX11::GetKeyModifierState(ev.xbutton.state));
		case Button4: return context->ProcessMouseWheel(-1, RmlX11::GetKeyModifierState(ev.xbutton.state));
		case Button5: return context->ProcessMouseWheel(1, RmlX11::GetKeyModifierState(ev.xbutton.state));
		default: return true;
		}
	}
	break;
	case ButtonRelease:
	{
		switch (ev.xbutton.button)
		{
		case Button1:
		case Button2:
		case Button3: return context->ProcessMouseButtonUp(ConvertMouseButton(ev.xbutton.button), RmlX11::GetKeyModifierState(ev.xbutton.state));
		default: return true;
		}
	}
	break;
	case MotionNotify:
	{
		return context->ProcessMouseMove(ev.xmotion.x, ev.xmotion.y, RmlX11::GetKeyModifierState(ev.xmotion.state));
	}
	break;
	case LeaveNotify:
	{
		return context->ProcessMouseLeave();
	}
	break;
	case KeyPress:
	{
		Rml::Input::KeyIdentifier key_identifier = RmlX11::ConvertKey(display, ev.xkey.keycode);
		const int key_modifier_state = RmlX11::GetKeyModifierState(ev.xkey.state);

		bool propagates = true;

		if (key_identifier != Rml::Input::KI_UNKNOWN)
			propagates = context->ProcessKeyDown(key_identifier, key_modifier_state);

		Rml::Character character = GetCharacterCode(key_identifier, key_modifier_state);
		if (character != Rml::Character::Null && !(key_modifier_state & Rml::Input::KM_CTRL))
			propagates &= context->ProcessTextInput(character);

		return propagates;
	}
	break;
	case KeyRelease:
	{
		Rml::Input::KeyIdentifier key_identifier = RmlX11::ConvertKey(display, ev.xkey.keycode);
		const int key_modifier_state = RmlX11::GetKeyModifierState(ev.xkey.state);

		bool propagates = true;

		if (key_identifier != Rml::Input::KI_UNKNOWN)
			propagates = context->ProcessKeyUp(key_identifier, key_modifier_state);

		return propagates;
	}
	break;
	default: break;
	}

	return true;
}

int RmlX11::GetKeyModifierState(int x11_state)
{
	int key_modifier_state = 0;

	if (x11_state & ShiftMask)
		key_modifier_state |= Rml::Input::KM_SHIFT;

	if (x11_state & LockMask)
		key_modifier_state |= Rml::Input::KM_CAPSLOCK;

	if (x11_state & ControlMask)
		key_modifier_state |= Rml::Input::KM_CTRL;

	if (x11_state & Mod5Mask)
		key_modifier_state |= Rml::Input::KM_ALT;

	if (x11_state & Mod2Mask)
		key_modifier_state |= Rml::Input::KM_NUMLOCK;

	return key_modifier_state;
}

/**
    X11 Key data used for key code conversion.
 */
struct XKeyData {
#ifdef HAS_X11XKBLIB
	bool has_xkblib = false;
#endif // HAS_X11XKBLIB

	bool initialized = false;
	int min_keycode = 0, max_keycode = 0, keysyms_per_keycode = 0;
	KeySym* x11_key_mapping = nullptr;
};

// Get the key data, which is initialized and filled on the first fetch.
static const XKeyData& GetXKeyData(Display* display)
{
	RMLUI_ASSERT(display);
	static XKeyData data;

	if (!data.initialized)
	{
		data.initialized = true;

#ifdef HAS_X11XKBLIB
		int opcode_rtrn = -1;
		int event_rtrn = -1;
		int error_rtrn = -1;
		int major_in_out = -1;
		int minor_in_out = -1;

		// Xkb extension may not exist in the server. This checks for its existence and initializes the extension if available.
		data.has_xkblib = XkbQueryExtension(display, &opcode_rtrn, &event_rtrn, &error_rtrn, &major_in_out, &minor_in_out);

		// if Xkb isn't available, fall back to using XGetKeyboardMapping, which may occur if RmlUi is compiled with Xkb support but the server
		// doesn't support it. This occurs with older X11 servers or virtual framebuffers such as x11vnc server.
		if (!data.has_xkblib)
#endif // HAS_X11XKBLIB
		{
			XDisplayKeycodes(display, &data.min_keycode, &data.max_keycode);

			data.x11_key_mapping = XGetKeyboardMapping(display, data.min_keycode, data.max_keycode + 1 - data.min_keycode, &data.keysyms_per_keycode);
		}
	}

	return data;
}

Rml::Input::KeyIdentifier RmlX11::ConvertKey(Display* display, unsigned int x11_key_code)
{
	RMLUI_ASSERT(display);
	const XKeyData& key_data = GetXKeyData(display);

	const int group_index = 0; // this is always 0 for our limited example
	KeySym sym = {};

#ifdef HAS_X11XKBLIB
	if (key_data.has_xkblib)
	{
		sym = XkbKeycodeToKeysym(display, x11_key_code, 0, group_index);
	}
	else
#endif // HAS_X11XKBLIB
	{
		KeySym sym_full = key_data.x11_key_mapping[(x11_key_code - key_data.min_keycode) * key_data.keysyms_per_keycode + group_index];

		KeySym lower_sym, upper_sym;
		XConvertCase(sym_full, &lower_sym, &upper_sym);
		sym = lower_sym;
	}

	// clang-format off
	switch (sym & 0xFF)
	{
	case XK_BackSpace & 0xFF:             return Rml::Input::KI_BACK;
	case XK_Tab & 0xFF:                   return Rml::Input::KI_TAB;
	case XK_Clear & 0xFF:                 return Rml::Input::KI_CLEAR;
	case XK_Return & 0xFF:                return Rml::Input::KI_RETURN;
	case XK_Pause & 0xFF:                 return Rml::Input::KI_PAUSE;
	case XK_Scroll_Lock & 0xFF:           return Rml::Input::KI_SCROLL;
	case XK_Escape & 0xFF:                return Rml::Input::KI_ESCAPE;
	case XK_Delete & 0xFF:                return Rml::Input::KI_DELETE;

	case XK_Kanji & 0xFF:                 return Rml::Input::KI_KANJI;
	//	case XK_Muhenkan & 0xFF:          return Rml::Input::; /* Cancel Conversion */
	//	case XK_Henkan_Mode & 0xFF:       return Rml::Input::; /* Start/Stop Conversion */
	//	case XK_Henkan & 0xFF:            return Rml::Input::; /* Alias for Henkan_Mode */
	//	case XK_Romaji & 0xFF:            return Rml::Input::; /* to Romaji */
	//	case XK_Hiragana & 0xFF:          return Rml::Input::; /* to Hiragana */
	//	case XK_Katakana & 0xFF:          return Rml::Input::; /* to Katakana */
	//	case XK_Hiragana_Katakana & 0xFF: return Rml::Input::; /* Hiragana/Katakana toggle */
	//	case XK_Zenkaku & 0xFF:           return Rml::Input::; /* to Zenkaku */
	//	case XK_Hankaku & 0xFF:           return Rml::Input::; /* to Hankaku */
	//	case XK_Zenkaku_Hankaku & 0xFF:   return Rml::Input::; /* Zenkaku/Hankaku toggle */
	case XK_Touroku & 0xFF:               return Rml::Input::KI_OEM_FJ_TOUROKU;
	//  case XK_Massyo & 0xFF:            return Rml::Input::KI_OEM_FJ_MASSHOU;
	//	case XK_Kana_Lock & 0xFF:         return Rml::Input::; /* Kana Lock */
	//	case XK_Kana_Shift & 0xFF:        return Rml::Input::; /* Kana Shift */
	//	case XK_Eisu_Shift & 0xFF:        return Rml::Input::; /* Alphanumeric Shift */
	//	case XK_Eisu_toggle & 0xFF:       return Rml::Input::; /* Alphanumeric toggle */

	case XK_Home & 0xFF:                  return Rml::Input::KI_HOME;
	case XK_Left & 0xFF:                  return Rml::Input::KI_LEFT;
	case XK_Up & 0xFF:                    return Rml::Input::KI_UP;
	case XK_Right & 0xFF:                 return Rml::Input::KI_RIGHT;
	case XK_Down & 0xFF:                  return Rml::Input::KI_DOWN;
	case XK_Prior & 0xFF:                 return Rml::Input::KI_PRIOR;
	case XK_Next & 0xFF:                  return Rml::Input::KI_NEXT;
	case XK_End & 0xFF:                   return Rml::Input::KI_END;
	case XK_Begin & 0xFF:                 return Rml::Input::KI_HOME;

	// case XK_Print & 0xFF:              return Rml::Input::KI_SNAPSHOT;
	// case XK_Insert & 0xFF:             return Rml::Input::KI_INSERT;
	case XK_Num_Lock & 0xFF:              return Rml::Input::KI_NUMLOCK;

	case XK_KP_Space & 0xFF:              return Rml::Input::KI_SPACE;
	case XK_KP_Tab & 0xFF:                return Rml::Input::KI_TAB;
	case XK_KP_Enter & 0xFF:              return Rml::Input::KI_NUMPADENTER;
	case XK_KP_F1 & 0xFF:                 return Rml::Input::KI_F1;
	case XK_KP_F2 & 0xFF:                 return Rml::Input::KI_F2;
	case XK_KP_F3 & 0xFF:                 return Rml::Input::KI_F3;
	case XK_KP_F4 & 0xFF:                 return Rml::Input::KI_F4;
	case XK_KP_Home & 0xFF:               return Rml::Input::KI_NUMPAD7;
	case XK_KP_Left & 0xFF:               return Rml::Input::KI_NUMPAD4;
	case XK_KP_Up & 0xFF:                 return Rml::Input::KI_NUMPAD8;
	case XK_KP_Right & 0xFF:              return Rml::Input::KI_NUMPAD6;
	case XK_KP_Down & 0xFF:               return Rml::Input::KI_NUMPAD2;
	case XK_KP_Prior & 0xFF:              return Rml::Input::KI_NUMPAD9;
	case XK_KP_Next & 0xFF:               return Rml::Input::KI_NUMPAD3;
	case XK_KP_End & 0xFF:                return Rml::Input::KI_NUMPAD1;
	case XK_KP_Begin & 0xFF:              return Rml::Input::KI_NUMPAD5;
	case XK_KP_Insert & 0xFF:             return Rml::Input::KI_NUMPAD0;
	case XK_KP_Delete & 0xFF:             return Rml::Input::KI_DECIMAL;
	case XK_KP_Equal & 0xFF:              return Rml::Input::KI_OEM_NEC_EQUAL;
	case XK_KP_Multiply & 0xFF:           return Rml::Input::KI_MULTIPLY;
	case XK_KP_Add & 0xFF:                return Rml::Input::KI_ADD;
	case XK_KP_Separator & 0xFF:          return Rml::Input::KI_SEPARATOR;
	case XK_KP_Subtract & 0xFF:           return Rml::Input::KI_SUBTRACT;
	case XK_KP_Decimal & 0xFF:            return Rml::Input::KI_DECIMAL;
	case XK_KP_Divide & 0xFF:             return Rml::Input::KI_DIVIDE;

	case XK_F1 & 0xFF:                    return Rml::Input::KI_F1;
	case XK_F2 & 0xFF:                    return Rml::Input::KI_F2;
	case XK_F3 & 0xFF:                    return Rml::Input::KI_F3;
	case XK_F4 & 0xFF:                    return Rml::Input::KI_F4;
	case XK_F5 & 0xFF:                    return Rml::Input::KI_F5;
	case XK_F6 & 0xFF:                    return Rml::Input::KI_F6;
	case XK_F7 & 0xFF:                    return Rml::Input::KI_F7;
	case XK_F8 & 0xFF:                    return Rml::Input::KI_F8;
	case XK_F9 & 0xFF:                    return Rml::Input::KI_F9;
	case XK_F10 & 0xFF:                   return Rml::Input::KI_F10;
	case XK_F11 & 0xFF:                   return Rml::Input::KI_F11;
	case XK_F12 & 0xFF:                   return Rml::Input::KI_F12;
	case XK_F13 & 0xFF:                   return Rml::Input::KI_F13;
	case XK_F14 & 0xFF:                   return Rml::Input::KI_F14;
	case XK_F15 & 0xFF:                   return Rml::Input::KI_F15;
	case XK_F16 & 0xFF:                   return Rml::Input::KI_F16;
	case XK_F17 & 0xFF:                   return Rml::Input::KI_F17;
	case XK_F18 & 0xFF:                   return Rml::Input::KI_F18;
	case XK_F19 & 0xFF:                   return Rml::Input::KI_F19;
	case XK_F20 & 0xFF:                   return Rml::Input::KI_F20;
	case XK_F21 & 0xFF:                   return Rml::Input::KI_F21;
	case XK_F22 & 0xFF:                   return Rml::Input::KI_F22;
	case XK_F23 & 0xFF:                   return Rml::Input::KI_F23;
	case XK_F24 & 0xFF:                   return Rml::Input::KI_F24;

	case XK_Shift_L & 0xFF:               return Rml::Input::KI_LSHIFT;
	case XK_Shift_R & 0xFF:               return Rml::Input::KI_RSHIFT;
	case XK_Control_L & 0xFF:             return Rml::Input::KI_LCONTROL;
	case XK_Control_R & 0xFF:             return Rml::Input::KI_RCONTROL;
	case XK_Caps_Lock & 0xFF:             return Rml::Input::KI_CAPITAL;

	case XK_Alt_L & 0xFF:                 return Rml::Input::KI_LMENU;
	case XK_Alt_R & 0xFF:                 return Rml::Input::KI_RMENU;

	case XK_space & 0xFF:                 return Rml::Input::KI_SPACE;
	case XK_apostrophe & 0xFF:            return Rml::Input::KI_OEM_7;
	case XK_comma & 0xFF:                 return Rml::Input::KI_OEM_COMMA;
	case XK_minus & 0xFF:                 return Rml::Input::KI_OEM_MINUS;
	case XK_period & 0xFF:                return Rml::Input::KI_OEM_PERIOD;
	case XK_slash & 0xFF:                 return Rml::Input::KI_OEM_2;
	case XK_0 & 0xFF:                     return Rml::Input::KI_0;
	case XK_1 & 0xFF:                     return Rml::Input::KI_1;
	case XK_2 & 0xFF:                     return Rml::Input::KI_2;
	case XK_3 & 0xFF:                     return Rml::Input::KI_3;
	case XK_4 & 0xFF:                     return Rml::Input::KI_4;
	case XK_5 & 0xFF:                     return Rml::Input::KI_5;
	case XK_6 & 0xFF:                     return Rml::Input::KI_6;
	case XK_7 & 0xFF:                     return Rml::Input::KI_7;
	case XK_8 & 0xFF:                     return Rml::Input::KI_8;
	case XK_9 & 0xFF:                     return Rml::Input::KI_9;
	case XK_semicolon & 0xFF:             return Rml::Input::KI_OEM_1;
	case XK_equal & 0xFF:                 return Rml::Input::KI_OEM_PLUS;
	case XK_bracketleft & 0xFF:           return Rml::Input::KI_OEM_4;
	case XK_backslash & 0xFF:             return Rml::Input::KI_OEM_5;
	case XK_bracketright & 0xFF:          return Rml::Input::KI_OEM_6;
	case XK_grave & 0xFF:                 return Rml::Input::KI_OEM_3;
	case XK_a & 0xFF:                     return Rml::Input::KI_A;
	case XK_b & 0xFF:                     return Rml::Input::KI_B;
	case XK_c & 0xFF:                     return Rml::Input::KI_C;
	case XK_d & 0xFF:                     return Rml::Input::KI_D;
	case XK_e & 0xFF:                     return Rml::Input::KI_E;
	case XK_f & 0xFF:                     return Rml::Input::KI_F;
	case XK_g & 0xFF:                     return Rml::Input::KI_G;
	case XK_h & 0xFF:                     return Rml::Input::KI_H;
	case XK_i & 0xFF:                     return Rml::Input::KI_I;
	case XK_j & 0xFF:                     return Rml::Input::KI_J;
	case XK_k & 0xFF:                     return Rml::Input::KI_K;
	case XK_l & 0xFF:                     return Rml::Input::KI_L;
	case XK_m & 0xFF:                     return Rml::Input::KI_M;
	case XK_n & 0xFF:                     return Rml::Input::KI_N;
	case XK_o & 0xFF:                     return Rml::Input::KI_O;
	case XK_p & 0xFF:                     return Rml::Input::KI_P;
	case XK_q & 0xFF:                     return Rml::Input::KI_Q;
	case XK_r & 0xFF:                     return Rml::Input::KI_R;
	case XK_s & 0xFF:                     return Rml::Input::KI_S;
	case XK_t & 0xFF:                     return Rml::Input::KI_T;
	case XK_u & 0xFF:                     return Rml::Input::KI_U;
	case XK_v & 0xFF:                     return Rml::Input::KI_V;
	case XK_w & 0xFF:                     return Rml::Input::KI_W;
	case XK_x & 0xFF:                     return Rml::Input::KI_X;
	case XK_y & 0xFF:                     return Rml::Input::KI_Y;
	case XK_z & 0xFF:                     return Rml::Input::KI_Z;
	default: break;
	}
	// clang-format on

	return Rml::Input::KI_UNKNOWN;
}

int RmlX11::ConvertMouseButton(unsigned int x11_mouse_button)
{
	switch (x11_mouse_button)
	{
	case Button1: return 0;
	case Button2: return 2;
	case Button3: return 1;
	default: break;
	}
	return 0;
}

/**
    This map contains 4 different mappings from key identifiers to character codes. Each entry represents a different
    combination of shift and capslock state.
 */

static char ascii_map[4][51] = {
	// shift off and capslock off
	{0, ' ', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
		'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ';', '=', ',', '-', '.', '/', '`', '[', '\\', ']', '\'', 0, 0},
	// shift on and capslock off
	{0, ' ', ')', '!', '@', '#', '$', '%', '^', '&', '*', '(', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ':', '+', '<', '_', '>', '?', '~', '{', '|', '}', '"', 0, 0},
	// shift on and capslock on
	{0, ' ', ')', '!', '@', '#', '$', '%', '^', '&', '*', '(', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
		'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ':', '+', '<', '_', '>', '?', '~', '{', '|', '}', '"', 0, 0},
	// shift off and capslock on
	{0, ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ';', '=', ',', '-', '.', '/', '`', '[', '\\', ']', '\'', 0, 0}};

static char keypad_map[2][18] = {{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\n', '*', '+', 0, '-', '.', '/', '='},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\n', '*', '+', 0, '-', 0, '/', '='}};

// Returns the character code for a key identifer / key modifier combination.
static Rml::Character GetCharacterCode(Rml::Input::KeyIdentifier key_identifier, int key_modifier_state)
{
	using Rml::Character;

	// Check if we have a keycode capable of generating characters on the main keyboard (ie, not on the numeric
	// keypad; that is dealt with below).
	if (key_identifier <= Rml::Input::KI_OEM_102)
	{
		// Get modifier states
		bool shift = (key_modifier_state & Rml::Input::KM_SHIFT) > 0;
		bool capslock = (key_modifier_state & Rml::Input::KM_CAPSLOCK) > 0;

		// Return character code based on identifier and modifiers
		if (shift && !capslock)
			return (Character)ascii_map[1][key_identifier];

		if (shift && capslock)
			return (Character)ascii_map[2][key_identifier];

		if (!shift && capslock)
			return (Character)ascii_map[3][key_identifier];

		return (Character)ascii_map[0][key_identifier];
	}

	// Check if we have a keycode from the numeric keypad.
	else if (key_identifier <= Rml::Input::KI_OEM_NEC_EQUAL)
	{
		if (key_modifier_state & Rml::Input::KM_NUMLOCK)
			return (Character)keypad_map[0][key_identifier - Rml::Input::KI_NUMPAD0];
		else
			return (Character)keypad_map[1][key_identifier - Rml::Input::KI_NUMPAD0];
	}

	else if (key_identifier == Rml::Input::KI_RETURN)
		return (Character)'\n';

	return Character::Null;
}
