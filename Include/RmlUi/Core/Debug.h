#pragma once

#include "Header.h"

// Define for breakpointing.
#if defined(RMLUI_PLATFORM_WIN32)
	#if defined(__MINGW32__)
		#define RMLUI_BREAK       \
			{                     \
				asm("int $0x03"); \
			}
	#elif defined(_MSC_VER)
		#define RMLUI_BREAK     \
			{                   \
				__debugbreak(); \
			}
	#else
		#define RMLUI_BREAK
	#endif
#elif defined(RMLUI_PLATFORM_LINUX)
	#if defined __GNUC__
		#define RMLUI_BREAK       \
			{                     \
				__builtin_trap(); \
			}
	#else
		#define RMLUI_BREAK
	#endif
#elif defined(RMLUI_PLATFORM_MACOSX)
	#define RMLUI_BREAK       \
		{                     \
			__builtin_trap(); \
		}
#else
	#define RMLUI_BREAK
#endif

namespace Rml {

bool RMLUICORE_API Assert(const char* message, const char* file, int line);

}

// Define the RmlUi assertion macros.
#if !defined RMLUI_DEBUG

	#define RMLUI_ASSERT(x)
	#define RMLUI_ASSERTMSG(x, m)
	#define RMLUI_ERROR
	#define RMLUI_ERRORMSG(m)
	#define RMLUI_VERIFY(x) x
	#define RMLUI_ASSERT_NONRECURSIVE

#else

	#define RMLUI_ASSERT(x)                                                   \
		if (!(x))                                                             \
		{                                                                     \
			if (!(::Rml::Assert("RMLUI_ASSERT(" #x ")", __FILE__, __LINE__))) \
			{                                                                 \
				RMLUI_BREAK;                                                  \
			}                                                                 \
		}
	#define RMLUI_ASSERTMSG(x, m)                        \
		if (!(x))                                        \
		{                                                \
			if (!(::Rml::Assert(m, __FILE__, __LINE__))) \
			{                                            \
				RMLUI_BREAK;                             \
			}                                            \
		}
	#define RMLUI_ERROR                                          \
		if (!(::Rml::Assert("RMLUI_ERROR", __FILE__, __LINE__))) \
		{                                                        \
			RMLUI_BREAK;                                         \
		}
	#define RMLUI_ERRORMSG(m)                        \
		if (!(::Rml::Assert(m, __FILE__, __LINE__))) \
		{                                            \
			RMLUI_BREAK;                             \
		}
	#define RMLUI_VERIFY(x) RMLUI_ASSERT(x)

struct RmlUiAssertNonrecursive {
	bool& entered;
	RmlUiAssertNonrecursive(bool& entered) : entered(entered)
	{
		RMLUI_ASSERTMSG(!entered, "A method defined as non-recursive was entered twice!");
		entered = true;
	}
	~RmlUiAssertNonrecursive() { entered = false; }
};

	#define RMLUI_ASSERT_NONRECURSIVE                   \
		static bool rmlui_nonrecursive_entered = false; \
		RmlUiAssertNonrecursive rmlui_nonrecursive(rmlui_nonrecursive_entered)

#endif // RMLUI_DEBUG
