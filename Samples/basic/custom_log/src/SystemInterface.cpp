#include "SystemInterface.h"
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/StringUtilities.h>
#include <Shell.h>
#include <stdio.h>
#ifdef RMLUI_PLATFORM_WIN32
	#include <RmlUi_Include_Windows.h>
#endif

SystemInterface::SystemInterface()
{
	fp = fopen("log.txt", "wt");
}

SystemInterface::~SystemInterface()
{
	if (fp)
		fclose(fp);
}

bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
	if (fp)
	{
		// Select a prefix appropriate for the severity of the message.
		const char* prefix;
		switch (type)
		{
		case Rml::Log::LT_ERROR:
		case Rml::Log::LT_ASSERT: prefix = "-!-"; break;

		case Rml::Log::LT_WARNING: prefix = "-*-"; break;

		default: prefix = "---"; break;
		}

		// Print the message and timestamp to file, and force a write in case of a crash.
		fprintf(fp, "%s (%.2f): %s\n", prefix, GetElapsedTime(), message.c_str());
		fflush(fp);

#ifdef RMLUI_PLATFORM_WIN32
		if (type == Rml::Log::LT_ASSERT)
		{
			Rml::String assert_message = Rml::CreateString("%s\nWould you like to interrupt execution?", message.c_str());

			// Return TRUE if the user presses NO (continue execution)
			return MessageBoxA(nullptr, assert_message.c_str(), "Assertion Failure", MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2 | MB_SYSTEMMODAL) == IDNO;
		}
#endif
	}

	return true;
}
