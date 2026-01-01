#include "RmlUi_Platform_Win32.h"
#include "RmlUi_Include_Windows.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/TextInputContext.h>
#include <RmlUi/Core/TextInputHandler.h>
#include <string.h>

// Used to interact with the input method editor (IME). Users of MinGW should manually link to this.
#ifdef _MSC_VER
	#pragma comment(lib, "imm32")
#endif

SystemInterface_Win32::SystemInterface_Win32()
{
	LARGE_INTEGER time_ticks_per_second;
	QueryPerformanceFrequency(&time_ticks_per_second);
	QueryPerformanceCounter(&time_startup);

	time_frequency = 1.0 / (double)time_ticks_per_second.QuadPart;

	// Load cursors
	cursor_default = LoadCursor(nullptr, IDC_ARROW);
	cursor_move = LoadCursor(nullptr, IDC_SIZEALL);
	cursor_pointer = LoadCursor(nullptr, IDC_HAND);
	cursor_resize = LoadCursor(nullptr, IDC_SIZENWSE);
	cursor_cross = LoadCursor(nullptr, IDC_CROSS);
	cursor_text = LoadCursor(nullptr, IDC_IBEAM);
	cursor_unavailable = LoadCursor(nullptr, IDC_NO);
}

SystemInterface_Win32::~SystemInterface_Win32() = default;

void SystemInterface_Win32::SetWindow(HWND in_window_handle)
{
	window_handle = in_window_handle;
}

double SystemInterface_Win32::GetElapsedTime()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return double(counter.QuadPart - time_startup.QuadPart) * time_frequency;
}

void SystemInterface_Win32::SetMouseCursor(const Rml::String& cursor_name)
{
	if (window_handle)
	{
		HCURSOR cursor_handle = nullptr;
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
		else if (cursor_name == "unavailable")
			cursor_handle = cursor_unavailable;
		else if (Rml::StringUtilities::StartsWith(cursor_name, "rmlui-scroll"))
			cursor_handle = cursor_move;

		if (cursor_handle)
		{
			SetCursor(cursor_handle);
			SetClassLongPtrA(window_handle, GCLP_HCURSOR, (LONG_PTR)cursor_handle);
		}
	}
}

void SystemInterface_Win32::SetClipboardText(const Rml::String& text_utf8)
{
	if (window_handle)
	{
		if (!OpenClipboard(window_handle))
			return;

		EmptyClipboard();

		const std::wstring text = RmlWin32::ConvertToUTF16(text_utf8);
		const size_t size = sizeof(wchar_t) * (text.size() + 1);

		HGLOBAL clipboard_data = GlobalAlloc(GMEM_FIXED, size);
		memcpy(clipboard_data, text.data(), size);

		if (SetClipboardData(CF_UNICODETEXT, clipboard_data) == nullptr)
		{
			CloseClipboard();
			GlobalFree(clipboard_data);
		}
		else
			CloseClipboard();
	}
}

void SystemInterface_Win32::GetClipboardText(Rml::String& text)
{
	if (window_handle)
	{
		if (!OpenClipboard(window_handle))
			return;

		HANDLE clipboard_data = GetClipboardData(CF_UNICODETEXT);
		if (clipboard_data == nullptr)
		{
			CloseClipboard();
			return;
		}

		const wchar_t* clipboard_text = (const wchar_t*)GlobalLock(clipboard_data);
		if (clipboard_text)
			text = RmlWin32::ConvertToUTF8(clipboard_text);
		GlobalUnlock(clipboard_data);

		CloseClipboard();
	}
}

void SystemInterface_Win32::ActivateKeyboard(Rml::Vector2f caret_position, float line_height)
{
	HIMC himc = ImmGetContext(window_handle);
	if (himc == NULL)
		return;

	constexpr LONG BottomMargin = 2;

	// Adjust the position of the input method editor (IME) to the caret.
	const LONG x = static_cast<LONG>(caret_position.x);
	const LONG y = static_cast<LONG>(caret_position.y);
	const LONG w = 1;
	const LONG h = static_cast<LONG>(line_height) + BottomMargin;

	COMPOSITIONFORM comp = {};
	comp.dwStyle = CFS_FORCE_POSITION;
	comp.ptCurrentPos = {x, y};
	ImmSetCompositionWindow(himc, &comp);

	CANDIDATEFORM cand = {};
	cand.dwStyle = CFS_EXCLUDE;
	cand.ptCurrentPos = {x, y};
	cand.rcArea = {x, y, x + w, y + h};
	ImmSetCandidateWindow(himc, &cand);

	ImmReleaseContext(window_handle, himc);
}

