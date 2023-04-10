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

#ifndef RMLUI_BACKENDS_PLATFORM_X11_H
#define RMLUI_BACKENDS_PLATFORM_X11_H

#include "RmlUi_Include_Xlib.h"
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <X11/Xutil.h>

class SystemInterface_X11 : public Rml::SystemInterface {
public:
	SystemInterface_X11(Display* display);

	// Optionally, provide or change the window to be used for setting the mouse cursors and clipboard text.
	void SetWindow(Window window);

	// Handle selection request events.
	// @return True if the event is still propagating, false if it was handled here.
	bool HandleSelectionRequest(const XEvent& ev);

	// -- Inherited from Rml::SystemInterface  --

	double GetElapsedTime() override;

	void SetMouseCursor(const Rml::String& cursor_name) override;

	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;

private:
	void XCopy(const Rml::String& clipboard_data, const XEvent& event);
	bool XPaste(Atom target_atom, Rml::String& clipboard_data);

	Display* display = nullptr;
	Window window = 0;
	timeval start_time;

	Cursor cursor_default = 0;
	Cursor cursor_move = 0;
	Cursor cursor_pointer = 0;
	Cursor cursor_resize = 0;
	Cursor cursor_cross = 0;
	Cursor cursor_text = 0;
	Cursor cursor_unavailable = 0;

	// -- Clipboard
	Rml::String clipboard_text;
	Atom XA_atom = 4;
	Atom XA_STRING_atom = 31;
	Atom UTF8_atom;
	Atom CLIPBOARD_atom;
	Atom XSEL_DATA_atom;
	Atom TARGETS_atom;
	Atom TEXT_atom;
};

/**
    Optional helper functions for the X11 plaform.
 */
namespace RmlX11 {

// Applies input on the context based on the given SFML event.
// @return True if the event is still propagating, false if it was handled by the context.
bool HandleInputEvent(Rml::Context* context, Display* display, const XEvent& ev);

// Converts the X11 key code to RmlUi key.
// @note The display must be passed here as it needs to query the connected X server display for information about its install keymap abilities.
Rml::Input::KeyIdentifier ConvertKey(Display* display, unsigned int x11_key_code);

// Converts the X11 mouse button to RmlUi mouse button.
int ConvertMouseButton(unsigned int x11_mouse_button);

// Converts the X11 key modifier to RmlUi key modifier.
int GetKeyModifierState(int x11_state);

} // namespace RmlX11

#endif
