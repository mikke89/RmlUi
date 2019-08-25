/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICOREPROFILING_H
#define RMLUICOREPROFILING_H


#ifdef RMLUI_ENABLE_PROFILING

#define TRACY_ENABLE
#include "../../../Dependencies/tracy/Tracy.hpp"

#define RMLUI_ZoneNamed(x,y)       ZoneNamed(x,y)
#define RMLUI_ZoneNamedN(x,y,z)    ZoneNamedN(x,y,z)
#define RMLUI_ZoneNamedC(x,y,z)    ZoneNamedC(x,y,z)
#define RMLUI_ZoneNamedNC(x,y,z,w) ZoneNamedNC(x,y,z,w)

#define RMLUI_ZoneScoped           ZoneScoped
#define RMLUI_ZoneScopedN(x)       ZoneScopedN(x)
#define RMLUI_ZoneScopedC(x)       ZoneScopedC(x)
#define RMLUI_ZoneScopedNC(x,y)    ZoneScopedNC(x,y)

#define RMLUI_ZoneText(x,y)        ZoneText(x,y)
#define RMLUI_ZoneName(x,y)        ZoneName(x,y)

#define RMLUI_TracyPlot(name,val)  TracyPlot(name,val)

#define RMLUI_FrameMark            FrameMark
#define RMLUI_FrameMarkNamed(x)    FrameMarkNamed(x)
#define RMLUI_FrameMarkStart(x)    FrameMarkStart(x)
#define RMLUI_FrameMarkEnd(x)      FrameMarkEnd(x)

#else

#define RMLUI_ZoneNamed(x,y)
#define RMLUI_ZoneNamedN(x,y,z)
#define RMLUI_ZoneNamedC(x,y,z)
#define RMLUI_ZoneNamedNC(x,y,z,w)

#define RMLUI_ZoneScoped
#define RMLUI_ZoneScopedN(x)
#define RMLUI_ZoneScopedC(x)
#define RMLUI_ZoneScopedNC(x,y)

#define RMLUI_ZoneText(x,y)
#define RMLUI_ZoneName(x,y)

#define RMLUI_TracyPlot(name,val)

#define RMLUI_FrameMark
#define RMLUI_FrameMarkNamed(x)
#define RMLUI_FrameMarkStart(x)
#define RMLUI_FrameMarkEnd(x)

#endif


#endif
