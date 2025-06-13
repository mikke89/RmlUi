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

#include "LogDefault.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"

#ifdef RMLUI_PLATFORM_WIN32_NATIVE
	#include <windows.h>
#else
	#include <stdio.h>
#endif

namespace Rml {

#if defined RMLUI_PLATFORM_WIN32_NATIVE
bool LogDefault::LogMessage(Log::Type type, const String& message)
{
	#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
	if (type == Log::LT_ASSERT)
	{
		String message_user = CreateString("%s\nWould you like to interrupt execution?", message.c_str());

		// Return TRUE if the user presses NO (continue execution)
		return (IDNO == MessageBoxA(nullptr, message_user.c_str(), "Assertion Failure", MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2 | MB_TASKMODAL));
	}
	else
	#endif
	{
		OutputDebugStringA(message.c_str());
		OutputDebugStringA("\r\n");
	}
	return true;
}
#else
bool LogDefault::LogMessage(Log::Type /*type*/, const String& message)
{
	#ifdef RMLUI_PLATFORM_EMSCRIPTEN
	puts(message.c_str());
	#else
	fprintf(stderr, "%s\n", message.c_str());
	#endif
	return true;
}
#endif

} // namespace Rml
