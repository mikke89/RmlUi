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