Rml::String RmlWin32::ConvertToUTF8(const std::wstring& wstr)
{
	const int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
	Rml::String str(count, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
	return str;
}

std::wstring RmlWin32::ConvertToUTF16(const Rml::String& str)
{
	const int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
	std::wstring wstr(count, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], count);
	return wstr;
}

static int IMEGetCursorPosition(HIMC context)
{
	return ImmGetCompositionString(context, GCS_CURSORPOS, nullptr, 0);
}

static std::wstring IMEGetCompositionString(HIMC context, bool finalize)
{
	DWORD type = finalize ? GCS_RESULTSTR : GCS_COMPSTR;
	int len_bytes = ImmGetCompositionString(context, type, nullptr, 0);

	if (len_bytes <= 0)
		return {};

	int len_chars = len_bytes / sizeof(TCHAR);
	Rml::UniquePtr<TCHAR[]> buffer(new TCHAR[len_chars + 1]);
	ImmGetCompositionString(context, type, buffer.get(), len_bytes);

#ifdef UNICODE
	return std::wstring(buffer.get(), len_chars);
#else
	return RmlWin32::ConvertToUTF16(Rml::String(buffer.get(), len_chars));
#endif
}

static void IMECompleteComposition(HWND window_handle)
{
	if (HIMC context = ImmGetContext(window_handle))
	{
		ImmNotifyIME(context, NI_COMPOSITIONSTR, CPS_COMPLETE, NULL);
		ImmReleaseContext(window_handle, context);
	}
}

