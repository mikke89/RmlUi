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
#include <Rocket/Core/String.h>
#include <Rocket/Core/StringBase.h>

namespace Rocket {
namespace Core {

int ROCKETCORE_API RocketStringFormatString(StringBase<char>& string, int max_size, const char* format, va_list argument_list)
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

template <>
StringBase<char>::StringBase(StringBase<char>::size_type max_size, const char* fmt, ...)
{
	string_id = 0;
	length = 0;
	value = StringStorage::empty_string;

	va_list argument_list;
	va_start(argument_list, fmt);

	RocketStringFormatString(*this, max_size, fmt, argument_list);

	va_end(argument_list);
}

template <>
int StringBase<char>::FormatString(StringBase<char>::size_type max_size, const char* fmt, ...)
{
	va_list argument_list;
	va_start(argument_list, fmt);

	int length = RocketStringFormatString(*this, max_size, fmt, argument_list);

	va_end(argument_list);

	return length;
}

String operator+(const char* cstring, const String& string)
{
	return String(cstring) + string;
}

//#define ENABLE_STRING_TESTS
#ifdef ENABLE_STRING_TESTS
#include <Rocket/Core/STL/string>
#include <SYSClock.h>
ROCKETCORE_API void EMPStringTests()
{
	std::string ss = "test";
	String es = "test";

	es = "hello";
	es.Resize(100);
	es.Erase(4);
	es.Erase(2,100);
	es += "y";

	String sub1 = es.Replace("lo", "l");
	sub1 = sub1.Replace("h", "!");

	EMPTime start;

	{
		// Create a few free buffers
		String tempstring("buffer");
		String tempstring1("buffer1");
		String tempstring2("buffer2");
	}	

	start = SYSClock::GetRealTime();
	for (int i = 0; i < 100000; i++)
	{
		std::string str("test");	
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "SS Assign Short: %f", SYSClock::GetRealTime() - start);
	
	start = SYSClock::GetRealTime();	
	for (int i = 0; i < 100000; i++)
	{
		String str("test");
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "ES Assign Short: %f", SYSClock::GetRealTime() - start);

	start = SYSClock::GetRealTime();
	for (int i = 0; i < 100000; i++)
	{
		std::string str("test this really long string that won't fit in a local buffer");	
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "SS Assign Long: %f", SYSClock::GetRealTime() - start);
	
	start = SYSClock::GetRealTime();	
	for (int i = 0; i < 100000; i++)
	{
		String str("test this really long string that won't fit in a local buffer");
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "ES Assign Long: %f", SYSClock::GetRealTime() - start);

	start = SYSClock::GetRealTime();
	for (int i = 0; i < 100000; i++)
	{
		if (ss == "hello")
		{
			int bob = 10;
		}
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "SS Compare: %f (char*)", SYSClock::GetRealTime() - start);

	std::string compare = "hello";
	start = SYSClock::GetRealTime();
	for (int i = 0; i < 100000; i++)
	{
		if (ss == compare)
		{
			int bob = 10;
		}
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "SS Compare: %f (std::string)", SYSClock::GetRealTime() - start);

	start = SYSClock::GetRealTime();
	for (int i = 0; i < 100000; i++)
	{
		if (es == "hello")
		{
			int bob = 10;
		}
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "ES Compare: %f (char*)", SYSClock::GetRealTime() - start);
	
	String oes = es;
	start = SYSClock::GetRealTime();
	for (int i = 0; i < 100000; i++)
	{
		if (es == oes)
		{
			int bob = 10;
		}
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "ES Compare: %f (String)", SYSClock::GetRealTime() - start);

	start = SYSClock::GetRealTime();
	std::string ss_concat = "hello";
	for (int i = 0; i < 100000; i++)
	{
		ss_concat += "y";
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "SS +=: %f", SYSClock::GetRealTime() - start);

	String es_concat = "hello";
	start = SYSClock::GetRealTime();
	for (int i = 0; i < 100000; i++)
	{
		if (i == 1016)
		{
			int bob = 10;
		}
		es_concat += "y";
	}
	Log::Message(LC_CORE, Log::LT_ALWAYS, "ES +=: %f", SYSClock::GetRealTime() - start);

	const char* x1 = "bob";
	String s;
	String t;
	String u;
	s = "hello";
	t = "hell";
	u = "hello";
	if (s == t)
	{
		int bob = 10;
	}
	if (s == u)
	{
		int bob = 10;
	}

	t = s + u;
	if (t == "hellohello")
	{
		int bob = 10;
	}
	if (t == "x")
	{
		int bob = 10;
	}

	t += u;


	size_t x = s.Find("e");
	size_t y = s.Find("z");
	
	String sub = t.Replace("lo", "l");
	sub = sub.Replace("h", "!");

	sub.FormatString(128, "%s", "hello");
	int bob = 10;
}
#endif

}
}

/*namespace std {

ROCKETCORE_API size_t hash< String >::operator()(const String& string) const
{
	return StringUtilities::FNVHash(string.CString());
}

}*/