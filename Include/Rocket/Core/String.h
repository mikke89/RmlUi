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

#ifndef ROCKETCORESTRING_H
#define ROCKETCORESTRING_H

#include "Header.h"
#include "StringBase.h"
#include <stdarg.h>
#include <string.h>
#include <vector>
#include <string>

namespace Rocket {
namespace Core {

//typedef StringBase< char > String;
typedef std::string String;
typedef std::vector< String > StringList;

// Redefine Windows APIs as their STDC counterparts.
#ifdef ROCKET_PLATFORM_WIN32
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#endif

int FormatString(String& string, size_t max_size, const char* format, ...);
String CreateString(size_t max_size, const char* format, ...);

String ToLower(const String& string);
std::string Replace(std::string subject, const std::string& search, const std::string& replace);

std::wstring ToWideString(const std::string& str);
std::string ToUTF8(const std::wstring& wstr);

struct hash_str_lowercase {
	std::size_t operator()(const String& string) const
	{
		std::hash<String> hash_fn;
		return hash_fn(ToLower(string));
	}
};

}
}
//
//namespace std {
//	template <> struct hash<::Rocket::Core::String> {
//		std::size_t operator()(const ::Rocket::Core::String& string) const
//		{
//			return string.Hash();
//		}
//	};
//}

#endif
