#pragma once

#ifdef RMLUI_TRACY_PROFILING

	#include <tracy/Tracy.hpp>

	#define RMLUI_ZoneNamed(varname, active) ZoneNamed(varname, active)
	#define RMLUI_ZoneNamedN(varname, name, active) ZoneNamedN(varname, name, active)
	#define RMLUI_ZoneNamedC(varname, color, active) ZoneNamedC(varname, color, active)
	#define RMLUI_ZoneNamedNC(varname, name, color, active) ZoneNamedNC(varname, name, color, active)

	#define RMLUI_ZoneScoped ZoneScoped
	#define RMLUI_ZoneScopedN(name) ZoneScopedN(name)
	#define RMLUI_ZoneScopedC(color) ZoneScopedC(color)
	#define RMLUI_ZoneScopedNC(name, color) ZoneScopedNC(name, color)

	#define RMLUI_ZoneText(txt, size) ZoneText(txt, size)
	#define RMLUI_ZoneName(txt, size) ZoneName(txt, size)

	#define RMLUI_TracyPlot(name, val) TracyPlot(name, val)

	#define RMLUI_FrameMark FrameMark
	#define RMLUI_FrameMarkNamed(name) FrameMarkNamed(name)
	#define RMLUI_FrameMarkStart(name) FrameMarkStart(name)
	#define RMLUI_FrameMarkEnd(name) FrameMarkEnd(name)

#else

	#define RMLUI_ZoneNamed(varname, active)
	#define RMLUI_ZoneNamedN(varname, name, active)
	#define RMLUI_ZoneNamedC(varname, color, active)
	#define RMLUI_ZoneNamedNC(varname, name, color, active)

	#define RMLUI_ZoneScoped
	#define RMLUI_ZoneScopedN(name)
	#define RMLUI_ZoneScopedC(color)
	#define RMLUI_ZoneScopedNC(name, color)

	#define RMLUI_ZoneText(txt, size)
	#define RMLUI_ZoneName(txt, size)

	#define RMLUI_TracyPlot(name, val)

	#define RMLUI_FrameMark
	#define RMLUI_FrameMarkNamed(name)
	#define RMLUI_FrameMarkStart(name)
	#define RMLUI_FrameMarkEnd(name)

#endif
