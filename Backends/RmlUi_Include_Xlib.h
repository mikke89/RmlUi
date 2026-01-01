#pragma once

#ifndef RMLUI_DISABLE_INCLUDE_XLIB

	#include <X11/Xlib.h>

	// The None and Always defines from X.h conflicts with RmlUi code base,
	// use their underlying constants where necessary.
	#ifdef None
		// Value 0L
		#undef None
	#endif
	#ifdef Always
		// Value 2
		#undef Always
	#endif

#endif
