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

 
#ifndef ROCKETCOREID_H
#define ROCKETCOREID_H

#include "Types.h"

namespace Rocket {
namespace Core {

PropertyId GetPropertyId(const String& property_name);
PropertyId CreateOrGetPropertyId(const String& name);
const String& GetName(PropertyId property_id);

EventId GetEventId(const String& event_name);
EventId CreateOrGetEventId(const String& name);
const String& GetName(EventId event_id); 



enum class PropertyId : uint16_t 
{
	Invalid,

	MarginTop,
	MarginRight,
	MarginBottom,
	MarginLeft,
	Margin,
	PaddingTop,
	PaddingRight,
	PaddingBottom,
	PaddingLeft,
	Padding,
	BorderTopWidth,
	BorderRightWidth,
	BorderBottomWidth,
	BorderLeftWidth,
	BorderWidth,
	BorderTopColor,
	BorderRightColor,
	BorderBottomColor,
	BorderLeftColor,
	BorderColor,
	BorderTop,
	BorderRight,
	BorderBottom,
	BorderLeft,
	Border,
	Display,
	Position,
	Top,
	Right,
	Bottom,
	Left,
	Float,
	Clear,
	ZIndex,
	Width,
	MinWidth,
	MaxWidth,
	Height,
	MinHeight,
	MaxHeight,
	LineHeight,
	VerticalAlign,
	Overflow,
	OverflowX,
	OverflowY,
	Clip,
	Visibility,
	BackgroundColor,
	Background,
	Color,
	ImageColor,
	FontFamily,
	FontCharset,
	FontStyle,
	FontWeight,
	FontSize,
	Font,
	TextAlign,
	TextDecoration,
	TextTransform,
	WhiteSpace,
	Cursor,
	Drag,
	TabIndex,
	ScrollbarMargin,

	Perspective,
	PerspectiveOrigin,
	PerspectiveOriginX,
	PerspectiveOriginY,
	Transform,
	TransformOrigin,
	TransformOriginX,
	TransformOriginY,
	TransformOriginZ,
	None,
	All,

	Transition,
	Animation,
	Keyframes,

	ScrollDefaultStepSize,
	Opacity,
	PointerEvents,
	Focus,

	NumDefinedIds,
	FirstCustomId = NumDefinedIds
};


enum class EventId : uint16_t 
{
	Invalid,

	Mousedown,
	Mousescroll,
	Mouseover,
	Mouseout,
	Focus,
	Blur,
	Keydown,
	Mouseup,
	Click,
	Load,
	Unload,
	Show,
	Hide,
	Keyup,
	Textinput,
	Mousemove,
	Dragmove,
	Dblclick,
	Drag,
	Dragstart,
	Dragover,
	Dragdrop,
	Dragout,
	Dragend,
	Handledrag,
	Resize,
	Scroll,
	Scrollchange,
	Animationend,
	Transitionend,

	NumDefinedIds,
	FirstCustomId = NumDefinedIds
};

// Edit: Actually, this might be too complicated due to structural pseudo class selectors
//PseudoId GetPseudoId(const String& event_name);
//PseudoId GetOrCreatePseudoId(const String& name);
//const String& GetName(PseudoId pseudo_id);
//enum class PseudoId : uint16_t
//{
//	Invalid,
//
//	Hover,
//	Active,
//	Focus,
//	Dragged,
//
//	Disabled,
//	Selected,
//	Checked,
//
//	NumDefinedIds,
//	FirstCustomId = NumDefinedIds
//};


}
}

#endif
