/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "precompiled.h"
#include "../../Include/Rocket/Core/String.h"
#include <stdarg.h>

namespace Rocket {
namespace Core {


int FormatString(String& string, size_t max_size, const char* format, va_list argument_list)
{
	const int INTERNAL_BUFFER_SIZE = 1024;
	static char buffer[INTERNAL_BUFFER_SIZE];
	char* buffer_ptr = buffer;

	if (max_size + 1 > INTERNAL_BUFFER_SIZE)
		buffer_ptr = new char[max_size + 1];

	int length = vsnprintf(buffer_ptr, max_size, format, argument_list);
	buffer_ptr[length >= 0 ? length : max_size] = '\0';
#ifdef ROCKET_DEBUG
	if (length == -1)
	{
		Log::Message(Log::LT_WARNING, "String::sprintf: String truncated to %d bytes when processing %s", max_size, format);
	}
#endif

	string = buffer_ptr;

	if (buffer_ptr != buffer)
		delete[] buffer_ptr;

	return length;
}

int FormatString(String& string, size_t max_size, const char* format, ...)
{
	va_list argument_list;
	va_start(argument_list, format);
	int result = FormatString(string, (int)max_size, format, argument_list);
	va_end(argument_list);
	return result;
}
String CreateString(size_t max_size, const char* format, ...)
{
	String result;
	result.reserve(max_size);
	va_list argument_list;
	va_start(argument_list, format);
	FormatString(result, max_size, format, argument_list);
	va_end(argument_list);
	return result;
}

String ToLower(const String& string) {
	std::string str_lower = string;
	std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::tolower);
	return str_lower;
}

WString ToWideString(const String& str)
{
	WString result;
	StringUtilities::UTF8toUCS2(str, result);
	return result;
}

String ToUTF8(const WString& wstr)
{
	std::string result;
	StringUtilities::UCS2toUTF8(wstr, result);
	return result;
}

String Replace(String subject, const String& search, const String& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != String::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

String Replace(String subject, char search, char replace)
{
	const size_t size = subject.size();
	for (size_t i = 0; i < size; i++)
	{
		if (subject[i] == search)
			subject[i] = replace;
	}
	return subject;
}


}
}
