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
#include "ElementStyle.h"
#include "ElementStyleCache.h"

namespace Rocket {
namespace Core {

ElementStyleCache::ElementStyleCache(ElementStyle *style) : style(style), 
	top(NULL), bottom(NULL), left(NULL), right(NULL),
	border_top_width(NULL), border_bottom_width(NULL), border_left_width(NULL), border_right_width(NULL),
	margin_top(NULL), margin_bottom(NULL), margin_left(NULL), margin_right(NULL),
	padding_top(NULL), padding_bottom(NULL), padding_left(NULL), padding_right(NULL),
	width(NULL), height(NULL),
	local_width(NULL), local_height(NULL), have_local_width(false), have_local_height(false),
	overflow_x(-1), overflow_y(-1),
	position(-1), float_(-1), display(-1), whitespace(-1),
	line_height(NULL), text_align(-1), text_transform(-1), vertical_align(NULL),
	pointer_events(-1)
{
}

void ElementStyleCache::Clear()
{
	ClearOffset();
	ClearBorder();
	ClearMargin();
	ClearPadding();
	ClearDimensions();
	ClearOverflow();
	ClearPosition();
	ClearFloat();
	ClearDisplay();
	ClearWhitespace();
}

void ElementStyleCache::ClearInherited()
{
	ClearLineHeight();
	ClearTextAlign();
	ClearTextTransform();
	ClearVerticalAlign();
	ClearPointerEvents();
}

void ElementStyleCache::ClearOffset()
{
	top = bottom = left = right = NULL;
}

void ElementStyleCache::ClearBorder()
{
	border_top_width = border_bottom_width = border_left_width = border_right_width = NULL;
}

void ElementStyleCache::ClearMargin()
{
	margin_top = margin_bottom = margin_left = margin_right = NULL;
}

void ElementStyleCache::ClearPadding()
{
	padding_top = padding_bottom = padding_left = padding_right = NULL;
}

void ElementStyleCache::ClearDimensions()
{
	width = height = NULL;
	have_local_width = have_local_height = false;
}

void ElementStyleCache::ClearOverflow()
{
	overflow_x = overflow_y = -1;
}

void ElementStyleCache::ClearPosition()
{
	position = -1;
}

void ElementStyleCache::ClearFloat()
{
	float_ = -1;
}

void ElementStyleCache::ClearDisplay()
{
	display = -1;
}

void ElementStyleCache::ClearWhitespace()
{
	whitespace = -1;
}

void ElementStyleCache::ClearLineHeight()
{
	line_height = NULL;
}

void ElementStyleCache::ClearTextAlign()
{
	text_align = -1;
}

void ElementStyleCache::ClearTextTransform()
{
	text_transform = -1;
}

void ElementStyleCache::ClearVerticalAlign()
{
	vertical_align = NULL;
}

void ElementStyleCache::ClearPointerEvents()
{
	pointer_events = -1;
}

void ElementStyleCache::GetOffsetProperties(const Property **o_top, const Property **o_bottom, const Property **o_left, const Property **o_right )
{
	if (o_top)
	{
		if (!top)
			top = style->GetProperty(PropertyId::Top);
		*o_top = top;
	}

	if (o_bottom)
	{
		if (!bottom)
			bottom = style->GetProperty(PropertyId::Bottom);
		*o_bottom = bottom;
	}

	if (o_left)
	{
		if (!left)
			left = style->GetProperty(PropertyId::Left);
		*o_left = left;
	}

	if (o_right)
	{
		if (!right)
			right = style->GetProperty(PropertyId::Right);
		*o_right = right;
	}
}

void ElementStyleCache::GetBorderWidthProperties(const Property **o_border_top_width, const Property **o_border_bottom_width, const Property **o_border_left_width, const Property **o_border_right_width)
{
	if (o_border_top_width)
	{
		if (!border_top_width)
			border_top_width = style->GetProperty(PropertyId::BorderTopWidth);
		*o_border_top_width = border_top_width;
	}

	if (o_border_bottom_width)
	{
		if (!border_bottom_width)
			border_bottom_width = style->GetProperty(PropertyId::BorderBottomWidth);
		*o_border_bottom_width = border_bottom_width;
	}

	if (o_border_left_width)
	{
		if (!border_left_width)
			border_left_width = style->GetProperty(PropertyId::BorderLeftWidth);
		*o_border_left_width = border_left_width;
	}

	if (o_border_right_width)
	{
		if (!border_right_width)
			border_right_width = style->GetProperty(PropertyId::BorderRightWidth);
		*o_border_right_width = border_right_width;
	}
}

void ElementStyleCache::GetMarginProperties(const Property **o_margin_top, const Property **o_margin_bottom, const Property **o_margin_left, const Property **o_margin_right)
{
	if (o_margin_top)
	{
		if (!margin_top)
			margin_top = style->GetProperty(PropertyId::MarginTop);
		*o_margin_top = margin_top;
	}

	if (o_margin_bottom)
	{
		if (!margin_bottom)
			margin_bottom = style->GetProperty(PropertyId::MarginBottom);
		*o_margin_bottom = margin_bottom;
	}

	if (o_margin_left)
	{
		if (!margin_left)
			margin_left = style->GetProperty(PropertyId::MarginLeft);
		*o_margin_left = margin_left;
	}

	if (o_margin_right)
	{
		if (!margin_right)
			margin_right = style->GetProperty(PropertyId::MarginRight);
		*o_margin_right = margin_right;
	}
}

void ElementStyleCache::GetPaddingProperties(const Property **o_padding_top, const Property **o_padding_bottom, const Property **o_padding_left, const Property **o_padding_right)
{
	if (o_padding_top)
	{
		if (!padding_top)
			padding_top = style->GetProperty(PropertyId::PaddingTop);
		*o_padding_top = padding_top;
	}

	if (o_padding_bottom)
	{
		if (!padding_bottom)
			padding_bottom = style->GetProperty(PropertyId::PaddingBottom);
		*o_padding_bottom = padding_bottom;
	}

	if (o_padding_left)
	{
		if (!padding_left)
			padding_left = style->GetProperty(PropertyId::PaddingLeft);
		*o_padding_left = padding_left;
	}

	if (o_padding_right)
	{
		if (!padding_right)
			padding_right = style->GetProperty(PropertyId::PaddingRight);
		*o_padding_right = padding_right;
	}
}

void ElementStyleCache::GetDimensionProperties(const Property **o_width, const Property **o_height)
{
	if (o_width)
	{
		if (!width)
			width = style->GetProperty(PropertyId::Width);
		*o_width = width;
	}

	if (o_height)
	{
		if (!height)
			height = style->GetProperty(PropertyId::Height);
		*o_height = height;
	}
}

void ElementStyleCache::GetLocalDimensionProperties(const Property **o_width, const Property **o_height)
{
	if (o_width)
	{
		if (!have_local_width)
		{
			have_local_width = true;
			local_width = style->GetLocalProperty(PropertyId::Width);
		}
		*o_width = local_width;
	}

	if (o_height)
	{
		if (!have_local_height)
		{
			have_local_height = true;
			local_height = style->GetLocalProperty(PropertyId::Height);
		}
		*o_height = local_height;
	}
}

void ElementStyleCache::GetOverflow(int *o_overflow_x, int *o_overflow_y)
{
	if (o_overflow_x)
	{
		if (overflow_x < 0)
			overflow_x = style->GetProperty(PropertyId::OverflowX)->Get< int >();
		*o_overflow_x = overflow_x;
	}

	if (o_overflow_y)
	{
		if (overflow_y < 0)
			overflow_y = style->GetProperty(PropertyId::OverflowY)->Get< int >();
		*o_overflow_y = overflow_y;
	}
}

int ElementStyleCache::GetPosition()
{
	if (position < 0)
		position = style->GetProperty(PropertyId::Position)->Get< int >();
	return position;
}

int ElementStyleCache::GetFloat()
{
	if (float_ < 0)
		float_ = style->GetProperty(PropertyId::Float)->Get< int >();
	return float_;
}

int ElementStyleCache::GetDisplay()
{
	if (display < 0)
		display = style->GetProperty(PropertyId::Display)->Get< int >();
	return display;
}

int ElementStyleCache::GetWhitespace()
{
	if (whitespace < 0)
		whitespace = style->GetProperty(PropertyId::WhiteSpace)->Get< int >();
	return whitespace;
}

int ElementStyleCache::GetPointerEvents()
{
	if (pointer_events < 0)
		pointer_events = style->GetProperty(PropertyId::PointerEvents)->Get< int >();
	return pointer_events;
}

const Property *ElementStyleCache::GetLineHeightProperty()
{
	if (!line_height)
		line_height = style->GetProperty(PropertyId::LineHeight);
	return line_height;
}

int ElementStyleCache::GetTextAlign()
{
	if (text_align < 0)
		text_align = style->GetProperty(PropertyId::TextAlign)->Get< int >();
	return text_align;
}

int ElementStyleCache::GetTextTransform()
{
	if (text_transform < 0)
		text_transform = style->GetProperty(PropertyId::TextTransform)->Get< int >();
	return text_transform;
}

const Property *ElementStyleCache::GetVerticalAlignProperty()
{
	if (!vertical_align)
		vertical_align = style->GetProperty(PropertyId::VerticalAlign);
	return vertical_align;
}

}
}
