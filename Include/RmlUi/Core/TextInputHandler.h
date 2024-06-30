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

#ifndef RMLUI_CORE_TEXTINPUTHANDLER_H
#define RMLUI_CORE_TEXTINPUTHANDLER_H

namespace Rml {

class TextInputContext;

/**
    Handler of changes to text editable areas. Implement this interface to pick up these events, and pass
    the custom implementation to a context (via its constructor) or globally (via SetTextInputHandler).

    Be aware that backends might provide their custom handler to, for example, handle the IME.

    The lifetime of a text input context is ended with the call of OnDestroy().

    @see Rml::TextInputContext
    @see Rml::SetTextInputHandler()
 */
class RMLUICORE_API TextInputHandler : public NonCopyMoveable {
public:
	virtual ~TextInputHandler() {}

	/// Called when a text input area is activated (e.g., focused).
	/// @param[in] input_context The input context to be activated.
	virtual void OnActivate(TextInputContext* /*input_context*/) {}

	/// Called when a text input area is deactivated (e.g., by losing focus).
	/// @param[in] input_context The input context to be deactivated.
	virtual void OnDeactivate(TextInputContext* /*input_context*/) {}

	/// Invoked when the context of a text input area is destroyed (e.g., when the element is being removed).
	/// @param[in] input_context The input context to be destroyed.
	virtual void OnDestroy(TextInputContext* /*input_context*/) {}
};

} // namespace Rml

#endif
