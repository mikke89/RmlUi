#pragma once

#include "../Core/Platform.h"

#if !defined RMLUI_STATIC_LIB
	#ifdef RMLUI_PLATFORM_WIN32
		#ifdef RMLUI_DEBUGGER_EXPORTS
			#define RMLUIDEBUGGER_API __declspec(dllexport)
		#else
			#define RMLUIDEBUGGER_API __declspec(dllimport)
		#endif
	#else
		#define RMLUIDEBUGGER_API __attribute__((visibility("default")))
	#endif
#else
	#define RMLUIDEBUGGER_API
#endif
