/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include <x11/InputX11.h>
#include <Rocket/Core/Context.h>
#include <Rocket/Core/Input.h>
#include <Rocket/Debugger.h>
#include <Shell.h>
#include <X11/Xlib.h>
#ifdef HAS_X11XKBLIB
#include <X11/XKBlib.h>
#endif // HAS_X11XKBLIB
#include <X11/keysym.h>
#include <X11/Xutil.h>

static void InitialiseKeymap();
static int GetKeyModifierState(int x_state);

static const int KEYMAP_SIZE = 256;
static Rocket::Core::Input::KeyIdentifier key_identifier_map[KEYMAP_SIZE];

#ifdef HAS_X11XKBLIB
static bool has_xkblib = false;
#endif // HAS_X11XKBLIB

static int min_keycode, max_keycode, keysyms_per_keycode;
static KeySym *x11_key_mapping = NULL;

bool InputX11::Initialise()
{
	InitialiseKeymap();
	return true;
}

void InputX11::Shutdown()
{
}

void InputX11::InitialiseX11Keymap(Display *display)
{
	ROCKET_ASSERT(display != NULL);

#ifdef HAS_X11XKBLIB
	int opcode_rtrn = -1;
	int event_rtrn = -1;
	int error_rtrn = -1;
	int major_in_out = -1;
	int minor_in_out = -1;

	// Xkb extension may not exist in the server.  This checks for its
	// existence and initializes the extension if available.
	has_xkblib = XkbQueryExtension(display, &opcode_rtrn, &event_rtrn, &error_rtrn, &major_in_out, &minor_in_out);

	// if Xkb isn't available, fall back to using XGetKeyboardMapping, 
	// which may occur if libRocket is compiled with Xkb support but the
	// server doesn't support it.  This occurs with older X11 servers or
	// virtual framebuffers such as x11vnc server.
	if(!has_xkblib)
#endif // HAS_X11XKBLIB
	{
		XDisplayKeycodes(display, &min_keycode, &max_keycode);

		ROCKET_ASSERT(x11_key_mapping != NULL);
		x11_key_mapping = XGetKeyboardMapping(display, min_keycode, max_keycode + 1 - min_keycode, &keysyms_per_keycode);
	}
}

void InputX11::ProcessXEvent(Display* display, const XEvent& event)
{
	// Process all mouse and keyboard events
	switch (event.type)
	{
		case ButtonPress:
		{
			int button_index;

			switch (event.xbutton.button)
			{
				case Button1:	button_index = 0; break;
				case Button2:	button_index = 2; break;
				case Button3:	button_index = 1; break;
				case Button4:	context->ProcessMouseWheel(-1, GetKeyModifierState(event.xbutton.state)); return;
				case Button5:	context->ProcessMouseWheel(1, GetKeyModifierState(event.xbutton.state)); return;
				default:		return;
			}

			context->ProcessMouseButtonDown(button_index, GetKeyModifierState(event.xbutton.state));
		}
		break;

		case ButtonRelease:
		{
			int button_index;

			switch (event.xbutton.button)
			{
				case Button1:	button_index = 0; break;
				case Button2:	button_index = 2; break;
				case Button3:	button_index = 1; break;
				default:		return;
			}

			context->ProcessMouseButtonUp(button_index, GetKeyModifierState(event.xbutton.state));
		}
		break;

		case MotionNotify:
			context->ProcessMouseMove(event.xmotion.x, event.xmotion.y, GetKeyModifierState(event.xmotion.state));
			break;

		case KeyPress: 
		{
			int group_index = 0; // this is always 0 for our limited example
			Rocket::Core::Input::KeyIdentifier key_identifier;
#ifdef HAS_X11XKBLIB
			if(has_xkblib)
			{
				KeySym sym = XkbKeycodeToKeysym(display, event.xkey.keycode, 0, group_index);

				key_identifier = key_identifier_map[sym & 0xFF];
			}
			else
#endif // HAS_X11XKBLIB
			{
				KeySym sym = x11_key_mapping[(event.xkey.keycode - min_keycode) * keysyms_per_keycode + group_index];

				KeySym lower_sym, upper_sym;
				XConvertCase(sym, &lower_sym, &upper_sym);

				key_identifier = key_identifier_map[lower_sym & 0xFF];
			}

			int key_modifier_state = GetKeyModifierState(event.xkey.state);

			// Check for a shift-~ to toggle the debugger.
			if (key_identifier == Rocket::Core::Input::KI_OEM_3 &&
				key_modifier_state & Rocket::Core::Input::KM_SHIFT)
			{
				Rocket::Debugger::SetVisible(!Rocket::Debugger::IsVisible());
				break;
			}

			if (key_identifier != Rocket::Core::Input::KI_UNKNOWN)
				context->ProcessKeyDown(key_identifier, key_modifier_state);

			Rocket::Core::word character = GetCharacterCode(key_identifier, key_modifier_state);
			if (character > 0)
				context->ProcessTextInput(character);
		}
		break;

		case KeyRelease:
		{
			int group_index = 0; // this is always 0 for our limited example
			Rocket::Core::Input::KeyIdentifier key_identifier;
#ifdef HAS_X11XKBLIB
			if(has_xkblib)
			{
				KeySym sym = XkbKeycodeToKeysym(display, event.xkey.keycode, 0, group_index);

				key_identifier = key_identifier_map[sym & 0xFF];
			}
			else
#endif // HAS_X11XKBLIB
			{
				KeySym sym = x11_key_mapping[(event.xkey.keycode - min_keycode) * keysyms_per_keycode + group_index];

				KeySym lower_sym, upper_sym;
				XConvertCase(sym, &lower_sym, &upper_sym);

				key_identifier = key_identifier_map[lower_sym & 0xFF];
			}

			int key_modifier_state = GetKeyModifierState(event.xkey.state);
			if (key_identifier != Rocket::Core::Input::KI_UNKNOWN)
				context->ProcessKeyUp(key_identifier, key_modifier_state);
		}
		break;
	}
}

