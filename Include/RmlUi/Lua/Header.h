#pragma once

#include <RmlUi/Core/Platform.h>

#ifdef RMLUILUA_API
	#undef RMLUILUA_API
#endif

#if !defined RMLUI_STATIC_LIB
	#ifdef RMLUI_PLATFORM_WIN32
		#if defined RMLUI_LUA_EXPORTS
			#define RMLUILUA_API __declspec(dllexport)
		#else
			#define RMLUILUA_API __declspec(dllimport)
		#endif
	#else
		#define RMLUILUA_API __attribute__((visibility("default")))
	#endif
#else
	#define RMLUILUA_API
#endif
