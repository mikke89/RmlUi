#pragma once

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
	#define RMLUI_PLATFORM_WIN32_NATIVE

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
