/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

 
#ifndef ROCKETCORESTRINGCACHE_H
#define ROCKETCORESTRINGCACHE_H

#include "../../Include/Rocket/Core/String.h"
#include "../../Include/Rocket/Core/Types.h"


namespace Rocket {
namespace Core {

PropertyId GetPropertyId(const String& property_name);
const String& GetName(PropertyId property_id);
PropertyId AddPropertyName(const String& name);

extern const PropertyId MARGIN_TOP;
extern const PropertyId MARGIN_RIGHT;
extern const PropertyId MARGIN_BOTTOM;
extern const PropertyId MARGIN_LEFT;
extern const PropertyId MARGIN;
extern const PropertyId PADDING_TOP;
extern const PropertyId PADDING_RIGHT;
extern const PropertyId PADDING_BOTTOM;
extern const PropertyId PADDING_LEFT;
extern const PropertyId PADDING;
extern const PropertyId BORDER_TOP_WIDTH;
extern const PropertyId BORDER_RIGHT_WIDTH;
extern const PropertyId BORDER_BOTTOM_WIDTH;
extern const PropertyId BORDER_LEFT_WIDTH;
extern const PropertyId BORDER_WIDTH;
extern const PropertyId BORDER_TOP_COLOR;
extern const PropertyId BORDER_RIGHT_COLOR;
extern const PropertyId BORDER_BOTTOM_COLOR;
extern const PropertyId BORDER_LEFT_COLOR;
extern const PropertyId BORDER_COLOR;
extern const PropertyId BORDER_TOP;
extern const PropertyId BORDER_RIGHT;
extern const PropertyId BORDER_BOTTOM;
extern const PropertyId BORDER_LEFT;
extern const PropertyId BORDER;
extern const PropertyId DISPLAY;
extern const PropertyId POSITION;
extern const PropertyId TOP;
extern const PropertyId RIGHT;
extern const PropertyId BOTTOM;
extern const PropertyId LEFT;
extern const PropertyId FLOAT;
extern const PropertyId CLEAR;
extern const PropertyId Z_INDEX;
extern const PropertyId WIDTH;
extern const PropertyId MIN_WIDTH;
extern const PropertyId MAX_WIDTH;
extern const PropertyId HEIGHT;
extern const PropertyId MIN_HEIGHT;
extern const PropertyId MAX_HEIGHT;
extern const PropertyId LINE_HEIGHT;
extern const PropertyId VERTICAL_ALIGN;
extern const PropertyId OVERFLOW_;
extern const PropertyId OVERFLOW_X;
extern const PropertyId OVERFLOW_Y;
extern const PropertyId CLIP;
extern const PropertyId VISIBILITY;
extern const PropertyId BACKGROUND_COLOR;
extern const PropertyId BACKGROUND;
extern const PropertyId COLOR;
extern const PropertyId IMAGE_COLOR;
extern const PropertyId FONT_FAMILY;
extern const PropertyId FONT_CHARSET;
extern const PropertyId FONT_STYLE;
extern const PropertyId FONT_WEIGHT;
extern const PropertyId FONT_SIZE;
extern const PropertyId FONT;
extern const PropertyId TEXT_ALIGN;
extern const PropertyId TEXT_DECORATION;
extern const PropertyId TEXT_TRANSFORM;
extern const PropertyId WHITE_SPACE;
extern const PropertyId CURSOR;
extern const PropertyId DRAG_PROPERTY;
extern const PropertyId TAB_INDEX;
extern const PropertyId SCROLLBAR_MARGIN;

extern const PropertyId PERSPECTIVE;
extern const PropertyId PERSPECTIVE_ORIGIN;
extern const PropertyId PERSPECTIVE_ORIGIN_X;
extern const PropertyId PERSPECTIVE_ORIGIN_Y;
extern const PropertyId TRANSFORM;
extern const PropertyId TRANSFORM_ORIGIN;
extern const PropertyId TRANSFORM_ORIGIN_X;
extern const PropertyId TRANSFORM_ORIGIN_Y;
extern const PropertyId TRANSFORM_ORIGIN_Z;
extern const PropertyId NONE;
extern const PropertyId ALL;

extern const PropertyId TRANSITION;
extern const PropertyId ANIMATION;
extern const PropertyId KEYFRAMES;

extern const PropertyId SCROLL_DEFAULT_STEP_SIZE;
extern const PropertyId OPACITY;
extern const PropertyId POINTER_EVENTS;
extern const PropertyId FOCUS_PROPERTY; // String is "focus", but variable name made distinct from event


EventId GetEventId(const String& property_name);
const String& GetName(EventId event_id);
EventId AddEventName(const String& name);

extern const EventId MOUSEDOWN;
extern const EventId MOUSESCROLL;
extern const EventId MOUSEOVER;
extern const EventId MOUSEOUT;
extern const EventId FOCUS;
extern const EventId BLUR;
extern const EventId KEYDOWN;
extern const EventId MOUSEUP;
extern const EventId CLICK;
extern const EventId DRAG;
extern const EventId DRAGSTART;
extern const EventId LOAD;
extern const EventId UNLOAD;
extern const EventId KEYUP;
extern const EventId TEXTINPUT;
extern const EventId MOUSEMOVE;
extern const EventId DRAGMOVE;
extern const EventId DBLCLICK;
extern const EventId DRAGDROP;
extern const EventId DRAGOUT;
extern const EventId DRAGEND;
extern const EventId DRAGOVER;
extern const EventId RESIZE;
extern const EventId SCROLL;
extern const EventId ANIMATIONEND;
extern const EventId TRANSITIONEND;

}
}

#endif