bool RmlWin32::WindowProcedure(Rml::Context* context, TextInputMethodEditor_Win32& text_input_method_editor, HWND window_handle, UINT message,
	WPARAM w_param, LPARAM l_param)
{
	if (!context)
		return true;

	static bool tracking_mouse_leave = false;

	// If the user tries to interact with the window by using the mouse in any way, end the
	// composition by committing the current string. This behavior is identical to other
	// browsers and is expected, yet, Windows does not send any IME messages in such a case.
	if (text_input_method_editor.IsComposing() && message >= WM_LBUTTONDOWN && message <= WM_MBUTTONDBLCLK)
		IMECompleteComposition(window_handle);

	bool result = true;

	switch (message)
	{
	case WM_LBUTTONDOWN:
		result = context->ProcessMouseButtonDown(0, RmlWin32::GetKeyModifierState());
		SetCapture(window_handle);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		result = context->ProcessMouseButtonUp(0, RmlWin32::GetKeyModifierState());
		break;
	case WM_RBUTTONDOWN: result = context->ProcessMouseButtonDown(1, RmlWin32::GetKeyModifierState()); break;
	case WM_RBUTTONUP: result = context->ProcessMouseButtonUp(1, RmlWin32::GetKeyModifierState()); break;
	case WM_MBUTTONDOWN: result = context->ProcessMouseButtonDown(2, RmlWin32::GetKeyModifierState()); break;
	case WM_MBUTTONUP: result = context->ProcessMouseButtonUp(2, RmlWin32::GetKeyModifierState()); break;
	case WM_MOUSEMOVE:
		result = context->ProcessMouseMove(static_cast<int>((short)LOWORD(l_param)), static_cast<int>((short)HIWORD(l_param)),
			RmlWin32::GetKeyModifierState());

		if (!tracking_mouse_leave)
		{
			TRACKMOUSEEVENT tme = {};
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = window_handle;
			tracking_mouse_leave = TrackMouseEvent(&tme);
		}
		break;
	case WM_MOUSEWHEEL:
		result = context->ProcessMouseWheel(static_cast<float>((short)HIWORD(w_param)) / static_cast<float>(-WHEEL_DELTA),
			RmlWin32::GetKeyModifierState());
		break;
	case WM_MOUSELEAVE:
		result = context->ProcessMouseLeave();
		tracking_mouse_leave = false;
		break;
	case WM_KEYDOWN: result = context->ProcessKeyDown(RmlWin32::ConvertKey((int)w_param), RmlWin32::GetKeyModifierState()); break;
	case WM_KEYUP: result = context->ProcessKeyUp(RmlWin32::ConvertKey((int)w_param), RmlWin32::GetKeyModifierState()); break;
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
				Rml::String utf8 = ConvertToUTF8(std::wstring{first_u16_code_unit, c});
				character = Rml::StringUtilities::ToCharacter(utf8.data(), utf8.data() + utf8.size());
			}
			else if (c == '\r')
			{
				// Windows sends new-lines as carriage returns, convert to endlines.
				character = (Rml::Character)'\n';
			}

			first_u16_code_unit = 0;

			// Only send through printable characters.
			if (((char32_t)character >= 32 || character == (Rml::Character)'\n') && character != (Rml::Character)127)
				result = context->ProcessTextInput(character);
		}
	}
	break;
	case WM_IME_STARTCOMPOSITION:
		text_input_method_editor.StartComposition();
		// Prevent the native composition window from appearing by capturing the message.
		result = false;
		break;
	case WM_IME_ENDCOMPOSITION:
		if (text_input_method_editor.IsComposing())
			text_input_method_editor.ConfirmComposition(Rml::StringView());
		break;
	case WM_IME_COMPOSITION:
	{
		HIMC imm_context = ImmGetContext(window_handle);

		// Not every IME starts a composition.
		if (!text_input_method_editor.IsComposing())
			text_input_method_editor.StartComposition();

		if (!!(l_param & GCS_CURSORPOS))
		{
			// The cursor position is the wchar_t offset in the composition string. Because we
			// work with UTF-8 and not UTF-16, we will have to convert the character offset.
			int cursor_pos = IMEGetCursorPosition(imm_context);

			std::wstring composition = IMEGetCompositionString(imm_context, false);
			Rml::String converted = RmlWin32::ConvertToUTF8(composition.substr(0, cursor_pos));
			cursor_pos = (int)Rml::StringUtilities::LengthUTF8(converted);

			text_input_method_editor.SetCursorPosition(cursor_pos, true);
		}

		if (!!(l_param & CS_NOMOVECARET))
		{
			// Suppress the cursor position update. CS_NOMOVECARET is always a part of a more
			// complex message which means that the cursor is updated from a different event.
			text_input_method_editor.SetCursorPosition(-1, false);
		}

		if (!!(l_param & GCS_RESULTSTR))
		{
			std::wstring composition = IMEGetCompositionString(imm_context, true);
			text_input_method_editor.ConfirmComposition(RmlWin32::ConvertToUTF8(composition));
		}

		if (!!(l_param & GCS_COMPSTR))
		{
			std::wstring composition = IMEGetCompositionString(imm_context, false);
			text_input_method_editor.SetComposition(RmlWin32::ConvertToUTF8(composition));
		}

		// The composition has been canceled.
		if (!l_param)
			text_input_method_editor.CancelComposition();

		ImmReleaseContext(window_handle, imm_context);
	}
	break;
	case WM_IME_CHAR:
	case WM_IME_REQUEST:
		// Ignore WM_IME_CHAR and WM_IME_REQUEST to block the system from appending the composition string.
		result = false;
		break;
	default: break;
	}

	return result;
}

int RmlWin32::GetKeyModifierState()
{
	int key_modifier_state = 0;

	if (GetKeyState(VK_CAPITAL) & 1)
		key_modifier_state |= Rml::Input::KM_CAPSLOCK;

	if (GetKeyState(VK_NUMLOCK) & 1)
		key_modifier_state |= Rml::Input::KM_NUMLOCK;

	if (HIWORD(GetKeyState(VK_SHIFT)) & 1)
		key_modifier_state |= Rml::Input::KM_SHIFT;

	if (HIWORD(GetKeyState(VK_CONTROL)) & 1)
		key_modifier_state |= Rml::Input::KM_CTRL;

	if (HIWORD(GetKeyState(VK_MENU)) & 1)
		key_modifier_state |= Rml::Input::KM_ALT;

	return key_modifier_state;
}

