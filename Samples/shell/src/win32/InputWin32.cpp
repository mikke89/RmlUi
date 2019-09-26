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

#include <win32/InputWin32.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Debugger.h>
#include <Shell.h>

static int GetKeyModifierState();
static void InitialiseKeymap();

static const int KEYMAP_SIZE = 256;
static Rml::Core::Input::KeyIdentifier key_identifier_map[KEYMAP_SIZE];

bool InputWin32::Initialise()
{
	InitialiseKeymap();
	return true;
}

void InputWin32::Shutdown()
{
}

void InputWin32::ProcessWindowsEvent(UINT message, WPARAM w_param, LPARAM l_param)
{
	if (context == nullptr)
		return;
	
	// Process all mouse and keyboard events
	switch (message)
	{
		case WM_LBUTTONDOWN:
			context->ProcessMouseButtonDown(0, GetKeyModifierState());
			break;

		case WM_LBUTTONUP:
			context->ProcessMouseButtonUp(0, GetKeyModifierState());
			break;

		case WM_RBUTTONDOWN:
			context->ProcessMouseButtonDown(1, GetKeyModifierState());
			break;

		case WM_RBUTTONUP:
			context->ProcessMouseButtonUp(1, GetKeyModifierState());
			break;

		case WM_MBUTTONDOWN:
			context->ProcessMouseButtonDown(2, GetKeyModifierState());
			break;

		case WM_MBUTTONUP:
			context->ProcessMouseButtonUp(2, GetKeyModifierState());
			break;

		case WM_MOUSEMOVE:
			context->ProcessMouseMove(LOWORD(l_param), HIWORD(l_param), GetKeyModifierState());
			break;

		case WM_MOUSEWHEEL:
			context->ProcessMouseWheel(static_cast<float>((short) HIWORD(w_param)) / static_cast<float>(-WHEEL_DELTA), GetKeyModifierState());
			break;

		case WM_KEYDOWN:
		{
			Rml::Core::Input::KeyIdentifier key_identifier = key_identifier_map[w_param];
			int key_modifier_state = GetKeyModifierState();

			// Check for F8 to toggle the debugger.
			if (key_identifier == Rml::Core::Input::KI_F8)
			{
				Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
				break;
			}

			context->ProcessKeyDown(key_identifier, key_modifier_state);
		}
		break;


		case WM_CHAR:
		{
			static char16_t first_u16_code_unit = 0;

			char16_t c = (char16_t)w_param;
			Rml::Core::Character character = (Rml::Core::Character)c;

			// Windows sends two-wide characters as two messages.
			if (c >= 0xD800 && c < 0xDC00)
			{
				// First 16-bit code unit of a two-wide character.
				first_u16_code_unit = c;
			}
			else
			{
				if (c >= 0xDC00 && c < 0xE000 && first_u16_code_unit != 0)
				{
					// Second 16-bit code unit of a two-wide character.
					Rml::Core::String utf8 = Rml::Core::StringUtilities::ToUTF8({ first_u16_code_unit, c });
					character = Rml::Core::StringUtilities::ToCharacter(utf8.data());
				}
				else if (c == '\r')
				{
					// Windows sends new-lines as carriage returns, convert to endlines.
					character = (Rml::Core::Character)'\n';
				}

				first_u16_code_unit = 0;

				// Only send through printable characters.
				if ((char32_t)character >= 32 || character == (Rml::Core::Character)'\n')
					context->ProcessTextInput(character);
			}
		}
		break;

		case WM_KEYUP:
			context->ProcessKeyUp(key_identifier_map[w_param], GetKeyModifierState());
			break;
	}
}

static int GetKeyModifierState()
{
	int key_modifier_state = 0;

	// Query the state of all modifier keys
	if (GetKeyState(VK_CAPITAL) & 1)
	{
		key_modifier_state |= Rml::Core::Input::KM_CAPSLOCK;
	}

	if (HIWORD(GetKeyState(VK_SHIFT)) & 1)
	{
		key_modifier_state |= Rml::Core::Input::KM_SHIFT;
	}

	if (GetKeyState(VK_NUMLOCK) & 1)
	{
		key_modifier_state |= Rml::Core::Input::KM_NUMLOCK;
	}

	if (HIWORD(GetKeyState(VK_CONTROL)) & 1)
	{
		key_modifier_state |= Rml::Core::Input::KM_CTRL;
	}

	if (HIWORD(GetKeyState(VK_MENU)) & 1)
	{
		key_modifier_state |= Rml::Core::Input::KM_ALT;
	}

	return key_modifier_state;
}

