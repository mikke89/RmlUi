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

#ifndef RMLUI_DEBUGGER_DEBUGGER_H
#define RMLUI_DEBUGGER_DEBUGGER_H

#include "Header.h"

namespace Rml {

class Context;

namespace Debugger {

	/// Initialises the debug plugin. The debugger will be loaded into the given context.
	/// @param[in] host_context RmlUi context to load the debugger into. The debugging tools will be displayed on this context. If this context is
	///     destroyed, the debugger will be released.
	/// @return True if the debugger was successfully initialised
	RMLUIDEBUGGER_API bool Initialise(Context* host_context);

	/// Shuts down the debugger.
	/// @note The debugger is automatically shutdown during the call to Rml::Shutdown(), calling this is only necessary to shutdown the debugger early
	///     or to re-initialize the debugger on another host context.
	RMLUIDEBUGGER_API void Shutdown();

	/// Sets the context to be debugged.
	/// @param[in] context The context to be debugged.
	/// @return True if the debugger is initialised and the context was switched, false otherwise.
	RMLUIDEBUGGER_API bool SetContext(Context* context);

	/// Sets the visibility of the debugger.
	/// @param[in] visibility True to show the debugger, false to hide it.
	RMLUIDEBUGGER_API void SetVisible(bool visibility);
	/// Returns the visibility of the debugger.
	/// @return True if the debugger is visible, false if not.
	RMLUIDEBUGGER_API bool IsVisible();

} // namespace Debugger
} // namespace Rml

#endif
