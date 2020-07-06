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

#include "SystemInterface.h"
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/StringUtilities.h>
#include <Shell.h>
#include <stdio.h>
#ifdef RMLUI_PLATFORM_WIN32
#include <windows.h>
#endif

SystemInterface::SystemInterface()
{
	fp = fopen("log.txt", "wt");
}

SystemInterface::~SystemInterface()
{
	if (fp != nullptr)
		fclose(fp);
}

// Get the number of seconds elapsed since the start of the application.
double SystemInterface::GetElapsedTime()
{
	return Shell::GetElapsedTime();
}

bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
	if (fp != nullptr)
	{
		// Select a prefix appropriate for the severity of the message.
		const char* prefix;
		switch (type)
		{
			case Rml::Log::LT_ERROR:
			case Rml::Log::LT_ASSERT:
				prefix = "-!-";
				break;

			case Rml::Log::LT_WARNING:
				prefix = "-*-";
				break;

			default:
				prefix = "---";
				break;
		}

		// Print the message and timestamp to file, and force a write in case of a crash.
		fprintf(fp, "%s (%.2f): %s", prefix, GetElapsedTime(), message.c_str());
		fflush(fp);

#ifdef RMLUI_PLATFORM_WIN32
		if (type == Rml::Log::LT_ASSERT)
		{
			Rml::String assert_message = Rml::CreateString(1024, "%s\nWould you like to interrupt execution?", message.c_str());

			// Return TRUE if the user presses NO (continue execution)
			return MessageBox(nullptr, assert_message.c_str(), "Assertion Failure", MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2 | MB_SYSTEMMODAL) == IDNO;
		}
#endif
	}

	return true;
}
