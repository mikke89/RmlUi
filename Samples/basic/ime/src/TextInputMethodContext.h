/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#ifndef TEXTINPUTMETHODCONTEXT_H
#define TEXTINPUTMETHODCONTEXT_H

#include <RmlUi/Core/StringUtilities.h>

/**
    Sample interface for a text input handler for IME.
 */

class TextInputMethodContext {
public:
	virtual ~TextInputMethodContext() = 0;

	/// Retrieves the selection range.
	/// @param[out] start The first character selected.
	/// @param[out] end The first character *after* the selection.
	virtual void GetSelectionRange(int& start, int& end) const = 0;

	/// Selects the text in the given character range.
	/// @param[in] start The first character to be selected.
	/// @param[in] end The first character *after* the selection.
	virtual void SetSelectionRange(int start, int end) = 0;

	/// Moves the cursor caret to after a specific character.
	/// @param[in] position The character after which the cursor should be moved.
	virtual void SetCursorPosition(int position) = 0;

	/// Replaces a text in the given character range.
	/// @param[in] text The string to replace the character range with.
	/// @param[in] start The first character to be replaced.
	/// @param[in] end The first character *after* the range.
	virtual void SetText(Rml::StringView text, int start, int end) = 0;

	/// Retrieves the screen-space bounds of the text area (in px).
	/// @param[out] position The screen-space position of the text area (in px).
	/// @param[out] size The screen-space size of the text area (in px).
	virtual void GetScreenBounds(Rml::Vector2f& position, Rml::Vector2f& size) const = 0;

	/// Updates the range of the text being composed (e.g., for visual feedback).
	/// @param[in] start The first character in the range.
	/// @param[in] end The first character *after* the range.
	virtual void SetCompositionRange(int start, int end) = 0;
};

namespace Rml {
class ElementFormControlInput;
class ElementFormControlTextArea;
} // namespace Rml

Rml::UniquePtr<TextInputMethodContext> CreateTextInputMethodContext(Rml::ElementFormControlInput* input);
Rml::UniquePtr<TextInputMethodContext> CreateTextInputMethodContext(Rml::ElementFormControlTextArea* text_area);

#endif
