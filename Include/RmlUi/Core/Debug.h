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

#ifndef RMLUICOREDEBUG_H
#define RMLUICOREDEBUG_H

#include "Header.h"

// Define for breakpointing.
#if defined (RMLUI_PLATFORM_WIN32)
	#if defined (__MINGW32__)
		#define RMLUI_BREAK {asm("int $0x03");}
	#elif defined (_MSC_VER)
		#define RMLUI_BREAK {__debugbreak();}
	#else
		#define RMLUI_BREAK
	#endif
#elif defined (RMLUI_PLATFORM_LINUX)
	#if defined __GNUC__
		#define RMLUI_BREAK {__builtin_trap();}
	#else
		#define RMLUI_BREAK
	#endif
#elif defined (RMLUI_PLATFORM_MACOSX)
	#define RMLUI_BREAK {__builtin_trap();}
#endif



// Define the LT_ASSERT and RMLUI_VERIFY macros.
#if !defined RMLUI_DEBUG
#define RMLUI_ASSERT(x)
#define RMLUI_ASSERTMSG(x, m)
#define RMLUI_ERROR
#define RMLUI_ERRORMSG(m)
#define RMLUI_VERIFY(x) x
#else
namespace Rml {
namespace Core {

bool RMLUICORE_API Assert(const char* message, const char* file, int line);
#define RMLUI_ASSERT(x) \
if (!(x)) \
{ \
	if (!Rml::Core::Assert("RMLUI_ASSERT("#x")", __FILE__, __LINE__ )) \
	{ \
		RMLUI_BREAK; \
	} \
}
#define RMLUI_ASSERTMSG(x, m)	\
if (!(x)) \
{ \
	if (!Rml::Core::Assert(m, __FILE__, __LINE__ )) \
	{ \
		RMLUI_BREAK; \
	} \
}
#define RMLUI_ERROR \
if (!Rml::Core::Assert("RMLUI_ERROR", __FILE__, __LINE__)) \
{ \
	RMLUI_BREAK; \
}
#define RMLUI_ERRORMSG(m) \
if (!Rml::Core::Assert(m, __FILE__, __LINE__)) \
{ \
	RMLUI_BREAK; \
}
#define RMLUI_VERIFY(x) RMLUI_ASSERT(x)

}
}
#endif

namespace Rml {
namespace Core {

template <bool> struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true>{};	
	
}
}
#define RMLUI_STATIC_ASSERT(cond, msg) Rml::Core::STATIC_ASSERTION_FAILURE<cond> msg; (void)&msg;

#endif
