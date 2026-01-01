#pragma once

#include "Platform.h"

// Note: Changing a RMLUICORE_API_INLINE method
// breaks ABI compatibility!!

#if !defined RMLUI_STATIC_LIB
	#if defined RMLUI_PLATFORM_WIN32
		#if defined RMLUI_CORE_EXPORTS
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
