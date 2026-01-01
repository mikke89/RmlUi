#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "LogDefault.h"
#include <stdarg.h>
#include <stdio.h>

namespace Rml {

void Log::Message(Log::Type type, const char* fmt, ...)
{
	const int buffer_size = 1024;
	char buffer[buffer_size];
	va_list argument_list;

	// Print the message to the buffer.
	va_start(argument_list, fmt);
	int len = vsnprintf(buffer, buffer_size - 2, fmt, argument_list);
	if (len < 0 || len > buffer_size - 2)
	{
		len = buffer_size - 2;
	}
	buffer[len] = '\0';
	va_end(argument_list);

	if (SystemInterface* system_interface = GetSystemInterface())
		system_interface->LogMessage(type, buffer);
	else
		LogDefault::LogMessage(type, buffer);
}

void Log::ParseError(const String& filename, int line_number, const char* fmt, ...)
{
	const int buffer_size = 1024;
	char buffer[buffer_size];
	va_list argument_list;

	// Print the message to the buffer.
	va_start(argument_list, fmt);
	int len = vsnprintf(buffer, buffer_size - 2, fmt, argument_list);
	if (len < 0 || len > buffer_size - 2)
	{
		len = buffer_size - 2;
	}
	buffer[len] = '\0';
	va_end(argument_list);

	if (line_number >= 0)
		Message(Log::LT_ERROR, "%s:%d: %s", filename.c_str(), line_number, buffer);
	else
		Message(Log::LT_ERROR, "%s: %s", filename.c_str(), buffer);
}

bool Assert(const char* msg, const char* file, int line)
{
	String message = CreateString("%s\n%s:%d", msg, file, line);

	bool result = true;
	if (SystemInterface* system_interface = GetSystemInterface())
		result = system_interface->LogMessage(Log::LT_ASSERT, message);
	else
		result = LogDefault::LogMessage(Log::LT_ASSERT, message);

	return result;
}

} // namespace Rml
