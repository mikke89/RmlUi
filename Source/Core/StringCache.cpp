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



template <typename ID>
class IdNameMap {

	static std::vector<String> name_map;  // IDs are indices into the NameMap
	static UnorderedMap<String, ID> reverse_map;

protected:
	IdNameMap(ID id_num_to_initialize) {
		name_map.resize((size_t)id_num_to_initialize);
		reverse_map.reserve((size_t)id_num_to_initialize);
	}

	void add(ID id, const String& name) {
		name_map[(size_t)id] = name;
		reverse_map.emplace(name, id);
	}

	void assert_all_inserted(ID id_num_to_initialize) const {
		ROCKET_ASSERT(name_map.size() == (size_t)id_num_to_initialize && reverse_map.size() == (size_t)id_num_to_initialize);
	}

public:
	ID GetId(const String& name)
	{
		if (auto it = reverse_map.find(ToLower(name)); it != reverse_map.end())
			return it->second;
		return ID::Invalid;
	}
	const String& GetName(ID id)
	{
		if (static_cast<size_t>(id) < name_map.size())
			return name_map[static_cast<size_t>(id)];
		return name_map[static_cast<size_t>(ID::Invalid)];
	}
	
	ID CreateIdFromName(const String& name)
	{
		String lower = ToLower(name);
		ID next_id = static_cast<ID>(name_map.size());

		// Only insert if not already in list
		auto [it, inserted] = reverse_map.emplace(lower, next_id);
		if (inserted)
			name_map.push_back(lower);

		// Return the property id that already existed, or the new one if inserted
		return it->second;
	}
};

class PropertyIdNameMap : public IdNameMap<PropertyId>
{
public:
	PropertyIdNameMap();
};

inline PropertyIdNameMap::PropertyIdNameMap() : IdNameMap(PropertyId::NumDefinedIds)
{
	add(PropertyId::Invalid, "invalid_property");
	add(PropertyId::MarginTop, "margin-top");
	add(PropertyId::MarginRight, "margin-right");
	add(PropertyId::MarginBottom, "margin-bottom");
	add(PropertyId::MarginLeft, "margin-left");
	add(PropertyId::Margin, "margin");
	add(PropertyId::PaddingTop, "padding-top");
	add(PropertyId::PaddingRight, "padding-right");
	add(PropertyId::PaddingBottom, "padding-bottom");
	add(PropertyId::PaddingLeft, "padding-left");
	add(PropertyId::Padding, "padding");
	add(PropertyId::BorderTopWidth, "border-top-width");
	add(PropertyId::BorderRightWidth, "border-right-width");
	add(PropertyId::BorderBottomWidth, "border-bottom-width");
	add(PropertyId::BorderLeftWidth, "border-left-width");
	add(PropertyId::BorderWidth, "border-width");
	add(PropertyId::BorderTopColor, "border-top-color");
	add(PropertyId::BorderRightColor, "border-right-color");
	add(PropertyId::BorderBottomColor, "border-bottom-color");
	add(PropertyId::BorderLeftColor, "border-left-color");
	add(PropertyId::BorderColor, "border-color");
	add(PropertyId::BorderTop, "border-top");
	add(PropertyId::BorderRight, "border-right");
	add(PropertyId::BorderBottom, "border-bottom");
	add(PropertyId::BorderLeft, "border-left");
	add(PropertyId::Border, "border");
	add(PropertyId::Display, "display");
	add(PropertyId::Position, "position");
	add(PropertyId::Top, "top");
	add(PropertyId::Right, "right");
	add(PropertyId::Bottom, "bottom");
	add(PropertyId::Left, "left");
	add(PropertyId::Float, "float");
	add(PropertyId::Clear, "clear");
	add(PropertyId::ZIndex, "z-index");
	add(PropertyId::Width, "width");
	add(PropertyId::MinWidth, "min-width");
	add(PropertyId::MaxWidth, "max-width");
	add(PropertyId::Height, "height");
	add(PropertyId::MinHeight, "min-height");
	add(PropertyId::MaxHeight, "max-height");
	add(PropertyId::LineHeight, "line-height");
	add(PropertyId::VerticalAlign, "vertical-align");
	add(PropertyId::Overflow, "overflow");
	add(PropertyId::OverflowX, "overflow-x");
	add(PropertyId::OverflowY, "overflow-y");
	add(PropertyId::Clip, "clip");
	add(PropertyId::Visibility, "visibility");
	add(PropertyId::BackgroundColor, "background-color");
	add(PropertyId::Background, "background");
	add(PropertyId::Color, "color");
	add(PropertyId::ImageColor, "image-color");
	add(PropertyId::FontFamily, "font-family");
	add(PropertyId::FontCharset, "font-charset");
	add(PropertyId::FontStyle, "font-style");
	add(PropertyId::FontWeight, "font-weight");
	add(PropertyId::FontSize, "font-size");
	add(PropertyId::Font, "font");
	add(PropertyId::TextAlign, "text-align");
	add(PropertyId::TextDecoration, "text-decoration");
	add(PropertyId::TextTransform, "text-transform");
	add(PropertyId::WhiteSpace, "white-space");
	add(PropertyId::Cursor, "cursor");
	add(PropertyId::Drag, "drag");
	add(PropertyId::TabIndex, "tab-index");
	add(PropertyId::ScrollbarMargin, "scrollbar-margin");
	add(PropertyId::Perspective, "perspective");
	add(PropertyId::PerspectiveOrigin, "perspective-origin");
	add(PropertyId::PerspectiveOriginX, "perspective-origin-x");
	add(PropertyId::PerspectiveOriginY, "perspective-origin-y");
	add(PropertyId::Transform, "transform");
	add(PropertyId::TransformOrigin, "transform-origin");
	add(PropertyId::TransformOriginX, "transform-origin-x");
	add(PropertyId::TransformOriginY, "transform-origin-y");
	add(PropertyId::TransformOriginZ, "transform-origin-z");
	add(PropertyId::None, "none");
	add(PropertyId::All, "all");
	add(PropertyId::Transition, "transition");
	add(PropertyId::Animation, "animation");
	add(PropertyId::Keyframes, "keyframes");
	add(PropertyId::ScrollDefaultStepSize, "scroll-default-step-size");
	add(PropertyId::Opacity, "opacity");
	add(PropertyId::PointerEvents, "pointer-events");
	add(PropertyId::Focus, "focus");

	assert_all_inserted(PropertyId::NumDefinedIds);
}


