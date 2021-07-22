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
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Debugger.h>
#include <Shell.h>
#include <string.h>

static int GetKeyModifierState();
static void InitialiseKeymap();

static const int KEYMAP_SIZE = 256;
static Rml::Input::KeyIdentifier key_identifier_map[KEYMAP_SIZE];

bool InputWin32::Initialise()
{
	InitialiseKeymap();
	return true;
}

void InputWin32::Shutdown()
{
}

void InputWin32::ProcessWindowsEvent(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	if (context == nullptr)
		return;
	
	// Process all mouse and keyboard events
	switch (message)
	{
		case WM_LBUTTONDOWN:
			context->ProcessMouseButtonDown(0, GetKeyModifierState());
			SetCapture(window);
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
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
			context->ProcessMouseMove(static_cast<int>((short)LOWORD(l_param)), static_cast<int>((short)HIWORD(l_param)), GetKeyModifierState());
			break;

		case WM_MOUSEWHEEL:
			context->ProcessMouseWheel(static_cast<float>((short) HIWORD(w_param)) / static_cast<float>(-WHEEL_DELTA), GetKeyModifierState());
			break;

		case WM_KEYDOWN:
		{
			Rml::Input::KeyIdentifier key_identifier = key_identifier_map[w_param];
			const int key_modifier_state = GetKeyModifierState();

			// Toggle debugger and set 'dp'-ratio ctrl +/-/0 keys. These global shortcuts take priority.
			if (key_identifier == Rml::Input::KI_F8)
			{
				Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
			}
			else if (key_identifier == Rml::Input::KI_0 && key_modifier_state & Rml::Input::KM_CTRL)
			{
				context->SetDensityIndependentPixelRatio(Shell::GetDensityIndependentPixelRatio());
			}
			else if (key_identifier == Rml::Input::KI_1 && key_modifier_state & Rml::Input::KM_CTRL)
			{
				context->SetDensityIndependentPixelRatio(1.f);
			}
			else if (key_identifier == Rml::Input::KI_OEM_MINUS && key_modifier_state & Rml::Input::KM_CTRL)
			{
				const float new_dp_ratio = Rml::Math::Max(context->GetDensityIndependentPixelRatio() / 1.2f, 0.5f);
				context->SetDensityIndependentPixelRatio(new_dp_ratio);
			}
			else if (key_identifier == Rml::Input::KI_OEM_PLUS && key_modifier_state & Rml::Input::KM_CTRL)
			{
				const float new_dp_ratio = Rml::Math::Min(context->GetDensityIndependentPixelRatio() * 1.2f, 2.5f);
				context->SetDensityIndependentPixelRatio(new_dp_ratio);
			}
			else
			{
				// No global shortcuts detected, submit the key to the context.
				if (context->ProcessKeyDown(key_identifier, key_modifier_state))
				{
					// The key was not consumed, check for shortcuts that are of lower priority.
					if (key_identifier == Rml::Input::KI_R && key_modifier_state & Rml::Input::KM_CTRL)
					{
						for (int i = 0; i < context->GetNumDocuments(); i++)
						{
							Rml::ElementDocument* document = context->GetDocument(i);
							const Rml::String& src = document->GetSourceURL();
							if (src.size() > 4 && src.substr(src.size() - 4) == ".rml")
							{
								document->ReloadStyleSheet();
							}
						}

					}
				}
			}
		}
		break;


		case WM_CHAR:
		{
			static wchar_t first_u16_code_unit = 0;

			const wchar_t c = (wchar_t)w_param;
			Rml::Character character = (Rml::Character)c;

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
					Rml::String utf8 = ConvertToUTF8(std::wstring{ first_u16_code_unit, c });
					character = Rml::StringUtilities::ToCharacter(utf8.data());
				}
				else if (c == '\r')
				{
					// Windows sends new-lines as carriage returns, convert to endlines.
					character = (Rml::Character)'\n';
				}

				first_u16_code_unit = 0;

				// Only send through printable characters.
				if (((char32_t)character >= 32 || character == (Rml::Character)'\n') && character != (Rml::Character)127)
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
		key_modifier_state |= Rml::Input::KM_CAPSLOCK;
	}

	if (HIWORD(GetKeyState(VK_SHIFT)) & 1)
	{
		key_modifier_state |= Rml::Input::KM_SHIFT;
	}

	if (GetKeyState(VK_NUMLOCK) & 1)
	{
		key_modifier_state |= Rml::Input::KM_NUMLOCK;
	}

	if (HIWORD(GetKeyState(VK_CONTROL)) & 1)
	{
		key_modifier_state |= Rml::Input::KM_CTRL;
	}

	if (HIWORD(GetKeyState(VK_MENU)) & 1)
	{
		key_modifier_state |= Rml::Input::KM_ALT;
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
	key_identifier_map['A'] = Rml::Input::KI_A;
	key_identifier_map['B'] = Rml::Input::KI_B;
	key_identifier_map['C'] = Rml::Input::KI_C;
	key_identifier_map['D'] = Rml::Input::KI_D;
	key_identifier_map['E'] = Rml::Input::KI_E;
	key_identifier_map['F'] = Rml::Input::KI_F;
	key_identifier_map['G'] = Rml::Input::KI_G;
	key_identifier_map['H'] = Rml::Input::KI_H;
	key_identifier_map['I'] = Rml::Input::KI_I;
	key_identifier_map['J'] = Rml::Input::KI_J;
	key_identifier_map['K'] = Rml::Input::KI_K;
	key_identifier_map['L'] = Rml::Input::KI_L;
	key_identifier_map['M'] = Rml::Input::KI_M;
	key_identifier_map['N'] = Rml::Input::KI_N;
	key_identifier_map['O'] = Rml::Input::KI_O;
	key_identifier_map['P'] = Rml::Input::KI_P;
	key_identifier_map['Q'] = Rml::Input::KI_Q;
	key_identifier_map['R'] = Rml::Input::KI_R;
	key_identifier_map['S'] = Rml::Input::KI_S;
	key_identifier_map['T'] = Rml::Input::KI_T;
	key_identifier_map['U'] = Rml::Input::KI_U;	
	key_identifier_map['V'] = Rml::Input::KI_V;
	key_identifier_map['W'] = Rml::Input::KI_W;
	key_identifier_map['X'] = Rml::Input::KI_X;
	key_identifier_map['Y'] = Rml::Input::KI_Y;
	key_identifier_map['Z'] = Rml::Input::KI_Z;

	key_identifier_map['0'] = Rml::Input::KI_0;
	key_identifier_map['1'] = Rml::Input::KI_1;
	key_identifier_map['2'] = Rml::Input::KI_2;
	key_identifier_map['3'] = Rml::Input::KI_3;
	key_identifier_map['4'] = Rml::Input::KI_4;
	key_identifier_map['5'] = Rml::Input::KI_5;
	key_identifier_map['6'] = Rml::Input::KI_6;
	key_identifier_map['7'] = Rml::Input::KI_7;
	key_identifier_map['8'] = Rml::Input::KI_8;
	key_identifier_map['9'] = Rml::Input::KI_9;

	key_identifier_map[VK_BACK] = Rml::Input::KI_BACK;
	key_identifier_map[VK_TAB] = Rml::Input::KI_TAB;

	key_identifier_map[VK_CLEAR] = Rml::Input::KI_CLEAR;
	key_identifier_map[VK_RETURN] = Rml::Input::KI_RETURN;

	key_identifier_map[VK_PAUSE] = Rml::Input::KI_PAUSE;
	key_identifier_map[VK_CAPITAL] = Rml::Input::KI_CAPITAL;

	key_identifier_map[VK_KANA] = Rml::Input::KI_KANA;
	key_identifier_map[VK_HANGUL] = Rml::Input::KI_HANGUL;
	key_identifier_map[VK_JUNJA] = Rml::Input::KI_JUNJA;
	key_identifier_map[VK_FINAL] = Rml::Input::KI_FINAL;
	key_identifier_map[VK_HANJA] = Rml::Input::KI_HANJA;
	key_identifier_map[VK_KANJI] = Rml::Input::KI_KANJI;

	key_identifier_map[VK_ESCAPE] = Rml::Input::KI_ESCAPE;

	key_identifier_map[VK_CONVERT] = Rml::Input::KI_CONVERT;
	key_identifier_map[VK_NONCONVERT] = Rml::Input::KI_NONCONVERT;
	key_identifier_map[VK_ACCEPT] = Rml::Input::KI_ACCEPT;
	key_identifier_map[VK_MODECHANGE] = Rml::Input::KI_MODECHANGE;

	key_identifier_map[VK_SPACE] = Rml::Input::KI_SPACE;
	key_identifier_map[VK_PRIOR] = Rml::Input::KI_PRIOR;
	key_identifier_map[VK_NEXT] = Rml::Input::KI_NEXT;
	key_identifier_map[VK_END] = Rml::Input::KI_END;
	key_identifier_map[VK_HOME] = Rml::Input::KI_HOME;
	key_identifier_map[VK_LEFT] = Rml::Input::KI_LEFT;
	key_identifier_map[VK_UP] = Rml::Input::KI_UP;
	key_identifier_map[VK_RIGHT] = Rml::Input::KI_RIGHT;
	key_identifier_map[VK_DOWN] = Rml::Input::KI_DOWN;
	key_identifier_map[VK_SELECT] = Rml::Input::KI_SELECT;
	key_identifier_map[VK_PRINT] = Rml::Input::KI_PRINT;
	key_identifier_map[VK_EXECUTE] = Rml::Input::KI_EXECUTE;
	key_identifier_map[VK_SNAPSHOT] = Rml::Input::KI_SNAPSHOT;
	key_identifier_map[VK_INSERT] = Rml::Input::KI_INSERT;
	key_identifier_map[VK_DELETE] = Rml::Input::KI_DELETE;
	key_identifier_map[VK_HELP] = Rml::Input::KI_HELP;

	key_identifier_map[VK_LWIN] = Rml::Input::KI_LWIN;
	key_identifier_map[VK_RWIN] = Rml::Input::KI_RWIN;
	key_identifier_map[VK_APPS] = Rml::Input::KI_APPS;

	key_identifier_map[VK_SLEEP] = Rml::Input::KI_SLEEP;

	key_identifier_map[VK_NUMPAD0] = Rml::Input::KI_NUMPAD0;
	key_identifier_map[VK_NUMPAD1] = Rml::Input::KI_NUMPAD1;
	key_identifier_map[VK_NUMPAD2] = Rml::Input::KI_NUMPAD2;
	key_identifier_map[VK_NUMPAD3] = Rml::Input::KI_NUMPAD3;
	key_identifier_map[VK_NUMPAD4] = Rml::Input::KI_NUMPAD4;
	key_identifier_map[VK_NUMPAD5] = Rml::Input::KI_NUMPAD5;
	key_identifier_map[VK_NUMPAD6] = Rml::Input::KI_NUMPAD6;
	key_identifier_map[VK_NUMPAD7] = Rml::Input::KI_NUMPAD7;
	key_identifier_map[VK_NUMPAD8] = Rml::Input::KI_NUMPAD8;
	key_identifier_map[VK_NUMPAD9] = Rml::Input::KI_NUMPAD9;
	key_identifier_map[VK_MULTIPLY] = Rml::Input::KI_MULTIPLY;
	key_identifier_map[VK_ADD] = Rml::Input::KI_ADD;
	key_identifier_map[VK_SEPARATOR] = Rml::Input::KI_SEPARATOR;
	key_identifier_map[VK_SUBTRACT] = Rml::Input::KI_SUBTRACT;
	key_identifier_map[VK_DECIMAL] = Rml::Input::KI_DECIMAL;
	key_identifier_map[VK_DIVIDE] = Rml::Input::KI_DIVIDE;
	key_identifier_map[VK_F1] = Rml::Input::KI_F1;
	key_identifier_map[VK_F2] = Rml::Input::KI_F2;
	key_identifier_map[VK_F3] = Rml::Input::KI_F3;
	key_identifier_map[VK_F4] = Rml::Input::KI_F4;
	key_identifier_map[VK_F5] = Rml::Input::KI_F5;
	key_identifier_map[VK_F6] = Rml::Input::KI_F6;
	key_identifier_map[VK_F7] = Rml::Input::KI_F7;
	key_identifier_map[VK_F8] = Rml::Input::KI_F8;
	key_identifier_map[VK_F9] = Rml::Input::KI_F9;
	key_identifier_map[VK_F10] = Rml::Input::KI_F10;
	key_identifier_map[VK_F11] = Rml::Input::KI_F11;
	key_identifier_map[VK_F12] = Rml::Input::KI_F12;
	key_identifier_map[VK_F13] = Rml::Input::KI_F13;
	key_identifier_map[VK_F14] = Rml::Input::KI_F14;
	key_identifier_map[VK_F15] = Rml::Input::KI_F15;
	key_identifier_map[VK_F16] = Rml::Input::KI_F16;
	key_identifier_map[VK_F17] = Rml::Input::KI_F17;
	key_identifier_map[VK_F18] = Rml::Input::KI_F18;
	key_identifier_map[VK_F19] = Rml::Input::KI_F19;
	key_identifier_map[VK_F20] = Rml::Input::KI_F20;
	key_identifier_map[VK_F21] = Rml::Input::KI_F21;
	key_identifier_map[VK_F22] = Rml::Input::KI_F22;
	key_identifier_map[VK_F23] = Rml::Input::KI_F23;
	key_identifier_map[VK_F24] = Rml::Input::KI_F24;

	key_identifier_map[VK_NUMLOCK] = Rml::Input::KI_NUMLOCK;
	key_identifier_map[VK_SCROLL] = Rml::Input::KI_SCROLL;

	key_identifier_map[VK_OEM_NEC_EQUAL] = Rml::Input::KI_OEM_NEC_EQUAL;

	key_identifier_map[VK_OEM_FJ_JISHO] = Rml::Input::KI_OEM_FJ_JISHO;
	key_identifier_map[VK_OEM_FJ_MASSHOU] = Rml::Input::KI_OEM_FJ_MASSHOU;
	key_identifier_map[VK_OEM_FJ_TOUROKU] = Rml::Input::KI_OEM_FJ_TOUROKU;
	key_identifier_map[VK_OEM_FJ_LOYA] = Rml::Input::KI_OEM_FJ_LOYA;
	key_identifier_map[VK_OEM_FJ_ROYA] = Rml::Input::KI_OEM_FJ_ROYA;

	key_identifier_map[VK_SHIFT] = Rml::Input::KI_LSHIFT;
	key_identifier_map[VK_CONTROL] = Rml::Input::KI_LCONTROL;
	key_identifier_map[VK_MENU] = Rml::Input::KI_LMENU;

	key_identifier_map[VK_BROWSER_BACK] = Rml::Input::KI_BROWSER_BACK;
	key_identifier_map[VK_BROWSER_FORWARD] = Rml::Input::KI_BROWSER_FORWARD;
	key_identifier_map[VK_BROWSER_REFRESH] = Rml::Input::KI_BROWSER_REFRESH;
	key_identifier_map[VK_BROWSER_STOP] = Rml::Input::KI_BROWSER_STOP;
	key_identifier_map[VK_BROWSER_SEARCH] = Rml::Input::KI_BROWSER_SEARCH;
	key_identifier_map[VK_BROWSER_FAVORITES] = Rml::Input::KI_BROWSER_FAVORITES;
	key_identifier_map[VK_BROWSER_HOME] = Rml::Input::KI_BROWSER_HOME;

	key_identifier_map[VK_VOLUME_MUTE] = Rml::Input::KI_VOLUME_MUTE;
	key_identifier_map[VK_VOLUME_DOWN] = Rml::Input::KI_VOLUME_DOWN;
	key_identifier_map[VK_VOLUME_UP] = Rml::Input::KI_VOLUME_UP;
	key_identifier_map[VK_MEDIA_NEXT_TRACK] = Rml::Input::KI_MEDIA_NEXT_TRACK;
	key_identifier_map[VK_MEDIA_PREV_TRACK] = Rml::Input::KI_MEDIA_PREV_TRACK;
	key_identifier_map[VK_MEDIA_STOP] = Rml::Input::KI_MEDIA_STOP;
	key_identifier_map[VK_MEDIA_PLAY_PAUSE] = Rml::Input::KI_MEDIA_PLAY_PAUSE;
	key_identifier_map[VK_LAUNCH_MAIL] = Rml::Input::KI_LAUNCH_MAIL;
	key_identifier_map[VK_LAUNCH_MEDIA_SELECT] = Rml::Input::KI_LAUNCH_MEDIA_SELECT;
	key_identifier_map[VK_LAUNCH_APP1] = Rml::Input::KI_LAUNCH_APP1;
	key_identifier_map[VK_LAUNCH_APP2] = Rml::Input::KI_LAUNCH_APP2;

	key_identifier_map[VK_OEM_1] = Rml::Input::KI_OEM_1;
	key_identifier_map[VK_OEM_PLUS] = Rml::Input::KI_OEM_PLUS;
	key_identifier_map[VK_OEM_COMMA] = Rml::Input::KI_OEM_COMMA;
	key_identifier_map[VK_OEM_MINUS] = Rml::Input::KI_OEM_MINUS;
	key_identifier_map[VK_OEM_PERIOD] = Rml::Input::KI_OEM_PERIOD;
	key_identifier_map[VK_OEM_2] = Rml::Input::KI_OEM_2;
	key_identifier_map[VK_OEM_3] = Rml::Input::KI_OEM_3;

	key_identifier_map[VK_OEM_4] = Rml::Input::KI_OEM_4;
	key_identifier_map[VK_OEM_5] = Rml::Input::KI_OEM_5;
	key_identifier_map[VK_OEM_6] = Rml::Input::KI_OEM_6;
	key_identifier_map[VK_OEM_7] = Rml::Input::KI_OEM_7;
	key_identifier_map[VK_OEM_8] = Rml::Input::KI_OEM_8;

	key_identifier_map[VK_OEM_AX] = Rml::Input::KI_OEM_AX;
	key_identifier_map[VK_OEM_102] = Rml::Input::KI_OEM_102;
	key_identifier_map[VK_ICO_HELP] = Rml::Input::KI_ICO_HELP;
	key_identifier_map[VK_ICO_00] = Rml::Input::KI_ICO_00;

	key_identifier_map[VK_PROCESSKEY] = Rml::Input::KI_PROCESSKEY;

	key_identifier_map[VK_ICO_CLEAR] = Rml::Input::KI_ICO_CLEAR;

	key_identifier_map[VK_ATTN] = Rml::Input::KI_ATTN;
	key_identifier_map[VK_CRSEL] = Rml::Input::KI_CRSEL;
	key_identifier_map[VK_EXSEL] = Rml::Input::KI_EXSEL;
	key_identifier_map[VK_EREOF] = Rml::Input::KI_EREOF;
	key_identifier_map[VK_PLAY] = Rml::Input::KI_PLAY;
	key_identifier_map[VK_ZOOM] = Rml::Input::KI_ZOOM;
	key_identifier_map[VK_PA1] = Rml::Input::KI_PA1;
	key_identifier_map[VK_OEM_CLEAR] = Rml::Input::KI_OEM_CLEAR;
}

Rml::String ConvertToUTF8(const std::wstring& wstr)
{
	const int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
	Rml::String str(count, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
	return str;
}

std::wstring ConvertToUTF16(const Rml::String& str)
{
	const int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
	std::wstring wstr(count, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], count);
	return wstr;
}