// These are defined in winuser.h of MinGW 64 but are missing from MinGW 32
// Visual Studio has them by default
#if defined(__MINGW32__) && !defined(__MINGW64__)
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

Rml::Input::KeyIdentifier RmlWin32::ConvertKey(int win32_key_code)
{
	// clang-format off
	switch (win32_key_code)
	{
		case 'A':                    return Rml::Input::KI_A;
		case 'B':                    return Rml::Input::KI_B;
		case 'C':                    return Rml::Input::KI_C;
		case 'D':                    return Rml::Input::KI_D;
		case 'E':                    return Rml::Input::KI_E;
		case 'F':                    return Rml::Input::KI_F;
		case 'G':                    return Rml::Input::KI_G;
		case 'H':                    return Rml::Input::KI_H;
		case 'I':                    return Rml::Input::KI_I;
		case 'J':                    return Rml::Input::KI_J;
		case 'K':                    return Rml::Input::KI_K;
		case 'L':                    return Rml::Input::KI_L;
		case 'M':                    return Rml::Input::KI_M;
		case 'N':                    return Rml::Input::KI_N;
		case 'O':                    return Rml::Input::KI_O;
		case 'P':                    return Rml::Input::KI_P;
		case 'Q':                    return Rml::Input::KI_Q;
		case 'R':                    return Rml::Input::KI_R;
		case 'S':                    return Rml::Input::KI_S;
		case 'T':                    return Rml::Input::KI_T;
		case 'U':                    return Rml::Input::KI_U;
		case 'V':                    return Rml::Input::KI_V;
		case 'W':                    return Rml::Input::KI_W;
		case 'X':                    return Rml::Input::KI_X;
		case 'Y':                    return Rml::Input::KI_Y;
		case 'Z':                    return Rml::Input::KI_Z;

		case '0':                    return Rml::Input::KI_0;
		case '1':                    return Rml::Input::KI_1;
		case '2':                    return Rml::Input::KI_2;
		case '3':                    return Rml::Input::KI_3;
		case '4':                    return Rml::Input::KI_4;
		case '5':                    return Rml::Input::KI_5;
		case '6':                    return Rml::Input::KI_6;
		case '7':                    return Rml::Input::KI_7;
		case '8':                    return Rml::Input::KI_8;
		case '9':                    return Rml::Input::KI_9;

		case VK_BACK:                return Rml::Input::KI_BACK;
		case VK_TAB:                 return Rml::Input::KI_TAB;

		case VK_CLEAR:               return Rml::Input::KI_CLEAR;
		case VK_RETURN:              return Rml::Input::KI_RETURN;

		case VK_PAUSE:               return Rml::Input::KI_PAUSE;
		case VK_CAPITAL:             return Rml::Input::KI_CAPITAL;

		case VK_KANA:                return Rml::Input::KI_KANA;
		//case VK_HANGUL:              return Rml::Input::KI_HANGUL; /* overlaps with VK_KANA */
		case VK_JUNJA:               return Rml::Input::KI_JUNJA;
		case VK_FINAL:               return Rml::Input::KI_FINAL;
		case VK_HANJA:               return Rml::Input::KI_HANJA;
		//case VK_KANJI:               return Rml::Input::KI_KANJI; /* overlaps with VK_HANJA */

		case VK_ESCAPE:              return Rml::Input::KI_ESCAPE;

		case VK_CONVERT:             return Rml::Input::KI_CONVERT;
		case VK_NONCONVERT:          return Rml::Input::KI_NONCONVERT;
		case VK_ACCEPT:              return Rml::Input::KI_ACCEPT;
		case VK_MODECHANGE:          return Rml::Input::KI_MODECHANGE;

		case VK_SPACE:               return Rml::Input::KI_SPACE;
		case VK_PRIOR:               return Rml::Input::KI_PRIOR;
		case VK_NEXT:                return Rml::Input::KI_NEXT;
		case VK_END:                 return Rml::Input::KI_END;
		case VK_HOME:                return Rml::Input::KI_HOME;
		case VK_LEFT:                return Rml::Input::KI_LEFT;
		case VK_UP:                  return Rml::Input::KI_UP;
		case VK_RIGHT:               return Rml::Input::KI_RIGHT;
		case VK_DOWN:                return Rml::Input::KI_DOWN;
		case VK_SELECT:              return Rml::Input::KI_SELECT;
		case VK_PRINT:               return Rml::Input::KI_PRINT;
		case VK_EXECUTE:             return Rml::Input::KI_EXECUTE;
		case VK_SNAPSHOT:            return Rml::Input::KI_SNAPSHOT;
		case VK_INSERT:              return Rml::Input::KI_INSERT;
		case VK_DELETE:              return Rml::Input::KI_DELETE;
		case VK_HELP:                return Rml::Input::KI_HELP;

		case VK_LWIN:                return Rml::Input::KI_LWIN;
		case VK_RWIN:                return Rml::Input::KI_RWIN;
		case VK_APPS:                return Rml::Input::KI_APPS;

		case VK_SLEEP:               return Rml::Input::KI_SLEEP;

		case VK_NUMPAD0:             return Rml::Input::KI_NUMPAD0;
		case VK_NUMPAD1:             return Rml::Input::KI_NUMPAD1;
		case VK_NUMPAD2:             return Rml::Input::KI_NUMPAD2;
		case VK_NUMPAD3:             return Rml::Input::KI_NUMPAD3;
		case VK_NUMPAD4:             return Rml::Input::KI_NUMPAD4;
		case VK_NUMPAD5:             return Rml::Input::KI_NUMPAD5;
		case VK_NUMPAD6:             return Rml::Input::KI_NUMPAD6;
		case VK_NUMPAD7:             return Rml::Input::KI_NUMPAD7;
		case VK_NUMPAD8:             return Rml::Input::KI_NUMPAD8;
		case VK_NUMPAD9:             return Rml::Input::KI_NUMPAD9;
		case VK_MULTIPLY:            return Rml::Input::KI_MULTIPLY;
		case VK_ADD:                 return Rml::Input::KI_ADD;
		case VK_SEPARATOR:           return Rml::Input::KI_SEPARATOR;
		case VK_SUBTRACT:            return Rml::Input::KI_SUBTRACT;
		case VK_DECIMAL:             return Rml::Input::KI_DECIMAL;
		case VK_DIVIDE:              return Rml::Input::KI_DIVIDE;
		case VK_F1:                  return Rml::Input::KI_F1;
		case VK_F2:                  return Rml::Input::KI_F2;
		case VK_F3:                  return Rml::Input::KI_F3;
		case VK_F4:                  return Rml::Input::KI_F4;
		case VK_F5:                  return Rml::Input::KI_F5;
		case VK_F6:                  return Rml::Input::KI_F6;
		case VK_F7:                  return Rml::Input::KI_F7;
		case VK_F8:                  return Rml::Input::KI_F8;
		case VK_F9:                  return Rml::Input::KI_F9;
		case VK_F10:                 return Rml::Input::KI_F10;
		case VK_F11:                 return Rml::Input::KI_F11;
		case VK_F12:                 return Rml::Input::KI_F12;
		case VK_F13:                 return Rml::Input::KI_F13;
		case VK_F14:                 return Rml::Input::KI_F14;
		case VK_F15:                 return Rml::Input::KI_F15;
		case VK_F16:                 return Rml::Input::KI_F16;
		case VK_F17:                 return Rml::Input::KI_F17;
		case VK_F18:                 return Rml::Input::KI_F18;
		case VK_F19:                 return Rml::Input::KI_F19;
		case VK_F20:                 return Rml::Input::KI_F20;
		case VK_F21:                 return Rml::Input::KI_F21;
		case VK_F22:                 return Rml::Input::KI_F22;
		case VK_F23:                 return Rml::Input::KI_F23;
		case VK_F24:                 return Rml::Input::KI_F24;

		case VK_NUMLOCK:             return Rml::Input::KI_NUMLOCK;
		case VK_SCROLL:              return Rml::Input::KI_SCROLL;

		case VK_OEM_NEC_EQUAL:       return Rml::Input::KI_OEM_NEC_EQUAL;

		//case VK_OEM_FJ_JISHO:        return Rml::Input::KI_OEM_FJ_JISHO; /* overlaps with VK_OEM_NEC_EQUAL */
		case VK_OEM_FJ_MASSHOU:      return Rml::Input::KI_OEM_FJ_MASSHOU;
		case VK_OEM_FJ_TOUROKU:      return Rml::Input::KI_OEM_FJ_TOUROKU;
		case VK_OEM_FJ_LOYA:         return Rml::Input::KI_OEM_FJ_LOYA;
		case VK_OEM_FJ_ROYA:         return Rml::Input::KI_OEM_FJ_ROYA;

		case VK_SHIFT:               return Rml::Input::KI_LSHIFT;
		case VK_CONTROL:             return Rml::Input::KI_LCONTROL;
		case VK_MENU:                return Rml::Input::KI_LMENU;

		case VK_BROWSER_BACK:        return Rml::Input::KI_BROWSER_BACK;
		case VK_BROWSER_FORWARD:     return Rml::Input::KI_BROWSER_FORWARD;
		case VK_BROWSER_REFRESH:     return Rml::Input::KI_BROWSER_REFRESH;
		case VK_BROWSER_STOP:        return Rml::Input::KI_BROWSER_STOP;
		case VK_BROWSER_SEARCH:      return Rml::Input::KI_BROWSER_SEARCH;
		case VK_BROWSER_FAVORITES:   return Rml::Input::KI_BROWSER_FAVORITES;
		case VK_BROWSER_HOME:        return Rml::Input::KI_BROWSER_HOME;

		case VK_VOLUME_MUTE:         return Rml::Input::KI_VOLUME_MUTE;
		case VK_VOLUME_DOWN:         return Rml::Input::KI_VOLUME_DOWN;
		case VK_VOLUME_UP:           return Rml::Input::KI_VOLUME_UP;
		case VK_MEDIA_NEXT_TRACK:    return Rml::Input::KI_MEDIA_NEXT_TRACK;
		case VK_MEDIA_PREV_TRACK:    return Rml::Input::KI_MEDIA_PREV_TRACK;
		case VK_MEDIA_STOP:          return Rml::Input::KI_MEDIA_STOP;
		case VK_MEDIA_PLAY_PAUSE:    return Rml::Input::KI_MEDIA_PLAY_PAUSE;
		case VK_LAUNCH_MAIL:         return Rml::Input::KI_LAUNCH_MAIL;
		case VK_LAUNCH_MEDIA_SELECT: return Rml::Input::KI_LAUNCH_MEDIA_SELECT;
		case VK_LAUNCH_APP1:         return Rml::Input::KI_LAUNCH_APP1;
		case VK_LAUNCH_APP2:         return Rml::Input::KI_LAUNCH_APP2;

		case VK_OEM_1:               return Rml::Input::KI_OEM_1;
		case VK_OEM_PLUS:            return Rml::Input::KI_OEM_PLUS;
		case VK_OEM_COMMA:           return Rml::Input::KI_OEM_COMMA;
		case VK_OEM_MINUS:           return Rml::Input::KI_OEM_MINUS;
		case VK_OEM_PERIOD:          return Rml::Input::KI_OEM_PERIOD;
		case VK_OEM_2:               return Rml::Input::KI_OEM_2;
		case VK_OEM_3:               return Rml::Input::KI_OEM_3;

		case VK_OEM_4:               return Rml::Input::KI_OEM_4;
		case VK_OEM_5:               return Rml::Input::KI_OEM_5;
		case VK_OEM_6:               return Rml::Input::KI_OEM_6;
		case VK_OEM_7:               return Rml::Input::KI_OEM_7;
		case VK_OEM_8:               return Rml::Input::KI_OEM_8;

		case VK_OEM_AX:              return Rml::Input::KI_OEM_AX;
		case VK_OEM_102:             return Rml::Input::KI_OEM_102;
		case VK_ICO_HELP:            return Rml::Input::KI_ICO_HELP;
		case VK_ICO_00:              return Rml::Input::KI_ICO_00;

		case VK_PROCESSKEY:          return Rml::Input::KI_PROCESSKEY;

		case VK_ICO_CLEAR:           return Rml::Input::KI_ICO_CLEAR;

		case VK_ATTN:                return Rml::Input::KI_ATTN;
		case VK_CRSEL:               return Rml::Input::KI_CRSEL;
		case VK_EXSEL:               return Rml::Input::KI_EXSEL;
		case VK_EREOF:               return Rml::Input::KI_EREOF;
		case VK_PLAY:                return Rml::Input::KI_PLAY;
		case VK_ZOOM:                return Rml::Input::KI_ZOOM;
		case VK_PA1:                 return Rml::Input::KI_PA1;
		case VK_OEM_CLEAR:           return Rml::Input::KI_OEM_CLEAR;
	}
	// clang-format on

	return Rml::Input::KI_UNKNOWN;
}

