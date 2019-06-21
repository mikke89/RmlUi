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

#ifndef RMLUICORESTRING_H
#define RMLUICORESTRING_H

#include "Header.h"
#include "StringBase.h"
#include <stdarg.h>
#include <string.h>
#include <vector>

namespace Rml {
namespace Core {

typedef StringBase< char > String;
typedef std::vector< String > StringList;

// Template specialisation of the constructor and FormatString() methods that use variable argument lists.
template<>
RMLUICORE_API StringBase<char>::StringBase(StringBase<char>::size_type max_size, const char* fmt, ...);
template<>
RMLUICORE_API int StringBase<char>::FormatString(StringBase<char>::size_type max_size, const char* fmt, ...);

// Global operators for adding C strings to strings.
RMLUICORE_API String operator+(const char* cstring, const String& string);

// partial specialization follows

/* !!! CHANGING THIS METHOD BREAKS ABI COMPATIBILITY DUE TO INLINING !!! */
template<>
RMLUICORE_API_INLINE bool StringBase< char >::operator<(const char * compare) const
{
	return strcmp( value, compare ) < 0;
}

/* !!! CHANGING THIS METHOD BREAKS ABI COMPATIBILITY DUE TO INLINING !!! */
template<>
RMLUICORE_API_INLINE bool StringBase< char >::operator==(const char * compare) const
{
	return strcmp( value, compare ) == 0;
}

/* !!! CHANGING THIS METHOD BREAKS ABI COMPATIBILITY DUE TO INLINING !!! */
template<>
RMLUICORE_API_INLINE bool StringBase< char >::operator!=(const char * compare) const
{
	return strcmp( value, compare ) != 0;
}

// Redefine Windows APIs as their STDC counterparts.
#ifdef RMLUI_PLATFORM_WIN32
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#endif

struct hash_str_lowercase {
	std::size_t operator()(const ::Rml::Core::String& string) const
	{
		auto str_lower = string.ToLower();
		return str_lower.Hash();
	}
};
}
}

namespace std {
	template <> struct hash<::Rml::Core::String> {
		std::size_t operator()(const ::Rml::Core::String& string) const
		{
			return string.Hash();
		}
	};
}

#endif
