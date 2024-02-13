/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
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

#ifndef RMLUI_CORE_STYLETYPES_H
#define RMLUI_CORE_STYLETYPES_H

#include "Types.h"

namespace Rml {
namespace Style {

	struct LengthPercentageAuto {
		enum Type : uint8_t { Auto, Length, Percentage } type = Length;
		float value = 0;
		LengthPercentageAuto() {}
		LengthPercentageAuto(Type type, float value = 0) : type(type), value(value) {}
	};
	struct LengthPercentage {
		enum Type : uint8_t { Length, Percentage } type = Length;
		float value = 0;
		LengthPercentage() {}
		LengthPercentage(Type type, float value = 0) : type(type), value(value) {}
	};

	struct NumberAuto {
		enum Type : uint8_t { Auto, Number } type = Number;
		float value = 0;
		NumberAuto() {}
		NumberAuto(Type type, float value = 0) : type(type), value(value) {}
	};

	using Margin = LengthPercentageAuto;
	using Padding = LengthPercentage;

	enum class Display : uint8_t {
		None,
		Block,
		Inline,
		InlineBlock,
		FlowRoot,
		Flex,
		InlineFlex,
		Table,
		InlineTable,
		TableRow,
		TableRowGroup,
		TableColumn,
		TableColumnGroup,
		TableCell
	};
	enum class Position : uint8_t { Static, Relative, Absolute, Fixed };

	using Top = LengthPercentageAuto;
	using Right = LengthPercentageAuto;
	using Bottom = LengthPercentageAuto;
	using Left = LengthPercentageAuto;

	enum class Float : uint8_t { None, Left, Right };
	enum class Clear : uint8_t { None, Left, Right, Both };

	enum class BoxSizing : uint8_t { ContentBox, BorderBox };

	using ZIndex = NumberAuto;

	using Width = LengthPercentageAuto;
	using MinWidth = LengthPercentage;
	using MaxWidth = LengthPercentage; // 'none' keyword converted to FLT_MAX length.

	using Height = LengthPercentageAuto;
	using MinHeight = LengthPercentage;
	using MaxHeight = LengthPercentage; // 'none' keyword converted to FLT_MAX length.

	struct LineHeight {
		float value = 12.f * 1.2f; // The computed value (length)
		enum InheritType : uint8_t { Number, Length } inherit_type = Number;
		float inherit_value = 1.2f;
		LineHeight() {}
		LineHeight(float value, InheritType inherit_type, float inherit_value) :
			value(value), inherit_type(inherit_type), inherit_value(inherit_value)
		{}
	};
	struct VerticalAlign {
		enum Type : uint8_t { Baseline, Middle, Sub, Super, TextTop, TextBottom, Top, Center, Bottom, Length } type;
		float value; // For length type
		VerticalAlign(Type type = Baseline) : type(type), value(0) {}
		VerticalAlign(float value) : type(Length), value(value) {}
		VerticalAlign(Type type, float value) : type(type), value(value) {}
	};

	enum class Overflow : uint8_t { Visible, Hidden, Auto, Scroll };
	struct Clip {
	private:
		// Here, 'value' is encoded with Auto <=> 0, None <=> -1, Always <=> -2. A value > 0 means the given number.
		int8_t value = 0;

	public:
		// The Type enum must instead correspond to the keywords in StyleSheetSpec.
		enum class Type : uint8_t { Auto, None, Always, Number };
		Clip() {}
		Clip(Type type, int8_t number = 0) : value(type == Type::Auto ? 0 : (type == Type::None ? -1 : (type == Type::Always ? -2 : number))) {}
		int GetNumber() const { return value < 0 ? 0 : value; }
		Type GetType() const { return value == 0 ? Type::Auto : (value == -1 ? Type::None : (value == -2 ? Type::Always : Type::Number)); }
		bool operator==(Type type) const { return GetType() == type; }
	};

	enum class Visibility : uint8_t { Visible, Hidden };

	enum class FontStyle : uint8_t { Normal, Italic };
	enum class FontWeight : uint16_t { Auto = 0, Normal = 400, Bold = 700 }; // Any definite value in the range [1,1000] is valid.

	enum class TextAlign : uint8_t { Left, Right, Center, Justify };
	enum class TextDecoration : uint8_t { None, Underline, Overline, LineThrough };
	enum class TextTransform : uint8_t { None, Capitalize, Uppercase, Lowercase };
	enum class WhiteSpace : uint8_t { Normal, Pre, Nowrap, Prewrap, Preline };
	enum class WordBreak : uint8_t { Normal, BreakAll, BreakWord };

	enum class Drag : uint8_t { None, Drag, DragDrop, Block, Clone };
	enum class TabIndex : uint8_t { None, Auto };
	enum class Focus : uint8_t { None, Auto };
	enum class OverscrollBehavior : uint8_t { Auto, Contain };
	enum class PointerEvents : uint8_t { None, Auto };

	using PerspectiveOrigin = LengthPercentage;
	using TransformOrigin = LengthPercentage;

	enum class OriginX : uint8_t { Left, Center, Right };
	enum class OriginY : uint8_t { Top, Center, Bottom };

	enum class AlignContent : uint8_t { FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly, Stretch };
	enum class AlignItems : uint8_t { FlexStart, FlexEnd, Center, Baseline, Stretch };
	enum class AlignSelf : uint8_t { Auto, FlexStart, FlexEnd, Center, Baseline, Stretch };
	using FlexBasis = LengthPercentageAuto;
	enum class FlexDirection : uint8_t { Row, RowReverse, Column, ColumnReverse };
	enum class FlexWrap : uint8_t { Nowrap, Wrap, WrapReverse };
	enum class JustifyContent : uint8_t { FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly };

	enum class Nav : uint8_t { None, Auto, Horizontal, Vertical };

	enum class Direction : uint8_t { Auto, Ltr, Rtl };

	class ComputedValues;

} // namespace Style

using ComputedValues = Style::ComputedValues;

} // namespace Rml
#endif
