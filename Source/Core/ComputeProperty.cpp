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

#include "../../Include/RmlUi/Core/Property.h"
#include "ComputeProperty.h"

namespace Rml {

const Style::ComputedValues DefaultComputedValues = Style::ComputedValues{};

static constexpr float PixelsPerInch = 96.0f;



float ResolveValue(Style::LengthPercentageAuto length, float base_value)
{
	if (length.type == Style::LengthPercentageAuto::Length)
		return length.value;
	else if (length.type == Style::LengthPercentageAuto::Percentage)
		return length.value * 0.01f * base_value;
	return 0.0f;
}

float ResolveValue(Style::LengthPercentage length, float base_value)
{
	if (length.type == Style::LengthPercentage::Length)
		return length.value;
	else if (length.type == Style::LengthPercentage::Percentage)
		return length.value * 0.01f * base_value;
	return 0.0f;
}


float ComputeLength(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	RMLUI_ASSERT(property);
	
	float value = property->value.Get<float>();

	switch (property->unit)
	{
	case Property::NUMBER:
	case Property::PX:
	case Property::RAD:
		return value;

	case Property::EM:
		return value * font_size;
	case Property::REM:
		return value * document_font_size;
	case Property::DP:
		return value * dp_ratio;
	case Property::VW:
		return value * vp_dimensions.x * 0.01f;
	case Property::VH:
		return value * vp_dimensions.y * 0.01f;

	case Property::DEG:
		return Math::DegreesToRadians(value);
	default: 
		break;
	}

	// Values based on pixels-per-inch.
	if (property->unit & Property::PPI_UNIT)
	{
		float inch = value * PixelsPerInch;

		switch (property->unit)
		{
		case Property::INCH: // inch
			return inch;
		case Property::CM: // centimeter
			return inch * (1.0f / 2.54f);
		case Property::MM: // millimeter
			return inch * (1.0f / 25.4f);
		case Property::PT: // point
			return inch * (1.0f / 72.0f);
		case Property::PC: // pica
			return inch * (1.0f / 6.0f);
		default:
			break;
		}
	}

	// We're not a numeric property; return 0.
	return 0.0f;
}

float ComputeAbsoluteLength(const Property& property, float dp_ratio, Vector2f vp_dimensions)
{
	RMLUI_ASSERT(property.unit & Property::ABSOLUTE_LENGTH);

	switch (property.unit)
	{
	case Property::PX:
		return property.value.Get< float >();
	case Property::DP:
		return property.value.Get< float >() * dp_ratio;
	case Property::VW:
		return property.value.Get< float >() * vp_dimensions.x * 0.01f;
	case Property::VH:
		return property.value.Get< float >() * vp_dimensions.y * 0.01f;
	default:
		// Values based on pixels-per-inch.
		if (property.unit & Property::PPI_UNIT)
		{
			float inch = property.value.Get< float >() * PixelsPerInch;

			switch (property.unit)
			{
			case Property::INCH: // inch
				return inch;
			case Property::CM: // centimeter
				return inch * (1.0f / 2.54f);
			case Property::MM: // millimeter
				return inch * (1.0f / 25.4f);
			case Property::PT: // point
				return inch * (1.0f / 72.0f);
			case Property::PC: // pica
				return inch * (1.0f / 6.0f);
			default:
				break;
			}
		}
	}

	RMLUI_ERROR;
	return 0.0f;
}

float ComputeAngle(const Property& property)
{
	float value = property.value.Get<float>();

	switch (property.unit)
	{
	case Property::NUMBER:
	case Property::RAD:
		return value;

	case Property::DEG:
		return Math::DegreesToRadians(value);
	default:
		break;
	}

	return 0.0f;
}

float ComputeFontsize(const Property& property, const Style::ComputedValues& values, const Style::ComputedValues* parent_values, const Style::ComputedValues* document_values, float dp_ratio, Vector2f vp_dimensions)
{
	// The calculated value of the font-size property is inherited, so we need to check if this
	// is an inherited property. If so, then we return our parent's font size instead.
	if (property.unit & Property::RELATIVE_UNIT)
	{
		float multiplier = 1.0f;

		switch (property.unit)
		{
		case Property::PERCENT:
			multiplier = 0.01f;
			//-fallthrough
		case Property::EM:
			if (!parent_values)
				return 0;
			return property.value.Get< float >() * multiplier * parent_values->font_size;

		case Property::REM:
			if (!document_values)
				return 0;
			// If the current element is a document, the rem unit is relative to the default size
			if(&values == document_values)
				return property.value.Get< float >() * DefaultComputedValues.font_size;
			// Otherwise it is relative to the document font size
			return property.value.Get< float >() * document_values->font_size;
		default:
			RMLUI_ERRORMSG("A relative unit must be percentage, em or rem.");
		}
	}

	return ComputeAbsoluteLength(property, dp_ratio, vp_dimensions);
}

Style::Clip ComputeClip(const Property* property)
{
	int value = property->Get<int>();
	if (property->unit == Property::KEYWORD)
		return Style::Clip(static_cast<Style::Clip::Type>(value));
	else if (property->unit == Property::NUMBER)
		return Style::Clip(Style::Clip::Type::Number, value);
	RMLUI_ERRORMSG("Invalid clip type");
	return Style::Clip();
}

Style::LineHeight ComputeLineHeight(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	if (property->unit & Property::LENGTH)
	{
		float value = ComputeLength(property, font_size, document_font_size, dp_ratio, vp_dimensions);
		return Style::LineHeight(value, Style::LineHeight::Length, value);
	}

	float scale_factor = 1.0f;

	switch (property->unit)
	{
	case Property::NUMBER:
		scale_factor = property->value.Get< float >();
		break;
	case Property::PERCENT:
		scale_factor = property->value.Get< float >() * 0.01f;
		break;
	default:
		RMLUI_ERRORMSG("Invalid unit for line-height");
	}

	float value = font_size * scale_factor;
	return Style::LineHeight(value, Style::LineHeight::Number, scale_factor);
}

Style::VerticalAlign ComputeVerticalAlign(const Property* property, float line_height, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	if (property->unit & Property::LENGTH)
	{
		float value = ComputeLength(property, font_size, document_font_size, dp_ratio, vp_dimensions);
		return Style::VerticalAlign(value);
	}
	else if (property->unit & Property::PERCENT)
	{
		return Style::VerticalAlign(property->Get<float>() * line_height);
	}

	RMLUI_ASSERT(property->unit & Property::KEYWORD);
	return Style::VerticalAlign((Style::VerticalAlign::Type)property->Get<int>());
}

Style::LengthPercentage ComputeLengthPercentage(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	using namespace Style;
	if (property->unit & Property::PERCENT)
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	return LengthPercentage(LengthPercentage::Length, ComputeLength(property, font_size, document_font_size, dp_ratio, vp_dimensions));
}


Style::LengthPercentageAuto ComputeLengthPercentageAuto(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	using namespace Style;
	// Assuming here that 'auto' is the only possible keyword
	if (property->unit & Property::PERCENT)
		return LengthPercentageAuto(LengthPercentageAuto::Percentage, property->Get<float>());
	else if (property->unit & Property::KEYWORD)
		return LengthPercentageAuto(LengthPercentageAuto::Auto);

	return LengthPercentageAuto(LengthPercentageAuto::Length, ComputeLength(property, font_size, document_font_size, dp_ratio, vp_dimensions));
}

Style::LengthPercentage ComputeOrigin(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	using namespace Style;
	static_assert((int)OriginX::Left == (int)OriginY::Top && (int)OriginX::Center == (int)OriginY::Center && (int)OriginX::Right == (int)OriginY::Bottom, "");

	if (property->unit & Property::KEYWORD)
	{
		float percent = 0.0f;
		OriginX origin = (OriginX)property->Get<int>();
		switch (origin)
		{
		case OriginX::Left: percent = 0.0f; break;
		case OriginX::Center: percent = 50.0f; break;
		case OriginX::Right: percent = 100.f; break;
		}
		return LengthPercentage(LengthPercentage::Percentage, percent);
	}
	else if (property->unit & Property::PERCENT)
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	return LengthPercentage(LengthPercentage::Length, ComputeLength(property, font_size, document_font_size, dp_ratio, vp_dimensions));
}


} // namespace Rml
