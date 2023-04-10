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

#ifndef RMLUI_BACKENDS_PLATFORM_WIN32_H
#define RMLUI_BACKENDS_PLATFORM_WIN32_H

#include "RmlUi_Include_Windows.h"
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <string>

class SystemInterface_Win32 : public Rml::SystemInterface {
public:
	SystemInterface_Win32();

	// Optionally, provide or change the window to be used for setting the mouse cursor, clipboard text and IME position.
	void SetWindow(HWND window_handle);

	// -- Inherited from Rml::SystemInterface  --

	double GetElapsedTime() override;

	void SetMouseCursor(const Rml::String& cursor_name) override;

	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

	void ActivateKeyboard(Rml::Vector2f caret_position, float line_height) override;

private:
	HWND window_handle = nullptr;

	double time_frequency = {};
	LARGE_INTEGER time_startup = {};

	HCURSOR cursor_default = nullptr;
	HCURSOR cursor_move = nullptr;
	HCURSOR cursor_pointer = nullptr;
	HCURSOR cursor_resize = nullptr;
	HCURSOR cursor_cross = nullptr;
	HCURSOR cursor_text = nullptr;
	HCURSOR cursor_unavailable = nullptr;
};

/**
    Optional helper functions for the Win32 plaform.
 */
namespace RmlWin32 {

// Convenience helpers for converting between RmlUi strings (UTF-8) and Windows strings (UTF-16).
Rml::String ConvertToUTF8(const std::wstring& wstr);
std::wstring ConvertToUTF16(const Rml::String& str);

// Window event handler to submit default input behavior to the context.
// @return True if the event is still propagating, false if it was handled by the context.
bool WindowProcedure(Rml::Context* context, HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);

// Converts the key from Win32 key code to RmlUi key.
Rml::Input::KeyIdentifier ConvertKey(int win32_key_code);

// Returns the active RmlUi key modifier state.
int GetKeyModifierState();

} // namespace RmlWin32

#endif
