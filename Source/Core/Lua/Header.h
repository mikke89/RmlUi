#ifndef ROCKETLUAHEADER_H
#define ROCKETLUAHEADER_H

#include <Rocket/Core/Platform.h>

#if !defined STATIC_LIB
	#ifdef ROCKET_PLATFORM_WIN32
		#ifdef RocketLua_EXPORTS
			#define ROCKETLUA_API __declspec(dllexport)
		#else
			#define ROCKETLUA_API __declspec(dllimport)
		#endif
	#else
		#define ROCKETLUA_API __attribute__((visibility("default")))
	#endif
#else
	#define ROCKETLUA_API
#endif

#endif