static int GetKeyModifierState(int x_state)
{
	int key_modifier_state = 0;

	if (x_state & ShiftMask)
		key_modifier_state |= Rocket::Core::Input::KM_SHIFT;

	if (x_state & LockMask)
		key_modifier_state |= Rocket::Core::Input::KM_CAPSLOCK;

	if (x_state & ControlMask)
		key_modifier_state |= Rocket::Core::Input::KM_CTRL;

	if (x_state & Mod5Mask)
		key_modifier_state |= Rocket::Core::Input::KM_ALT;

	if (x_state & Mod2Mask)
		key_modifier_state |= Rocket::Core::Input::KM_NUMLOCK;

	return key_modifier_state;
}

static void InitialiseKeymap()
{
	// Initialise the key map with default values.
	memset(key_identifier_map, 0, sizeof(key_identifier_map));

	key_identifier_map[XK_BackSpace & 0xFF] = Rocket::Core::Input::KI_BACK;
	key_identifier_map[XK_Tab & 0xFF] = Rocket::Core::Input::KI_TAB;
	key_identifier_map[XK_Clear & 0xFF] = Rocket::Core::Input::KI_CLEAR;
	key_identifier_map[XK_Return & 0xFF] = Rocket::Core::Input::KI_RETURN;
	key_identifier_map[XK_Pause & 0xFF] = Rocket::Core::Input::KI_PAUSE;
	key_identifier_map[XK_Scroll_Lock & 0xFF] = Rocket::Core::Input::KI_SCROLL;
	key_identifier_map[XK_Escape & 0xFF] = Rocket::Core::Input::KI_ESCAPE;
	key_identifier_map[XK_Delete & 0xFF] = Rocket::Core::Input::KI_DELETE;

	key_identifier_map[XK_Kanji & 0xFF] = Rocket::Core::Input::KI_KANJI;
//	key_identifier_map[XK_Muhenkan & 0xFF] = Rocket::Core::Input::; /* Cancel Conversion */
//	key_identifier_map[XK_Henkan_Mode & 0xFF] = Rocket::Core::Input::; /* Start/Stop Conversion */
//	key_identifier_map[XK_Henkan & 0xFF] = Rocket::Core::Input::; /* Alias for Henkan_Mode */
//	key_identifier_map[XK_Romaji & 0xFF] = Rocket::Core::Input::; /* to Romaji */
//	key_identifier_map[XK_Hiragana & 0xFF] = Rocket::Core::Input::; /* to Hiragana */
//	key_identifier_map[XK_Katakana & 0xFF] = Rocket::Core::Input::; /* to Katakana */
//	key_identifier_map[XK_Hiragana_Katakana & 0xFF] = Rocket::Core::Input::; /* Hiragana/Katakana toggle */
//	key_identifier_map[XK_Zenkaku & 0xFF] = Rocket::Core::Input::; /* to Zenkaku */
//	key_identifier_map[XK_Hankaku & 0xFF] = Rocket::Core::Input::; /* to Hankaku */
//	key_identifier_map[XK_Zenkaku_Hankaku & 0xFF] = Rocket::Core::Input::; /* Zenkaku/Hankaku toggle */
	key_identifier_map[XK_Touroku & 0xFF] = Rocket::Core::Input::KI_OEM_FJ_TOUROKU;
	key_identifier_map[XK_Massyo & 0xFF] = Rocket::Core::Input::KI_OEM_FJ_MASSHOU;
//	key_identifier_map[XK_Kana_Lock & 0xFF] = Rocket::Core::Input::; /* Kana Lock */
//	key_identifier_map[XK_Kana_Shift & 0xFF] = Rocket::Core::Input::; /* Kana Shift */
//	key_identifier_map[XK_Eisu_Shift & 0xFF] = Rocket::Core::Input::; /* Alphanumeric Shift */
//	key_identifier_map[XK_Eisu_toggle & 0xFF] = Rocket::Core::Input::; /* Alphanumeric toggle */

	key_identifier_map[XK_Home & 0xFF] = Rocket::Core::Input::KI_HOME;
	key_identifier_map[XK_Left & 0xFF] = Rocket::Core::Input::KI_LEFT;
	key_identifier_map[XK_Up & 0xFF] = Rocket::Core::Input::KI_UP;
	key_identifier_map[XK_Right & 0xFF] = Rocket::Core::Input::KI_RIGHT;
	key_identifier_map[XK_Down & 0xFF] = Rocket::Core::Input::KI_DOWN;
	key_identifier_map[XK_Prior & 0xFF] = Rocket::Core::Input::KI_PRIOR;
	key_identifier_map[XK_Next & 0xFF] = Rocket::Core::Input::KI_NEXT;
	key_identifier_map[XK_End & 0xFF] = Rocket::Core::Input::KI_END;
	key_identifier_map[XK_Begin & 0xFF] = Rocket::Core::Input::KI_HOME;

	key_identifier_map[XK_Print & 0xFF] = Rocket::Core::Input::KI_SNAPSHOT;
	key_identifier_map[XK_Insert & 0xFF] = Rocket::Core::Input::KI_INSERT;
	key_identifier_map[XK_Num_Lock & 0xFF] = Rocket::Core::Input::KI_NUMLOCK;

	key_identifier_map[XK_KP_Space & 0xFF] = Rocket::Core::Input::KI_SPACE;
	key_identifier_map[XK_KP_Tab & 0xFF] = Rocket::Core::Input::KI_TAB;
	key_identifier_map[XK_KP_Enter & 0xFF] = Rocket::Core::Input::KI_NUMPADENTER;
	key_identifier_map[XK_KP_F1 & 0xFF] = Rocket::Core::Input::KI_F1;
	key_identifier_map[XK_KP_F2 & 0xFF] = Rocket::Core::Input::KI_F2;
	key_identifier_map[XK_KP_F3 & 0xFF] = Rocket::Core::Input::KI_F3;
	key_identifier_map[XK_KP_F4 & 0xFF] = Rocket::Core::Input::KI_F4;
	key_identifier_map[XK_KP_Home & 0xFF] = Rocket::Core::Input::KI_NUMPAD7;
	key_identifier_map[XK_KP_Left & 0xFF] = Rocket::Core::Input::KI_NUMPAD4;
	key_identifier_map[XK_KP_Up & 0xFF] = Rocket::Core::Input::KI_NUMPAD8;
	key_identifier_map[XK_KP_Right & 0xFF] = Rocket::Core::Input::KI_NUMPAD6;
	key_identifier_map[XK_KP_Down & 0xFF] = Rocket::Core::Input::KI_NUMPAD2;
	key_identifier_map[XK_KP_Prior & 0xFF] = Rocket::Core::Input::KI_NUMPAD9;
	key_identifier_map[XK_KP_Next & 0xFF] = Rocket::Core::Input::KI_NUMPAD3;
	key_identifier_map[XK_KP_End & 0xFF] = Rocket::Core::Input::KI_NUMPAD1;
	key_identifier_map[XK_KP_Begin & 0xFF] = Rocket::Core::Input::KI_NUMPAD5;
	key_identifier_map[XK_KP_Insert & 0xFF] = Rocket::Core::Input::KI_NUMPAD0;
	key_identifier_map[XK_KP_Delete & 0xFF] = Rocket::Core::Input::KI_DECIMAL;
	key_identifier_map[XK_KP_Equal & 0xFF] = Rocket::Core::Input::KI_OEM_NEC_EQUAL;
	key_identifier_map[XK_KP_Multiply & 0xFF] = Rocket::Core::Input::KI_MULTIPLY;
	key_identifier_map[XK_KP_Add & 0xFF] = Rocket::Core::Input::KI_ADD;
	key_identifier_map[XK_KP_Separator & 0xFF] = Rocket::Core::Input::KI_SEPARATOR;
	key_identifier_map[XK_KP_Subtract & 0xFF] = Rocket::Core::Input::KI_SUBTRACT;
	key_identifier_map[XK_KP_Decimal & 0xFF] = Rocket::Core::Input::KI_DECIMAL;
	key_identifier_map[XK_KP_Divide & 0xFF] = Rocket::Core::Input::KI_DIVIDE;

	key_identifier_map[XK_F1 & 0xFF] = Rocket::Core::Input::KI_F1;
	key_identifier_map[XK_F2 & 0xFF] = Rocket::Core::Input::KI_F2;
	key_identifier_map[XK_F3 & 0xFF] = Rocket::Core::Input::KI_F3;
	key_identifier_map[XK_F4 & 0xFF] = Rocket::Core::Input::KI_F4;
	key_identifier_map[XK_F5 & 0xFF] = Rocket::Core::Input::KI_F5;
	key_identifier_map[XK_F6 & 0xFF] = Rocket::Core::Input::KI_F6;
	key_identifier_map[XK_F7 & 0xFF] = Rocket::Core::Input::KI_F7;
	key_identifier_map[XK_F8 & 0xFF] = Rocket::Core::Input::KI_F8;
	key_identifier_map[XK_F9 & 0xFF] = Rocket::Core::Input::KI_F9;
	key_identifier_map[XK_F10 & 0xFF] = Rocket::Core::Input::KI_F10;
	key_identifier_map[XK_F11 & 0xFF] = Rocket::Core::Input::KI_F11;
	key_identifier_map[XK_F12 & 0xFF] = Rocket::Core::Input::KI_F12;
	key_identifier_map[XK_F13 & 0xFF] = Rocket::Core::Input::KI_F13;
	key_identifier_map[XK_F14 & 0xFF] = Rocket::Core::Input::KI_F14;
	key_identifier_map[XK_F15 & 0xFF] = Rocket::Core::Input::KI_F15;
	key_identifier_map[XK_F16 & 0xFF] = Rocket::Core::Input::KI_F16;
	key_identifier_map[XK_F17 & 0xFF] = Rocket::Core::Input::KI_F17;
	key_identifier_map[XK_F18 & 0xFF] = Rocket::Core::Input::KI_F18;
	key_identifier_map[XK_F19 & 0xFF] = Rocket::Core::Input::KI_F19;
	key_identifier_map[XK_F20 & 0xFF] = Rocket::Core::Input::KI_F20;
	key_identifier_map[XK_F21 & 0xFF] = Rocket::Core::Input::KI_F21;
	key_identifier_map[XK_F22 & 0xFF] = Rocket::Core::Input::KI_F22;
	key_identifier_map[XK_F23 & 0xFF] = Rocket::Core::Input::KI_F23;
	key_identifier_map[XK_F24 & 0xFF] = Rocket::Core::Input::KI_F24;

	key_identifier_map[XK_Shift_L & 0xFF] = Rocket::Core::Input::KI_LSHIFT;
	key_identifier_map[XK_Shift_R & 0xFF] = Rocket::Core::Input::KI_RSHIFT;
	key_identifier_map[XK_Control_L & 0xFF] = Rocket::Core::Input::KI_LCONTROL;
	key_identifier_map[XK_Control_R & 0xFF] = Rocket::Core::Input::KI_RCONTROL;
	key_identifier_map[XK_Caps_Lock & 0xFF] = Rocket::Core::Input::KI_CAPITAL;

	key_identifier_map[XK_Alt_L & 0xFF] = Rocket::Core::Input::KI_LMENU;
	key_identifier_map[XK_Alt_R & 0xFF] = Rocket::Core::Input::KI_RMENU;

	key_identifier_map[XK_space & 0xFF] = Rocket::Core::Input::KI_SPACE;
	key_identifier_map[XK_apostrophe & 0xFF] = Rocket::Core::Input::KI_OEM_7;
	key_identifier_map[XK_comma & 0xFF] = Rocket::Core::Input::KI_OEM_COMMA;
	key_identifier_map[XK_minus & 0xFF] = Rocket::Core::Input::KI_OEM_MINUS;
	key_identifier_map[XK_period & 0xFF] = Rocket::Core::Input::KI_OEM_PERIOD;
	key_identifier_map[XK_slash & 0xFF] = Rocket::Core::Input::KI_OEM_2;
	key_identifier_map[XK_0 & 0xFF] = Rocket::Core::Input::KI_0;
	key_identifier_map[XK_1 & 0xFF] = Rocket::Core::Input::KI_1;
	key_identifier_map[XK_2 & 0xFF] = Rocket::Core::Input::KI_2;
	key_identifier_map[XK_3 & 0xFF] = Rocket::Core::Input::KI_3;
	key_identifier_map[XK_4 & 0xFF] = Rocket::Core::Input::KI_4;
	key_identifier_map[XK_5 & 0xFF] = Rocket::Core::Input::KI_5;
	key_identifier_map[XK_6 & 0xFF] = Rocket::Core::Input::KI_6;
	key_identifier_map[XK_7 & 0xFF] = Rocket::Core::Input::KI_7;
	key_identifier_map[XK_8 & 0xFF] = Rocket::Core::Input::KI_8;
	key_identifier_map[XK_9 & 0xFF] = Rocket::Core::Input::KI_9;
	key_identifier_map[XK_semicolon & 0xFF] = Rocket::Core::Input::KI_OEM_1;
	key_identifier_map[XK_equal & 0xFF] = Rocket::Core::Input::KI_OEM_PLUS;
	key_identifier_map[XK_bracketleft & 0xFF] = Rocket::Core::Input::KI_OEM_4;
	key_identifier_map[XK_backslash & 0xFF] = Rocket::Core::Input::KI_OEM_5;
	key_identifier_map[XK_bracketright & 0xFF] = Rocket::Core::Input::KI_OEM_6;
	key_identifier_map[XK_grave & 0xFF] = Rocket::Core::Input::KI_OEM_3;
	key_identifier_map[XK_a & 0xFF] = Rocket::Core::Input::KI_A;
	key_identifier_map[XK_b & 0xFF] = Rocket::Core::Input::KI_B;
	key_identifier_map[XK_c & 0xFF] = Rocket::Core::Input::KI_C;
	key_identifier_map[XK_d & 0xFF] = Rocket::Core::Input::KI_D;
	key_identifier_map[XK_e & 0xFF] = Rocket::Core::Input::KI_E;
	key_identifier_map[XK_f & 0xFF] = Rocket::Core::Input::KI_F;
	key_identifier_map[XK_g & 0xFF] = Rocket::Core::Input::KI_G;
	key_identifier_map[XK_h & 0xFF] = Rocket::Core::Input::KI_H;
	key_identifier_map[XK_i & 0xFF] = Rocket::Core::Input::KI_I;
	key_identifier_map[XK_j & 0xFF] = Rocket::Core::Input::KI_J;
	key_identifier_map[XK_k & 0xFF] = Rocket::Core::Input::KI_K;
	key_identifier_map[XK_l & 0xFF] = Rocket::Core::Input::KI_L;
	key_identifier_map[XK_m & 0xFF] = Rocket::Core::Input::KI_M;
	key_identifier_map[XK_n & 0xFF] = Rocket::Core::Input::KI_N;
	key_identifier_map[XK_o & 0xFF] = Rocket::Core::Input::KI_O;
	key_identifier_map[XK_p & 0xFF] = Rocket::Core::Input::KI_P;
	key_identifier_map[XK_q & 0xFF] = Rocket::Core::Input::KI_Q;
	key_identifier_map[XK_r & 0xFF] = Rocket::Core::Input::KI_R;
	key_identifier_map[XK_s & 0xFF] = Rocket::Core::Input::KI_S;
	key_identifier_map[XK_t & 0xFF] = Rocket::Core::Input::KI_T;
	key_identifier_map[XK_u & 0xFF] = Rocket::Core::Input::KI_U;
	key_identifier_map[XK_v & 0xFF] = Rocket::Core::Input::KI_V;
	key_identifier_map[XK_w & 0xFF] = Rocket::Core::Input::KI_W;
	key_identifier_map[XK_x & 0xFF] = Rocket::Core::Input::KI_X;
	key_identifier_map[XK_y & 0xFF] = Rocket::Core::Input::KI_Y;
	key_identifier_map[XK_z & 0xFF] = Rocket::Core::Input::KI_Z;
}