TextInputMethodEditor_Win32::TextInputMethodEditor_Win32() :
	input_context(nullptr), composing(false), cursor_pos(-1), composition_range_start(0), composition_range_end(0)
{}

void TextInputMethodEditor_Win32::OnActivate(Rml::TextInputContext* _input_context)
{
	input_context = _input_context;
}

void TextInputMethodEditor_Win32::OnDeactivate(Rml::TextInputContext* _input_context)
{
	if (input_context == _input_context)
		input_context = nullptr;
}

void TextInputMethodEditor_Win32::OnDestroy(Rml::TextInputContext* _input_context)
{
	if (input_context == _input_context)
		input_context = nullptr;
}

bool TextInputMethodEditor_Win32::IsComposing() const
{
	return composing;
}

void TextInputMethodEditor_Win32::StartComposition()
{
	RMLUI_ASSERT(!composing);
	composing = true;
}

void TextInputMethodEditor_Win32::EndComposition()
{
	if (input_context != nullptr)
		input_context->SetCompositionRange(0, 0);

	RMLUI_ASSERT(composing);
	composing = false;

	composition_range_start = 0;
	composition_range_end = 0;
}

void TextInputMethodEditor_Win32::CancelComposition()
{
	RMLUI_ASSERT(IsComposing());

	if (input_context != nullptr)
	{
		// Purge the current composition string.
		input_context->SetText(Rml::StringView(), composition_range_start, composition_range_end);
		// Move the cursor back to where the composition began.
		input_context->SetCursorPosition(composition_range_start);
	}

	EndComposition();
}

