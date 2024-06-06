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

#ifndef RMLUI_CORE_ID_H
#define RMLUI_CORE_ID_H

#include <stdint.h>

namespace Rml {

enum class ShorthandId : uint8_t {
	Invalid,

	/*
	  The following values define the shorthand ids for the main stylesheet specification.
	  These values must not be used in places that have their own property specification,
	  such as decorators and font-effects.
	*/
	Margin,
	Padding,
	BorderWidth,
	BorderColor,
	BorderTop,
	BorderRight,
	BorderBottom,
	BorderLeft,
	Border,
	BorderRadius,
	Overflow,
	Background,
	Font,
	Gap,
	PerspectiveOrigin,
	TransformOrigin,
	Flex,
	FlexFlow,
	Nav,

	NumDefinedIds,
	FirstCustomId = NumDefinedIds,

	// The maximum number of IDs. This limits the number of possible custom IDs to MaxNumIds - FirstCustomId.
	MaxNumIds = 0xff
};

enum class PropertyId : uint8_t {
	Invalid,

	/*
	  The following values define the property ids for the main stylesheet specification.
	  These values must not be used in places that have their own property specification,
	  such as decorators and font-effects.
	*/
	MarginTop,
	MarginRight,
	MarginBottom,
	MarginLeft,
	PaddingTop,
	PaddingRight,
	PaddingBottom,
	PaddingLeft,
	BorderTopWidth,
	BorderRightWidth,
	BorderBottomWidth,
	BorderLeftWidth,
	BorderTopColor,
	BorderRightColor,
	BorderBottomColor,
	BorderLeftColor,
	BorderTopLeftRadius,
	BorderTopRightRadius,
	BorderBottomRightRadius,
	BorderBottomLeftRadius,
	Display,
	Position,
	Top,
	Right,
	Bottom,
	Left,
	Float,
	Clear,
	BoxSizing,
	ZIndex,
	Width,
	MinWidth,
	MaxWidth,
	Height,
	MinHeight,
	MaxHeight,
	LineHeight,
	VerticalAlign,
	OverflowX,
	OverflowY,
	Clip,
	Visibility,
	BackgroundColor,
	Color,
	CaretColor,
	ImageColor,
	FontFamily,
	FontStyle,
	FontWeight,
	FontSize,
	LetterSpacing,
	TextAlign,
	TextDecoration,
	TextTransform,
	WhiteSpace,
	WordBreak,
	RowGap,
	ColumnGap,
	Cursor,
	Drag,
	TabIndex,
	ScrollbarMargin,
	OverscrollBehavior,

	Perspective,
	PerspectiveOriginX,
	PerspectiveOriginY,
	Transform,
	TransformOriginX,
	TransformOriginY,
	TransformOriginZ,

	Transition,
	Animation,

	Opacity,
	PointerEvents,
	Focus,

	Decorator,
	MaskImage,
	FontEffect,

	Filter,
	BackdropFilter,
	BoxShadow,

	FillImage,

	AlignContent,
	AlignItems,
	AlignSelf,
	FlexBasis,
	FlexDirection,
	FlexGrow,
	FlexShrink,
	FlexWrap,
	JustifyContent,

	NavUp,
	NavRight,
	NavDown,
	NavLeft,

	RmlUi_Language,
	RmlUi_Direction,

	NumDefinedIds,
	FirstCustomId = NumDefinedIds,

	// The maximum number of IDs. This limits the number of possible custom IDs to MaxNumIds - FirstCustomId.
	MaxNumIds = 128
};

enum class MediaQueryId : uint8_t {
	Invalid,

	Width,
	MinWidth,
	MaxWidth,
	Height,
	MinHeight,
	MaxHeight,
	AspectRatio,
	MinAspectRatio,
	MaxAspectRatio,
	Resolution,
	MinResolution,
	MaxResolution,
	Orientation,
	Theme,

	NumDefinedIds
};

enum class EventId : uint16_t {
	Invalid,

	// Core events
	Mousedown,
	Mousescroll,
	Mouseover,
	Mouseout,
	Focus,
	Blur,
	Keydown,
	Keyup,
	Textinput,
	Mouseup,
	Click,
	Dblclick,
	Load,
	Unload,
	Show,
	Hide,
	Mousemove,
	Dragmove,
	Drag,
	Dragstart,
	Dragover,
	Dragdrop,
	Dragout,
	Dragend,
	Handledrag,
	Resize,
	Scroll,
	Animationend,
	Transitionend,

	// Form control events
	Change,
	Submit,
	Tabchange,

	NumDefinedIds,

	// Custom IDs start here
	FirstCustomId = NumDefinedIds,

	// The maximum number of IDs. This limits the number of possible custom IDs to MaxNumIds - FirstCustomId.
	MaxNumIds = 0xffff
};

} // namespace Rml
#endif
