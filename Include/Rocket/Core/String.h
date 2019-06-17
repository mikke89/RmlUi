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
#include <string>
#include <vector>

namespace Rocket {
namespace Core {

typedef std::string String;
typedef std::wstring WString;
typedef std::vector< String > StringList;

// Redefine Windows APIs as their STDC counterparts.
#ifdef ROCKET_PLATFORM_WIN32
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#endif

ROCKETCORE_API int FormatString(String& string, size_t max_size, const char* format, ...);
ROCKETCORE_API String CreateString(size_t max_size, const char* format, ...);

ROCKETCORE_API String ToLower(const String& string);
ROCKETCORE_API String Replace(String subject, const String& search, const String& replace);
ROCKETCORE_API String Replace(String subject, char search, char replace);

ROCKETCORE_API WString ToWideString(const String& str);
ROCKETCORE_API String ToUTF8(const WString& wstr);

}
}

#endif
