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

 
#include "precompiled.h"

namespace Rocket {
namespace Core {

static std::vector<String> PropertyNameMap;
static UnorderedMap<String, PropertyId> PropertyReverseMap;

PropertyId GetPropertyId(const String& property_name)
{
	if (auto it = PropertyReverseMap.find(ToLower(property_name)); it != PropertyReverseMap.end())
		return it->second;
	return InvalidPropertyId;
}

const String& GetName(PropertyId property_id)
{
	if (static_cast<size_t>(property_id) < PropertyNameMap.size())
		return PropertyNameMap[static_cast<size_t>(property_id)];
	return PropertyNameMap[0];
}

PropertyId AddPropertyName(const String & name)
{
	//ROCKET_ASSERT(ToLower(name) == name);
	String lower = ToLower(name);
	if (PropertyNameMap.empty())
	{
		PropertyReverseMap.reserve(200);
		PropertyNameMap.reserve(200);
		PropertyNameMap.push_back("invalid_property");
	}
	// Only insert if not already in list
	PropertyId next_id = static_cast<PropertyId>(PropertyNameMap.size());
	auto [it, inserted] = PropertyReverseMap.emplace(lower, next_id);
	if (inserted)
		PropertyNameMap.push_back(lower);

	// Return the property id that already existed, or the new one if inserted
	return it->second;
}



static std::vector<String> EventNameMap;
static UnorderedMap<String, EventId> EventReverseMap;

EventId GetEventId(const String& event_name)
{
	if (auto it = EventReverseMap.find(ToLower(event_name)); it != EventReverseMap.end())
		return it->second;
	return InvalidEventId;
}

const String& GetName(EventId property_id)
{
	if (static_cast<size_t>(property_id) < EventNameMap.size())
		return EventNameMap[static_cast<size_t>(property_id)];
	return EventNameMap[0];
}

EventId AddEventName(const String & name)
{
	ROCKET_ASSERT(ToLower(name) == name);
	if (EventNameMap.empty())
	{
		EventNameMap.reserve(150);
		EventNameMap.push_back("invalid_property");
	}
	EventId result = static_cast<EventId>(EventNameMap.size());
	EventNameMap.push_back(name);
	EventReverseMap[name] = result;
	return result;
}

const PropertyId MARGIN_TOP = AddPropertyName("margin-top");
const PropertyId MARGIN_RIGHT = AddPropertyName("margin-right");
const PropertyId MARGIN_BOTTOM = AddPropertyName("margin-bottom");
const PropertyId MARGIN_LEFT = AddPropertyName("margin-left");
const PropertyId MARGIN = AddPropertyName("margin");
const PropertyId PADDING_TOP = AddPropertyName("padding-top");
const PropertyId PADDING_RIGHT = AddPropertyName("padding-right");
const PropertyId PADDING_BOTTOM = AddPropertyName("padding-bottom");
const PropertyId PADDING_LEFT = AddPropertyName("padding-left");
const PropertyId PADDING = AddPropertyName("padding");
const PropertyId BORDER_TOP_WIDTH = AddPropertyName("border-top-width");
const PropertyId BORDER_RIGHT_WIDTH = AddPropertyName("border-right-width");
const PropertyId BORDER_BOTTOM_WIDTH = AddPropertyName("border-bottom-width");
const PropertyId BORDER_LEFT_WIDTH = AddPropertyName("border-left-width");
const PropertyId BORDER_WIDTH = AddPropertyName("border-width");
const PropertyId BORDER_TOP_COLOR = AddPropertyName("border-top-color");
const PropertyId BORDER_RIGHT_COLOR = AddPropertyName("border-right-color");
const PropertyId BORDER_BOTTOM_COLOR = AddPropertyName("border-bottom-color");
const PropertyId BORDER_LEFT_COLOR = AddPropertyName("border-left-color");
const PropertyId BORDER_COLOR = AddPropertyName("border-color");
const PropertyId BORDER_TOP = AddPropertyName("border-top");
const PropertyId BORDER_RIGHT = AddPropertyName("border-right");
const PropertyId BORDER_BOTTOM = AddPropertyName("border-bottom");
const PropertyId BORDER_LEFT = AddPropertyName("border-left");
const PropertyId BORDER = AddPropertyName("border");
const PropertyId DISPLAY = AddPropertyName("display");
const PropertyId POSITION = AddPropertyName("position");
const PropertyId TOP = AddPropertyName("top");
const PropertyId RIGHT = AddPropertyName("right");
const PropertyId BOTTOM = AddPropertyName("bottom");
const PropertyId LEFT = AddPropertyName("left");
const PropertyId FLOAT = AddPropertyName("float");
const PropertyId CLEAR = AddPropertyName("clear");
const PropertyId Z_INDEX = AddPropertyName("z-index");
const PropertyId WIDTH = AddPropertyName("width");
const PropertyId MIN_WIDTH = AddPropertyName("min-width");
const PropertyId MAX_WIDTH = AddPropertyName("max-width");
const PropertyId HEIGHT = AddPropertyName("height");
const PropertyId MIN_HEIGHT = AddPropertyName("min-height");
const PropertyId MAX_HEIGHT = AddPropertyName("max-height");
const PropertyId LINE_HEIGHT = AddPropertyName("line-height");
const PropertyId VERTICAL_ALIGN = AddPropertyName("vertical-align");
const PropertyId OVERFLOW_ = AddPropertyName("overflow");
const PropertyId OVERFLOW_X = AddPropertyName("overflow-x");
const PropertyId OVERFLOW_Y = AddPropertyName("overflow-y");
const PropertyId CLIP = AddPropertyName("clip");
const PropertyId VISIBILITY = AddPropertyName("visibility");
const PropertyId BACKGROUND_COLOR = AddPropertyName("background-color");
const PropertyId BACKGROUND = AddPropertyName("background");
const PropertyId COLOR = AddPropertyName("color");
const PropertyId IMAGE_COLOR = AddPropertyName("image-color");
const PropertyId FONT_FAMILY = AddPropertyName("font-family");
const PropertyId FONT_CHARSET = AddPropertyName("font-charset");
const PropertyId FONT_STYLE = AddPropertyName("font-style");
const PropertyId FONT_WEIGHT = AddPropertyName("font-weight");
const PropertyId FONT_SIZE = AddPropertyName("font-size");
const PropertyId FONT = AddPropertyName("font");
const PropertyId TEXT_ALIGN = AddPropertyName("text-align");
const PropertyId TEXT_DECORATION = AddPropertyName("text-decoration");
const PropertyId TEXT_TRANSFORM = AddPropertyName("text-transform");
const PropertyId WHITE_SPACE = AddPropertyName("white-space");
const PropertyId CURSOR = AddPropertyName("cursor");
const PropertyId DRAG_PROPERTY = AddPropertyName("drag");
const PropertyId TAB_INDEX = AddPropertyName("tab-index");
const PropertyId SCROLLBAR_MARGIN = AddPropertyName("scrollbar-margin");
const PropertyId PERSPECTIVE = AddPropertyName("perspective");
const PropertyId PERSPECTIVE_ORIGIN = AddPropertyName("perspective-origin");
const PropertyId PERSPECTIVE_ORIGIN_X = AddPropertyName("perspective-origin-x");
const PropertyId PERSPECTIVE_ORIGIN_Y = AddPropertyName("perspective-origin-y");
const PropertyId TRANSFORM = AddPropertyName("transform");
const PropertyId TRANSFORM_ORIGIN = AddPropertyName("transform-origin");
const PropertyId TRANSFORM_ORIGIN_X = AddPropertyName("transform-origin-x");
const PropertyId TRANSFORM_ORIGIN_Y = AddPropertyName("transform-origin-y");
const PropertyId TRANSFORM_ORIGIN_Z = AddPropertyName("transform-origin-z");
const PropertyId NONE = AddPropertyName("none");
const PropertyId ALL = AddPropertyName("all");
const PropertyId TRANSITION = AddPropertyName("transition");
const PropertyId ANIMATION = AddPropertyName("animation");
const PropertyId KEYFRAMES = AddPropertyName("keyframes");
const PropertyId SCROLL_DEFAULT_STEP_SIZE = AddPropertyName("scroll-default-step-size");
const PropertyId OPACITY = AddPropertyName("opacity");
const PropertyId POINTER_EVENTS = AddPropertyName("pointer-events");
const PropertyId FOCUS_PROPERTY = AddPropertyName("focus");


const EventId MOUSEDOWN = AddEventName("mousedown");
const EventId MOUSESCROLL = AddEventName("mousescroll");
const EventId MOUSEOVER = AddEventName("mouseover");
const EventId MOUSEOUT = AddEventName("mouseout");
const EventId FOCUS = AddEventName("focus");
const EventId BLUR = AddEventName("blur");
const EventId KEYDOWN = AddEventName("keydown");
const EventId MOUSEUP = AddEventName("mouseup");
const EventId CLICK = AddEventName("click");
const EventId DRAG = AddEventName("drag");
const EventId DRAGSTART = AddEventName("dragstart");
const EventId DRAGOVER = AddEventName("dragover");
const EventId LOAD = AddEventName("load");
const EventId UNLOAD = AddEventName("unload");
const EventId KEYUP = AddEventName("keyup");
const EventId TEXTINPUT = AddEventName("textinput");
const EventId MOUSEMOVE = AddEventName("mousemove");
const EventId DRAGMOVE = AddEventName("dragmove");
const EventId DBLCLICK = AddEventName("dblclick");
const EventId DRAGDROP = AddEventName("dragdrop");
const EventId DRAGOUT = AddEventName("dragout");
const EventId DRAGEND = AddEventName("dragend");
const EventId RESIZE = AddEventName("resize");
const EventId SCROLL = AddEventName("scroll");
const EventId ANIMATIONEND = AddEventName("animationend");
const EventId TRANSITIONEND = AddEventName("transitionend");

}
}