// These are defined in winuser.h of MinGW 64 but are missing from MinGW 32
// Visual Studio has them by default
#if defined(__MINGW32__)  && !defined(__MINGW64__)
#define VK_OEM_NEC_EQUAL 0x92
#define VK_OEM_FJ_JISHO 0x92
#define VK_OEM_FJ_MASSHOU 0x93
#define VK_OEM_FJ_TOUROKU 0x94
#define VK_OEM_FJ_LOYA 0x95
#define VK_OEM_FJ_ROYA 0x96
#define VK_OEM_AX 0xE1
#define VK_ICO_HELP 0xE3
#define VK_ICO_00 0xE4
#define VK_ICO_CLEAR 0xE6
#endif // !defined(__MINGW32__)  || defined(__MINGW64__)

static void InitialiseKeymap()
{
	// Initialise the key map with default values.
	memset(key_identifier_map, 0, sizeof(key_identifier_map));
	
	// Assign individual values.
	key_identifier_map['A'] = Rml::Core::Input::KI_A;
	key_identifier_map['B'] = Rml::Core::Input::KI_B;
	key_identifier_map['C'] = Rml::Core::Input::KI_C;
	key_identifier_map['D'] = Rml::Core::Input::KI_D;
	key_identifier_map['E'] = Rml::Core::Input::KI_E;
	key_identifier_map['F'] = Rml::Core::Input::KI_F;
	key_identifier_map['G'] = Rml::Core::Input::KI_G;
	key_identifier_map['H'] = Rml::Core::Input::KI_H;
	key_identifier_map['I'] = Rml::Core::Input::KI_I;
	key_identifier_map['J'] = Rml::Core::Input::KI_J;
	key_identifier_map['K'] = Rml::Core::Input::KI_K;
	key_identifier_map['L'] = Rml::Core::Input::KI_L;
	key_identifier_map['M'] = Rml::Core::Input::KI_M;
	key_identifier_map['N'] = Rml::Core::Input::KI_N;
	key_identifier_map['O'] = Rml::Core::Input::KI_O;
	key_identifier_map['P'] = Rml::Core::Input::KI_P;
	key_identifier_map['Q'] = Rml::Core::Input::KI_Q;
	key_identifier_map['R'] = Rml::Core::Input::KI_R;
	key_identifier_map['S'] = Rml::Core::Input::KI_S;
	key_identifier_map['T'] = Rml::Core::Input::KI_T;
	key_identifier_map['U'] = Rml::Core::Input::KI_U;	
	key_identifier_map['V'] = Rml::Core::Input::KI_V;
	key_identifier_map['W'] = Rml::Core::Input::KI_W;
	key_identifier_map['X'] = Rml::Core::Input::KI_X;
	key_identifier_map['Y'] = Rml::Core::Input::KI_Y;
	key_identifier_map['Z'] = Rml::Core::Input::KI_Z;

	key_identifier_map['0'] = Rml::Core::Input::KI_0;
	key_identifier_map['1'] = Rml::Core::Input::KI_1;
	key_identifier_map['2'] = Rml::Core::Input::KI_2;
	key_identifier_map['3'] = Rml::Core::Input::KI_3;
	key_identifier_map['4'] = Rml::Core::Input::KI_4;
	key_identifier_map['5'] = Rml::Core::Input::KI_5;
	key_identifier_map['6'] = Rml::Core::Input::KI_6;
	key_identifier_map['7'] = Rml::Core::Input::KI_7;
	key_identifier_map['8'] = Rml::Core::Input::KI_8;
	key_identifier_map['9'] = Rml::Core::Input::KI_9;

	key_identifier_map[VK_BACK] = Rml::Core::Input::KI_BACK;
	key_identifier_map[VK_TAB] = Rml::Core::Input::KI_TAB;

	key_identifier_map[VK_CLEAR] = Rml::Core::Input::KI_CLEAR;
	key_identifier_map[VK_RETURN] = Rml::Core::Input::KI_RETURN;

	key_identifier_map[VK_PAUSE] = Rml::Core::Input::KI_PAUSE;
	key_identifier_map[VK_CAPITAL] = Rml::Core::Input::KI_CAPITAL;

	key_identifier_map[VK_KANA] = Rml::Core::Input::KI_KANA;
	key_identifier_map[VK_HANGUL] = Rml::Core::Input::KI_HANGUL;
	key_identifier_map[VK_JUNJA] = Rml::Core::Input::KI_JUNJA;
	key_identifier_map[VK_FINAL] = Rml::Core::Input::KI_FINAL;
	key_identifier_map[VK_HANJA] = Rml::Core::Input::KI_HANJA;
	key_identifier_map[VK_KANJI] = Rml::Core::Input::KI_KANJI;

	key_identifier_map[VK_ESCAPE] = Rml::Core::Input::KI_ESCAPE;

	key_identifier_map[VK_CONVERT] = Rml::Core::Input::KI_CONVERT;
	key_identifier_map[VK_NONCONVERT] = Rml::Core::Input::KI_NONCONVERT;
	key_identifier_map[VK_ACCEPT] = Rml::Core::Input::KI_ACCEPT;
	key_identifier_map[VK_MODECHANGE] = Rml::Core::Input::KI_MODECHANGE;

	key_identifier_map[VK_SPACE] = Rml::Core::Input::KI_SPACE;
	key_identifier_map[VK_PRIOR] = Rml::Core::Input::KI_PRIOR;
	key_identifier_map[VK_NEXT] = Rml::Core::Input::KI_NEXT;
	key_identifier_map[VK_END] = Rml::Core::Input::KI_END;
	key_identifier_map[VK_HOME] = Rml::Core::Input::KI_HOME;
	key_identifier_map[VK_LEFT] = Rml::Core::Input::KI_LEFT;
	key_identifier_map[VK_UP] = Rml::Core::Input::KI_UP;
	key_identifier_map[VK_RIGHT] = Rml::Core::Input::KI_RIGHT;
	key_identifier_map[VK_DOWN] = Rml::Core::Input::KI_DOWN;
	key_identifier_map[VK_SELECT] = Rml::Core::Input::KI_SELECT;
	key_identifier_map[VK_PRINT] = Rml::Core::Input::KI_PRINT;
	key_identifier_map[VK_EXECUTE] = Rml::Core::Input::KI_EXECUTE;
	key_identifier_map[VK_SNAPSHOT] = Rml::Core::Input::KI_SNAPSHOT;
	key_identifier_map[VK_INSERT] = Rml::Core::Input::KI_INSERT;
	key_identifier_map[VK_DELETE] = Rml::Core::Input::KI_DELETE;
	key_identifier_map[VK_HELP] = Rml::Core::Input::KI_HELP;

	key_identifier_map[VK_LWIN] = Rml::Core::Input::KI_LWIN;
	key_identifier_map[VK_RWIN] = Rml::Core::Input::KI_RWIN;
	key_identifier_map[VK_APPS] = Rml::Core::Input::KI_APPS;

	key_identifier_map[VK_SLEEP] = Rml::Core::Input::KI_SLEEP;

	key_identifier_map[VK_NUMPAD0] = Rml::Core::Input::KI_NUMPAD0;
	key_identifier_map[VK_NUMPAD1] = Rml::Core::Input::KI_NUMPAD1;
	key_identifier_map[VK_NUMPAD2] = Rml::Core::Input::KI_NUMPAD2;
	key_identifier_map[VK_NUMPAD3] = Rml::Core::Input::KI_NUMPAD3;
	key_identifier_map[VK_NUMPAD4] = Rml::Core::Input::KI_NUMPAD4;
	key_identifier_map[VK_NUMPAD5] = Rml::Core::Input::KI_NUMPAD5;
	key_identifier_map[VK_NUMPAD6] = Rml::Core::Input::KI_NUMPAD6;
	key_identifier_map[VK_NUMPAD7] = Rml::Core::Input::KI_NUMPAD7;
	key_identifier_map[VK_NUMPAD8] = Rml::Core::Input::KI_NUMPAD8;
	key_identifier_map[VK_NUMPAD9] = Rml::Core::Input::KI_NUMPAD9;
	key_identifier_map[VK_MULTIPLY] = Rml::Core::Input::KI_MULTIPLY;
	key_identifier_map[VK_ADD] = Rml::Core::Input::KI_ADD;
	key_identifier_map[VK_SEPARATOR] = Rml::Core::Input::KI_SEPARATOR;
	key_identifier_map[VK_SUBTRACT] = Rml::Core::Input::KI_SUBTRACT;
	key_identifier_map[VK_DECIMAL] = Rml::Core::Input::KI_DECIMAL;
	key_identifier_map[VK_DIVIDE] = Rml::Core::Input::KI_DIVIDE;
	key_identifier_map[VK_F1] = Rml::Core::Input::KI_F1;
	key_identifier_map[VK_F2] = Rml::Core::Input::KI_F2;
	key_identifier_map[VK_F3] = Rml::Core::Input::KI_F3;
	key_identifier_map[VK_F4] = Rml::Core::Input::KI_F4;
	key_identifier_map[VK_F5] = Rml::Core::Input::KI_F5;
	key_identifier_map[VK_F6] = Rml::Core::Input::KI_F6;
	key_identifier_map[VK_F7] = Rml::Core::Input::KI_F7;
	key_identifier_map[VK_F8] = Rml::Core::Input::KI_F8;
	key_identifier_map[VK_F9] = Rml::Core::Input::KI_F9;
	key_identifier_map[VK_F10] = Rml::Core::Input::KI_F10;
	key_identifier_map[VK_F11] = Rml::Core::Input::KI_F11;
	key_identifier_map[VK_F12] = Rml::Core::Input::KI_F12;
	key_identifier_map[VK_F13] = Rml::Core::Input::KI_F13;
	key_identifier_map[VK_F14] = Rml::Core::Input::KI_F14;
	key_identifier_map[VK_F15] = Rml::Core::Input::KI_F15;
	key_identifier_map[VK_F16] = Rml::Core::Input::KI_F16;
	key_identifier_map[VK_F17] = Rml::Core::Input::KI_F17;
	key_identifier_map[VK_F18] = Rml::Core::Input::KI_F18;
	key_identifier_map[VK_F19] = Rml::Core::Input::KI_F19;
	key_identifier_map[VK_F20] = Rml::Core::Input::KI_F20;
	key_identifier_map[VK_F21] = Rml::Core::Input::KI_F21;
	key_identifier_map[VK_F22] = Rml::Core::Input::KI_F22;
	key_identifier_map[VK_F23] = Rml::Core::Input::KI_F23;
	key_identifier_map[VK_F24] = Rml::Core::Input::KI_F24;

	key_identifier_map[VK_NUMLOCK] = Rml::Core::Input::KI_NUMLOCK;
	key_identifier_map[VK_SCROLL] = Rml::Core::Input::KI_SCROLL;

	key_identifier_map[VK_OEM_NEC_EQUAL] = Rml::Core::Input::KI_OEM_NEC_EQUAL;

	key_identifier_map[VK_OEM_FJ_JISHO] = Rml::Core::Input::KI_OEM_FJ_JISHO;
	key_identifier_map[VK_OEM_FJ_MASSHOU] = Rml::Core::Input::KI_OEM_FJ_MASSHOU;
	key_identifier_map[VK_OEM_FJ_TOUROKU] = Rml::Core::Input::KI_OEM_FJ_TOUROKU;
	key_identifier_map[VK_OEM_FJ_LOYA] = Rml::Core::Input::KI_OEM_FJ_LOYA;
	key_identifier_map[VK_OEM_FJ_ROYA] = Rml::Core::Input::KI_OEM_FJ_ROYA;

	key_identifier_map[VK_SHIFT] = Rml::Core::Input::KI_LSHIFT;
	key_identifier_map[VK_CONTROL] = Rml::Core::Input::KI_LCONTROL;
	key_identifier_map[VK_MENU] = Rml::Core::Input::KI_LMENU;

	key_identifier_map[VK_BROWSER_BACK] = Rml::Core::Input::KI_BROWSER_BACK;
	key_identifier_map[VK_BROWSER_FORWARD] = Rml::Core::Input::KI_BROWSER_FORWARD;
	key_identifier_map[VK_BROWSER_REFRESH] = Rml::Core::Input::KI_BROWSER_REFRESH;
	key_identifier_map[VK_BROWSER_STOP] = Rml::Core::Input::KI_BROWSER_STOP;
	key_identifier_map[VK_BROWSER_SEARCH] = Rml::Core::Input::KI_BROWSER_SEARCH;
	key_identifier_map[VK_BROWSER_FAVORITES] = Rml::Core::Input::KI_BROWSER_FAVORITES;
	key_identifier_map[VK_BROWSER_HOME] = Rml::Core::Input::KI_BROWSER_HOME;

	key_identifier_map[VK_VOLUME_MUTE] = Rml::Core::Input::KI_VOLUME_MUTE;
	key_identifier_map[VK_VOLUME_DOWN] = Rml::Core::Input::KI_VOLUME_DOWN;
	key_identifier_map[VK_VOLUME_UP] = Rml::Core::Input::KI_VOLUME_UP;
	key_identifier_map[VK_MEDIA_NEXT_TRACK] = Rml::Core::Input::KI_MEDIA_NEXT_TRACK;
	key_identifier_map[VK_MEDIA_PREV_TRACK] = Rml::Core::Input::KI_MEDIA_PREV_TRACK;
	key_identifier_map[VK_MEDIA_STOP] = Rml::Core::Input::KI_MEDIA_STOP;
	key_identifier_map[VK_MEDIA_PLAY_PAUSE] = Rml::Core::Input::KI_MEDIA_PLAY_PAUSE;
	key_identifier_map[VK_LAUNCH_MAIL] = Rml::Core::Input::KI_LAUNCH_MAIL;
	key_identifier_map[VK_LAUNCH_MEDIA_SELECT] = Rml::Core::Input::KI_LAUNCH_MEDIA_SELECT;
	key_identifier_map[VK_LAUNCH_APP1] = Rml::Core::Input::KI_LAUNCH_APP1;
	key_identifier_map[VK_LAUNCH_APP2] = Rml::Core::Input::KI_LAUNCH_APP2;

	key_identifier_map[VK_OEM_1] = Rml::Core::Input::KI_OEM_1;
	key_identifier_map[VK_OEM_PLUS] = Rml::Core::Input::KI_OEM_PLUS;
	key_identifier_map[VK_OEM_COMMA] = Rml::Core::Input::KI_OEM_COMMA;
	key_identifier_map[VK_OEM_MINUS] = Rml::Core::Input::KI_OEM_MINUS;
	key_identifier_map[VK_OEM_PERIOD] = Rml::Core::Input::KI_OEM_PERIOD;
	key_identifier_map[VK_OEM_2] = Rml::Core::Input::KI_OEM_2;
	key_identifier_map[VK_OEM_3] = Rml::Core::Input::KI_OEM_3;

	key_identifier_map[VK_OEM_4] = Rml::Core::Input::KI_OEM_4;
	key_identifier_map[VK_OEM_5] = Rml::Core::Input::KI_OEM_5;
	key_identifier_map[VK_OEM_6] = Rml::Core::Input::KI_OEM_6;
	key_identifier_map[VK_OEM_7] = Rml::Core::Input::KI_OEM_7;
	key_identifier_map[VK_OEM_8] = Rml::Core::Input::KI_OEM_8;

	key_identifier_map[VK_OEM_AX] = Rml::Core::Input::KI_OEM_AX;
	key_identifier_map[VK_OEM_102] = Rml::Core::Input::KI_OEM_102;
	key_identifier_map[VK_ICO_HELP] = Rml::Core::Input::KI_ICO_HELP;
	key_identifier_map[VK_ICO_00] = Rml::Core::Input::KI_ICO_00;

	key_identifier_map[VK_PROCESSKEY] = Rml::Core::Input::KI_PROCESSKEY;

	key_identifier_map[VK_ICO_CLEAR] = Rml::Core::Input::KI_ICO_CLEAR;

	key_identifier_map[VK_ATTN] = Rml::Core::Input::KI_ATTN;
	key_identifier_map[VK_CRSEL] = Rml::Core::Input::KI_CRSEL;
	key_identifier_map[VK_EXSEL] = Rml::Core::Input::KI_EXSEL;
	key_identifier_map[VK_EREOF] = Rml::Core::Input::KI_EREOF;
	key_identifier_map[VK_PLAY] = Rml::Core::Input::KI_PLAY;
	key_identifier_map[VK_ZOOM] = Rml::Core::Input::KI_ZOOM;
	key_identifier_map[VK_PA1] = Rml::Core::Input::KI_PA1;
	key_identifier_map[VK_OEM_CLEAR] = Rml::Core::Input::KI_OEM_CLEAR;
}