void TextInputMethodEditor_Win32::SetComposition(Rml::StringView composition)
{
	RMLUI_ASSERT(IsComposing());

	SetCompositionString(composition);
	UpdateCursorPosition();

	// Update the composition range only if the cursor can be moved around. Editors working with a single
	// character (e.g., Hangul IME) should have no visual feedback; they use a selection range instead.
	if (cursor_pos != -1 && input_context != nullptr)
		input_context->SetCompositionRange(composition_range_start, composition_range_end);
}

void TextInputMethodEditor_Win32::ConfirmComposition(Rml::StringView composition)
{
	RMLUI_ASSERT(IsComposing());

	SetCompositionString(composition);

	if (input_context != nullptr)
	{
		input_context->SetCompositionRange(composition_range_start, composition_range_end);
		input_context->CommitComposition(composition);
	}

	// Move the cursor to the end of the string.
	SetCursorPosition(composition_range_end - composition_range_start, true);

	EndComposition();
}

void TextInputMethodEditor_Win32::SetCursorPosition(int _cursor_pos, bool update)
{
	RMLUI_ASSERT(IsComposing());

	cursor_pos = _cursor_pos;

	if (update)
		UpdateCursorPosition();
}

void TextInputMethodEditor_Win32::SetCompositionString(Rml::StringView composition)
{
	if (input_context == nullptr)
		return;

	// Retrieve the composition range if it is missing.
	if (composition_range_start == 0 && composition_range_end == 0)
		input_context->GetSelectionRange(composition_range_start, composition_range_end);

	input_context->SetText(composition, composition_range_start, composition_range_end);

	size_t length = Rml::StringUtilities::LengthUTF8(composition);
	composition_range_end = composition_range_start + (int)length;
}

void TextInputMethodEditor_Win32::UpdateCursorPosition()
{
	// Cursor position update happens before a composition is set; ignore this event.
	if (input_context == nullptr || (composition_range_start == 0 && composition_range_end == 0))
		return;

	if (cursor_pos != -1)
	{
		int position = composition_range_start + cursor_pos;
		input_context->SetCursorPosition(position);
	}
	else
	{
		// If the API reports no cursor position, select the entire composition string for a better UX.
		input_context->SetSelectionRange(composition_range_start, composition_range_end);
	}
}
