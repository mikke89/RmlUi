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

#ifndef RMLUI_CORE_HEADER_H
#define RMLUI_CORE_HEADER_H

#include "Platform.h"

// Note: Changing a RMLUICORE_API_INLINE method
// breaks ABI compatibility!!

#if !defined RMLUI_STATIC_LIB
	#if defined RMLUI_PLATFORM_WIN32
		#if defined RmlCore_EXPORTS
			#define RMLUICORE_API __declspec(dllexport)
			// Note: Changing a RMLUICORE_API_INLINE method
			// breaks ABI compatibility!!

			// This results in an exported method from the DLL
			// that may be inlined in DLL clients, or if not
			// possible the client may choose to import the copy
			// in the DLL if it can not be inlined.
			#define RMLUICORE_API_INLINE __declspec(dllexport) inline
		#else
			#define RMLUICORE_API __declspec(dllimport)
			// Note: Changing a RMLUICORE_API_INLINE method
			// breaks ABI compatibility!!

			// Based on the warnngs emitted by GCC/MinGW if using
			// dllimport and inline together, the information at
			// http://msdn.microsoft.com/en-us/library/xa0d9ste.aspx
			// using dllimport inline is tricky.
			#if defined(_MSC_VER)
				// VisualStudio dllimport inline is supported
				// and may be expanded to inline code when the
				// /Ob1 or /Ob2 options are given for inline
				// expansion, or pulled from the DLL if it can
				// not be inlined.
				#define RMLUICORE_API_INLINE __declspec(dllimport) inline
			#else
				// MinGW 32/64 dllimport inline is not supported
				// and dllimport is ignored, so we avoid using
				// it here to squelch compiler generated
				// warnings.
				#define RMLUICORE_API_INLINE inline
			#endif
		#endif
	#else
		#define RMLUICORE_API __attribute__((visibility("default")))
		// Note: Changing a RMLUICORE_API_INLINE method
		// breaks ABI compatibility!!
		#define RMLUICORE_API_INLINE __attribute__((visibility("default"))) inline
	#endif
#else
	#define RMLUICORE_API
	// Note: Changing a RMLUICORE_API_INLINE method
	// breaks ABI compatibility!!
	#define RMLUICORE_API_INLINE inline
#endif

#endif
