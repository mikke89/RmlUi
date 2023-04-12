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

#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/URL.h"

#ifdef RMLUI_PLATFORM_WIN32
	#include <windows.h>
#else
	#include <stdio.h>
#endif

namespace Rml {

static String clipboard_text;

SystemInterface::SystemInterface() {}

SystemInterface::~SystemInterface() {}

#ifdef RMLUI_PLATFORM_WIN32
bool SystemInterface::LogMessage(Log::Type logtype, const String& message)
{
	// By default we just send a platform message
	#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
	if (logtype == Log::LT_ASSERT)
	{
		String message_user = CreateString(1024, "%s\nWould you like to interrupt execution?", message.c_str());

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
bool SystemInterface::LogMessage(Log::Type /*logtype*/, const String& message)
{
	#ifdef RMLUI_PLATFORM_EMSCRIPTEN
	puts(message.c_str());
	#else
	fprintf(stderr, "%s\n", message.c_str());
	#endif
	return true;
}
#endif

void SystemInterface::SetMouseCursor(const String& /*cursor_name*/) {}

void SystemInterface::SetClipboardText(const String& text)
{
	// The default implementation will only copy and paste within the application
	clipboard_text = text;
}

void SystemInterface::GetClipboardText(String& text)
{
	text = clipboard_text;
}

int SystemInterface::TranslateString(String& translated, const String& input)
{
	translated = input;
	return 0;
}

void SystemInterface::JoinPath(String& translated_path, const String& document_path, const String& path)
{
	// If the path is absolute, strip the leading / and return it.
	if (path.size() > 0 && path[0] == '/')
	{
		translated_path = path.substr(1);
		return;
	}

	// If the path is a Windows-style absolute path, return it directly.
	size_t drive_pos = path.find(':');
	size_t slash_pos = Math::Min(path.find('/'), path.find('\\'));
	if (drive_pos != String::npos && drive_pos < slash_pos)
	{
		translated_path = path;
		return;
	}

	using StringUtilities::Replace;

	// Strip off the referencing document name.
	translated_path = document_path;
	translated_path = Replace(translated_path, '\\', '/');
	size_t file_start = translated_path.rfind('/');
	if (file_start != String::npos)
		translated_path.resize(file_start + 1);
	else
		translated_path.clear();

	// Append the paths and send through URL to removing any '..'.
	URL url(Replace(translated_path, ':', '|') + Replace(path, '\\', '/'));
	translated_path = Replace(url.GetPathedFileName(), '|', ':');
}

void SystemInterface::ActivateKeyboard(Rml::Vector2f /*caret_position*/, float /*line_height*/) {}

void SystemInterface::DeactivateKeyboard() {}

} // namespace Rml