class EventIdNameMap : public IdNameMap<EventId>
{
public:
	EventIdNameMap();
};

inline EventIdNameMap::EventIdNameMap() : IdNameMap(EventId::NumDefinedIds)
{
	add(EventId::Invalid, "invalid_event");
	add(EventId::Mousedown, "mousedown");
	add(EventId::Mousescroll, "mousescroll");
	add(EventId::Mouseover, "mouseover");
	add(EventId::Mouseout, "mouseout");
	add(EventId::Focus, "focus");
	add(EventId::Blur, "blur");
	add(EventId::Keydown, "keydown");
	add(EventId::Mouseup, "mouseup");
	add(EventId::Click, "click");
	add(EventId::Load, "load");
	add(EventId::Unload, "unload");
	add(EventId::Show, "show");
	add(EventId::Hide, "hide");
	add(EventId::Keyup, "keyup");
	add(EventId::Textinput, "textinput");
	add(EventId::Mousemove, "mousemove");
	add(EventId::Dragmove, "dragmove");
	add(EventId::Dblclick, "dblclick");
	add(EventId::Drag, "drag");
	add(EventId::Dragstart, "dragstart");
	add(EventId::Dragover, "dragover");
	add(EventId::Dragdrop, "dragdrop");
	add(EventId::Dragout, "dragout");
	add(EventId::Dragend, "dragend");
	add(EventId::Handledrag, "handledrag");
	add(EventId::Resize, "resize");
	add(EventId::Scroll, "scroll");
	add(EventId::Animationend, "animationend");
	add(EventId::Transitionend, "transitionend");

	assert_all_inserted(EventId::NumDefinedIds);
}

static PropertyIdNameMap property_map;

PropertyId GetPropertyId(const String& property_name) {
	return property_map.GetId(property_name);
}
PropertyId CreateOrGetPropertyId(const String& name) {
	return property_map.CreateIdFromName(name);
}
const String& GetName(PropertyId property_id) {
	return property_map.GetName(property_id);
}

static EventIdNameMap event_map;

EventId GetEventId(const String& event_name) {
	return event_map.GetId(event_name);
}
EventId CreateOrGetEventId(const String& name) {
	return event_map.CreateIdFromName(name);
}
const String& GetName(EventId event_id) {
	return event_map.GetName(event_id);
}

//
//class PseudoIdNameMap : public IdNameMap<PseudoId>
//{
//public:
//	PseudoIdNameMap();
//};
//
//inline PseudoIdNameMap::PseudoIdNameMap() : IdNameMap(PseudoId::NumDefinedIds)
//{
//	add(PseudoId::Invalid, "invalid_pseudo_class");
//	add(PseudoId::Hover, "hover");
//	add(PseudoId::Active, "active");
//	add(PseudoId::Focus, "focus");
//	add(PseudoId::Dragged, "dragged");
//
//	add(PseudoId::Disabled, "disabled");
//	add(PseudoId::Selected, "selected");
//	add(PseudoId::Checked, "checked");
//
//	assert_all_inserted(PseudoId::NumDefinedIds);
//}
//
//
//static PseudoIdNameMap pseudo_map;
//
//PseudoId GetPseudoId(const String& event_name) {
//	return pseudo_map.GetId(event_name);
//}
//PseudoId GetOrCreatePseudoId(const String& name) {
//	return pseudo_map.CreateIdFromName(name);
//}
//const String& GetName(PseudoId pseudo_id) {
//	return pseudo_map.GetName(pseudo_id);
//}



}
}
