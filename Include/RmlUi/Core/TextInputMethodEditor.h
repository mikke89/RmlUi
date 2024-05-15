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

#ifndef TEXTINPUTMETHODEDITOR_H
#define TEXTINPUTMETHODEDITOR_H

#include <RmlUi/Core/StringUtilities.h>

namespace Rml {

class TextInputMethodContext;

/**
    An interface for an IME (Input Method Editor) system.
 */

class RMLUICORE_API TextInputMethodEditor {
public:
	virtual ~TextInputMethodEditor() {}

	/// Return whether the system native composition should be blocked.
	/// @return True to block the system composition implementation, false otherwise.
	virtual bool IsNativeCompositionBlocked() const = 0;

	/// Activate the input context.
	/// @param[in] context Context to be activated.
	virtual void ActivateContext(SharedPtr<TextInputMethodContext> context) = 0;

	/// Deactivate the input context.
	/// @param[in] context Context to be deactivated.
	virtual void DeactivateContext(TextInputMethodContext* context) = 0;

	/// Check that a composition is currently active.
	/// @return True if we are composing, false otherwise.
	virtual bool IsComposing() const = 0;

	/// Start a composition (e.g., by displaying the composition window).
	virtual void StartComposition() = 0;

	/// End the current composition.
	virtual void EndComposition() = 0;

	/// Cancel the current composition and purge the string.
	virtual void CancelComposition() = 0;

	/// Set the composition string.
	/// @param[in] composition A string to be set.
	virtual void SetComposition(StringView composition) = 0;

	/// End the current composition by confirming the composition string.
	/// @param[in] composition A string to confirm.
	virtual void ConfirmComposition(StringView composition) = 0;

	/// Set the cursor position within the composition.
	/// @param[in] cursor_pos A character position of the cursor within the composition string.
	/// @param[in] update Update the cursor position within active input contexts.
	virtual void SetCursorPosition(int cursor_pos, bool update) = 0;
};

RMLUICORE_API UniquePtr<TextInputMethodEditor> CreateDefaultTextInputMethodEditor();

} // namespace Rml

#endif
