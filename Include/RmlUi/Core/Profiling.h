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

#ifndef RMLUI_CORE_PROFILING_H
#define RMLUI_CORE_PROFILING_H

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

#endif
