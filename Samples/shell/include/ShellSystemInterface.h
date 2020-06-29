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

#ifndef RMLUI_SHELL_SHELLSYSTEMINTERFACE_H
#define RMLUI_SHELL_SHELLSYSTEMINTERFACE_H

#include <RmlUi/Core/SystemInterface.h>

/**
	A custom system interface for RmlUi. This provides timing information.
	@author Lloyd Weehuizen
 */

class ShellSystemInterface : public Rml::SystemInterface
{
public:
	/// Get the number of seconds elapsed since the start of the application
	/// @returns Seconds elapsed
	double GetElapsedTime() override;

	/// Set mouse cursor.
	/// @param[in] cursor_name Cursor name to activate.
	void SetMouseCursor(const Rml::String& cursor_name) override;

	/// Set clipboard text.
	/// @param[in] text Text to apply to clipboard.
	void SetClipboardText(const Rml::String& text) override;

	/// Get clipboard text.
	/// @param[out] text Retrieved text from clipboard.
	void GetClipboardText(Rml::String& text) override;
};

#endif
