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
