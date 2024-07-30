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

#ifndef RMLUI_CORE_SYSTEMINTERFACE_H
#define RMLUI_CORE_SYSTEMINTERFACE_H

#include "Header.h"
#include "Log.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

/**
    RmlUi's system interface provides an interface for time, translation, logging, and other system utilities.

    The default logging implementation outputs to the Windows Debug Console on Windows, and Standard Error on other
    platforms.

    @author Lloyd Weehuizen
 */

class RMLUICORE_API SystemInterface : public NonCopyMoveable {
public:
	SystemInterface();
	virtual ~SystemInterface();

	/// Get the number of seconds elapsed since the start of the application.
	/// @return Elapsed time, in seconds.
	virtual double GetElapsedTime();

	/// Translate the input string into the translated string.
	/// @param[out] translated Translated string ready for display.
	/// @param[in] input String as received from XML.
	/// @return Number of translations that occured.
	virtual int TranslateString(String& translated, const String& input);

	/// Joins the path of an RML or RCSS file with the path of a resource specified within the file.
	/// @param[out] translated_path The joined path.
	/// @param[in] document_path The path of the source document (including the file name).
	/// @param[in] path The path of the resource specified in the document.
	virtual void JoinPath(String& translated_path, const String& document_path, const String& path);

	/// Log the specified message.
	/// @param[in] type Type of log message, ERROR, WARNING, etc.
	/// @param[in] message Message to log.
	/// @return True to continue execution, false to break into the debugger.
	virtual bool LogMessage(Log::Type type, const String& message);

	/// Set mouse cursor.
	/// @param[in] cursor_name Cursor name to activate.
	virtual void SetMouseCursor(const String& cursor_name);

	/// Set clipboard text.
	/// @param[in] text Text to apply to clipboard.
	virtual void SetClipboardText(const String& text);

	/// Get clipboard text.
	/// @param[out] text Retrieved text from clipboard.
	virtual void GetClipboardText(String& text);

	/// Activate keyboard (for touchscreen devices).
	/// @param[in] caret_position Position of the caret in absolute window coordinates.
	/// @param[in] line_height Height of the current line being edited.
	virtual void ActivateKeyboard(Rml::Vector2f caret_position, float line_height);

	/// Deactivate keyboard (for touchscreen devices).
	virtual void DeactivateKeyboard();
};

} // namespace Rml
#endif
