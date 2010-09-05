/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

#include "SystemInterface.h"
#include <Rocket/Core/Platform.h>
#include <Shell.h>
#include <stdio.h>
#ifdef EMP_PLATFORM_WIN32
#include <windows.h>
#endif

SystemInterface::SystemInterface()
{
	fp = fopen("log.txt", "wt");
}

SystemInterface::~SystemInterface()
{
	if (fp != NULL)
		fclose(fp);
}

// Get the number of seconds elapsed since the start of the application.
float SystemInterface::GetElapsedTime()
{
	return Shell::GetElapsedTime();
}

bool SystemInterface::LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message)
{
	if (fp != NULL)
	{
		// Select a prefix appropriate for the severity of the message.
		const char* prefix;
		switch (type)
		{
			case Rocket::Core::Log::LT_ERROR:
			case Rocket::Core::Log::LT_ASSERT:
				prefix = "-!-";
				break;

			case Rocket::Core::Log::LT_WARNING:
				prefix = "-*-";
				break;

			default:
				prefix = "---";
				break;
		}

		// Print the message and timestamp to file, and force a write in case of a crash.
		fprintf(fp, "%s (%.2f): %s", prefix, GetElapsedTime(), message.CString());
		fflush(fp);

#ifdef EMP_PLATFORM_WIN32
		if (type == Rocket::Core::Log::LT_ASSERT)
		{
			Rocket::Core::String assert_message(1024, "%s\nWould you like to interrupt execution?", message.CString());

			// Return TRUE if the user presses NO (continue execution)
			return MessageBox(NULL, assert_message.CString(), "Assertion Failure", MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2 | MB_SYSTEMMODAL) == IDNO;
		}
#endif
	}

	return true;
}
