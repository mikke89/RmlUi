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

#ifndef RMLUI_CORE_PLATFORM_H
#define RMLUI_CORE_PLATFORM_H

#if defined __WIN32__ || defined _WIN32
	#define RMLUI_PLATFORM_WIN32
	#define RMLUI_PLATFORM_NAME "win32"
#elif defined __APPLE_CC__
	#define RMLUI_PLATFORM_UNIX
	#define RMLUI_PLATFORM_MACOSX
	#define RMLUI_PLATFORM_NAME "macosx"
#elif defined __EMSCRIPTEN__
	#define RMLUI_PLATFORM_UNIX
	#define RMLUI_PLATFORM_EMSCRIPTEN
	#define RMLUI_PLATFORM_NAME "emscripten"
#else
	#define RMLUI_PLATFORM_UNIX
	#define RMLUI_PLATFORM_LINUX
	#define RMLUI_PLATFORM_NAME "linux"
#endif

#if !defined NDEBUG && !defined RMLUI_DEBUG
	#define RMLUI_DEBUG
#endif

#if defined __LP64__ || defined _M_X64 || defined _WIN64 || defined __MINGW64__ || defined _LP64
	#define RMLUI_ARCH_64
#else
	#define RMLUI_ARCH_32
#endif

#if defined(RMLUI_PLATFORM_WIN32) && !defined(__MINGW32__)
	// declaration of 'identifier' hides class member
	#pragma warning(disable : 4458)

	// <type> needs to have dll-interface to be used by clients
	#pragma warning(disable : 4251)

	// <function> was declared deprecated
	#pragma warning(disable : 4996)
#endif

// Tell the compiler of printf-like functions, warns on incorrect usage.
#if defined __MINGW32__
	#define RMLUI_ATTRIBUTE_FORMAT_PRINTF(i, f) __attribute__((format(__MINGW_PRINTF_FORMAT, i, f)))
#elif defined __GNUC__ || defined __clang__
	#define RMLUI_ATTRIBUTE_FORMAT_PRINTF(i, f) __attribute__((format(printf, i, f)))
#else
	#define RMLUI_ATTRIBUTE_FORMAT_PRINTF(i, f)
#endif

#endif
