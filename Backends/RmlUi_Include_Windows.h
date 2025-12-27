#pragma once

#ifndef RMLUI_DISABLE_INCLUDE_WINDOWS

	#if !defined _WIN32_WINNT || _WIN32_WINNT < 0x0601
		#undef _WIN32_WINNT
		// Target Windows 7
		#define _WIN32_WINNT 0x0601
	#endif

	#define UNICODE
	#define _UNICODE
	#define WIN32_LEAN_AND_MEAN
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <windows.h>

namespace Backend {
struct RmlProcessEventInfo {
	union {
	#ifdef _WIN32
		HWND hwnd;
		UINT msg;
		WPARAM wParam;
		LPARAM lParam;
	#endif
	};
	// todo: add other info that comes from window proc on different platforms
};
} // namespace Backend

#endif
