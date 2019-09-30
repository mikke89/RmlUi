/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
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

#ifndef RMLUICORECOMPUTEDVALUES_H
#define RMLUICORECOMPUTEDVALUES_H

#include "Types.h"
#include "Animation.h"

namespace Rml {
namespace Core {

namespace Style
{

struct LengthPercentageAuto {
	enum Type { Auto, Length, Percentage } type = Length;
	float value = 0;
	LengthPercentageAuto() {}
	LengthPercentageAuto(Type type, float value = 0) : type(type), value(value) {}
};
struct LengthPercentage {
	enum Type { Length, Percentage } type = Length;
	float value = 0;
	LengthPercentage() {}
	LengthPercentage(Type type, float value = 0) : type(type), value(value) {}
};

struct NumberAuto {
	enum Type { Auto, Number } type = Number;
	float value = 0;
	NumberAuto() {}
	NumberAuto(Type type, float value = 0) : type(type), value(value) {}
};


using Margin = LengthPercentageAuto;
using Padding = LengthPercentage;

enum class Display : uint8_t { None, Block, Inline, InlineBlock };
enum class Position : uint8_t { Static, Relative, Absolute, Fixed };

using Top = LengthPercentageAuto;
using Right = LengthPercentageAuto;
using Bottom = LengthPercentageAuto;
using Left = LengthPercentageAuto;

enum class Float : uint8_t { None, Left, Right };
enum class Clear : uint8_t { None, Left, Right, Both };

using ZIndex = NumberAuto;

using Width = LengthPercentageAuto;
using MinWidth = LengthPercentage;
using MaxWidth = LengthPercentage;

using Height = LengthPercentageAuto;
using MinHeight = LengthPercentage;
using MaxHeight = LengthPercentage;

struct LineHeight {
	float value = 12.f * 1.2f; // The computed value (length)
	enum InheritType { Number, Length } inherit_type = Number;
	float inherit_value = 1.2f;
	LineHeight() {}
	LineHeight(float value, InheritType inherit_type, float inherit_value) : value(value), inherit_type(inherit_type), inherit_value(inherit_value) {}
};
struct VerticalAlign {
	enum Type { Baseline, Middle, Sub, Super, TextTop, TextBottom, Top, Bottom, Length } type;
	float value; // For length type
	VerticalAlign(Type type = Baseline) : type(type), value(0) {}
	VerticalAlign(float value) : type(Length), value(value) {}
};

enum class Overflow : uint8_t { Visible, Hidden, Auto, Scroll };
struct Clip {
	enum class Type : uint8_t { Auto, None, Number };
	// Note, internally 'number' is encoded with Auto as 0 and None as -1. However, the enum must correspond to the keywords in StyleSheetSpec.
	int number = 0;
	Clip() {}
	Clip(Type type, int number = 0) : number(type == Type::Auto ? 0 : (type == Type::None ? -1 : number)) {}
};

enum class Visibility : uint8_t { Visible, Hidden };

enum class FontStyle : uint8_t { Normal, Italic };
enum class FontWeight : uint8_t { Normal, Bold };

enum class TextAlign : uint8_t { Left, Right, Center, Justify };
enum class TextDecoration : uint8_t { None, Underline, Overline, LineThrough };
enum class TextTransform : uint8_t { None, Capitalize, Uppercase, Lowercase };
enum class WhiteSpace : uint8_t { Normal, Pre, Nowrap, Prewrap, Preline };

enum class Drag : uint8_t { None, Drag, DragDrop, Block, Clone };
enum class TabIndex : uint8_t { None, Auto };
enum class Focus : uint8_t { None, Auto };
enum class PointerEvents : uint8_t { None, Auto };

using PerspectiveOrigin = LengthPercentage;
using TransformOrigin = LengthPercentage;

enum class OriginX : uint8_t { Left, Center, Right };
enum class OriginY : uint8_t { Top, Center, Bottom };


/* 
	A computed value is a value resolved as far as possible :before: introducing layouting. See CSS specs for details of each property.

	Note: Enums and default values must correspond to the keywords and defaults in `StyleSheetSpecification.cpp`.
*/

struct ComputedValues
{
	Margin margin_top, margin_right, margin_bottom, margin_left;
	Padding padding_top, padding_right, padding_bottom, padding_left;
	float border_top_width = 0, border_right_width = 0, border_bottom_width = 0, border_left_width = 0;
	Colourb border_top_color{ 255, 255, 255 }, border_right_color{ 255, 255, 255 }, border_bottom_color{ 255, 255, 255 }, border_left_color{ 255, 255, 255 };

	Display display = Display::Inline;
	Position position = Position::Static;

	Top top{ Top::Auto };
	Right right{ Right::Auto };
	Bottom bottom{ Bottom::Auto };
	Left left{ Left::Auto };

	Float float_ = Float::None;
	Clear clear = Clear::None;

	ZIndex z_index = { ZIndex::Auto };

	Width width = { Width::Auto };
	MinWidth min_width;
	MaxWidth max_width{ MaxWidth::Length, -1.f };
	Height height = { Height::Auto };
	MinHeight min_height;
	MaxHeight max_height{ MaxHeight::Length, -1.f };

	LineHeight line_height;
	VerticalAlign vertical_align;

	Overflow overflow_x = Overflow::Visible, overflow_y = Overflow::Visible;
	Clip clip;

	Visibility visibility = Visibility::Visible;

	Colourb background_color = Colourb(255, 255, 255, 0);
	Colourb color = Colourb(255, 255, 255);
	Colourb image_color = Colourb(255, 255, 255);
	float opacity = 1;

	String font_family;
	FontStyle font_style = FontStyle::Normal;
	FontWeight font_weight = FontWeight::Normal;
	float font_size = 12.f;
	// Font face used to render text and resolve ex properties. Does not represent a true property
	// like most computed values, but placed here as it is used and inherited in a similar manner.
	FontFaceHandle font_face_handle = 0;

	TextAlign text_align = TextAlign::Left;
	TextDecoration text_decoration = TextDecoration::None;
	TextTransform text_transform = TextTransform::None;
	WhiteSpace white_space = WhiteSpace::Normal;

	String cursor;

	Drag drag = Drag::None;
	TabIndex tab_index = TabIndex::None;
	Focus focus = Focus::Auto;
	float scrollbar_margin = 0;
	PointerEvents pointer_events = PointerEvents::Auto;

	float perspective = 0;
	PerspectiveOrigin perspective_origin_x = { PerspectiveOrigin::Percentage, 50.f };
	PerspectiveOrigin perspective_origin_y = { PerspectiveOrigin::Percentage, 50.f };

	TransformPtr transform;
	TransformOrigin transform_origin_x = { TransformOrigin::Percentage, 50.f };
	TransformOrigin transform_origin_y = { TransformOrigin::Percentage, 50.f };
	float transform_origin_z = 0.0f;

	TransitionList transition;
	AnimationList animation;

	DecoratorsPtr decorator;
	FontEffectsPtr font_effect; // Sorted by layer first (back then front), then by declaration order.
};
}


// Resolves a computed LengthPercentage value to the base unit 'px'. 
// Percentages are scaled by the base_value.
// Note: Auto must be manually handled during layout, here it returns zero.
RMLUICORE_API float ResolveValue(Style::LengthPercentageAuto length, float base_value);
RMLUICORE_API float ResolveValue(Style::LengthPercentage length, float base_value);


using ComputedValues = Style::ComputedValues;

}
}

#endif
