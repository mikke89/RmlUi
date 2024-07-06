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

#include "ComputeProperty.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"

namespace Rml {

const Style::ComputedValues DefaultComputedValues{nullptr};

static constexpr float PixelsPerInch = 96.0f;

static float ComputePPILength(NumericValue value, float dp_ratio)
{
	RMLUI_ASSERT(Any(value.unit & Unit::PPI_UNIT));

	// Values based on pixels-per-inch. Scaled by the dp-ratio as a placeholder solution until we make the pixel unit itself scalable.
	const float inch = value.number * PixelsPerInch * dp_ratio;

	switch (value.unit)
	{
	case Unit::INCH: return inch;
	case Unit::CM: return inch * (1.0f / 2.54f);
	case Unit::MM: return inch * (1.0f / 25.4f);
	case Unit::PT: return inch * (1.0f / 72.0f);
	case Unit::PC: return inch * (1.0f / 6.0f);
	default: break;
	}

	RMLUI_ERROR;
	return 0.f;
}

float ComputeLength(NumericValue value, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	if (Any(value.unit & Unit::PPI_UNIT))
		return ComputePPILength(value, dp_ratio);

	switch (value.unit)
	{
	case Unit::PX: return value.number;
	case Unit::EM: return value.number * font_size;
	case Unit::REM: return value.number * document_font_size;
	case Unit::DP: return value.number * dp_ratio;
	case Unit::VW: return value.number * vp_dimensions.x * 0.01f;
	case Unit::VH: return value.number * vp_dimensions.y * 0.01f;
	default: break;
	}

	RMLUI_ERROR;
	return 0.0f;
}

float ComputeAngle(NumericValue value)
{
	switch (value.unit)
	{
	case Unit::NUMBER:
	case Unit::RAD: return value.number;

	case Unit::DEG: return Math::DegreesToRadians(value.number);
	default: break;
	}

	RMLUI_ERROR;
	return 0.0f;
}

float ComputeFontsize(NumericValue value, const Style::ComputedValues& values, const Style::ComputedValues* parent_values,
	const Style::ComputedValues* document_values, float dp_ratio, Vector2f vp_dimensions)
{
	if (Any(value.unit & (Unit::PERCENT | Unit::EM | Unit::REM)))
	{
		// Relative values are based on the parent's or document's font size instead of our own.
		float multiplier = 1.0f;

		switch (value.unit)
		{
		case Unit::PERCENT:
			multiplier = 0.01f;
			//-fallthrough
		case Unit::EM:
			if (!parent_values)
				return 0;
			return value.number * multiplier * parent_values->font_size();

		case Unit::REM:
			// If the current element is a document, the rem unit is relative to the default size.
			if (!document_values || &values == document_values)
				return value.number * DefaultComputedValues.font_size();

			// Otherwise it is relative to the document font size.
			return value.number * document_values->font_size();
		default: break;
		}
	}

	// Font-relative lengths handled above, other lengths should be handled as normal.
	return ComputeLength(value, 0.f, 0.f, dp_ratio, vp_dimensions);
}

String ComputeFontFamily(String font_family)
{
	return StringUtilities::ToLower(std::move(font_family));
}

Style::Clip ComputeClip(const Property* property)
{
	const int value = property->Get<int>();
	if (property->unit == Unit::KEYWORD)
		return Style::Clip(static_cast<Style::Clip::Type>(value));
	else if (property->unit == Unit::NUMBER)
		return Style::Clip(Style::Clip::Type::Number, static_cast<int8_t>(value));
	RMLUI_ERRORMSG("Invalid clip type");
	return Style::Clip();
}

Style::LineHeight ComputeLineHeight(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	if (Any(property->unit & Unit::LENGTH))
	{
		float value = ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions);
		return Style::LineHeight(value, Style::LineHeight::Length, value);
	}

	float scale_factor = 1.0f;

	switch (property->unit)
	{
	case Unit::NUMBER: scale_factor = property->value.Get<float>(); break;
	case Unit::PERCENT: scale_factor = property->value.Get<float>() * 0.01f; break;
	default: RMLUI_ERRORMSG("Invalid unit for line-height");
	}

	float value = font_size * scale_factor;
	return Style::LineHeight(value, Style::LineHeight::Number, scale_factor);
}

Style::VerticalAlign ComputeVerticalAlign(const Property* property, float line_height, float font_size, float document_font_size, float dp_ratio,
	Vector2f vp_dimensions)
{
	if (Any(property->unit & Unit::LENGTH))
	{
		float value = ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions);
		return Style::VerticalAlign(value);
	}
	else if (property->unit == Unit::PERCENT)
	{
		return Style::VerticalAlign(property->Get<float>() * line_height * 0.01f);
	}

	RMLUI_ASSERT(property->unit == Unit::KEYWORD);
	return Style::VerticalAlign((Style::VerticalAlign::Type)property->Get<int>());
}

Style::LengthPercentage ComputeLengthPercentage(const Property* property, float font_size, float document_font_size, float dp_ratio,
	Vector2f vp_dimensions)
{
	using namespace Style;
	if (property->unit == Unit::PERCENT)
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	return LengthPercentage(LengthPercentage::Length,
		ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
}

Style::LengthPercentageAuto ComputeLengthPercentageAuto(const Property* property, float font_size, float document_font_size, float dp_ratio,
	Vector2f vp_dimensions)
{
	using namespace Style;
	if (property->unit == Unit::PERCENT)
		return LengthPercentageAuto(LengthPercentageAuto::Percentage, property->Get<float>());
	else if (property->unit == Unit::KEYWORD)
		return LengthPercentageAuto(LengthPercentageAuto::Auto);

	return LengthPercentageAuto(LengthPercentageAuto::Length,
		ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
}

Style::LengthPercentage ComputeOrigin(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	using namespace Style;
	static_assert(
		(int)OriginX::Left == (int)OriginY::Top && (int)OriginX::Center == (int)OriginY::Center && (int)OriginX::Right == (int)OriginY::Bottom, "");

	if (property->unit == Unit::KEYWORD)
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
	else if (property->unit == Unit::PERCENT)
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	return LengthPercentage(LengthPercentage::Length,
		ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
}

Style::LengthPercentage ComputeMaxSize(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions)
{
	using namespace Style;
	if (Any(property->unit & Unit::KEYWORD))
		return LengthPercentage(LengthPercentage::Length, FLT_MAX);
	else if (Any(property->unit & Unit::PERCENT))
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	const float length = ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions);
	return LengthPercentage(LengthPercentage::Length, length < 0.f ? FLT_MAX : length);
}

uint16_t ComputeBorderWidth(float computed_length)
{
	if (computed_length <= 0.f)
		return 0;

	if (computed_length <= 1.f)
		return 1;

	return uint16_t(computed_length + 0.5f);
}

String GetFontFaceDescription(const String& font_family, Style::FontStyle style, Style::FontWeight weight)
{
	String font_attributes;

	if (style == Style::FontStyle::Italic)
		font_attributes += "italic, ";
	if (weight == Style::FontWeight::Bold)
		font_attributes += "bold, ";
	else if (weight != Style::FontWeight::Auto && weight != Style::FontWeight::Normal)
		font_attributes += "weight=" + ToString((int)weight) + ", ";

	if (font_attributes.empty())
		font_attributes = "regular";
	else
		font_attributes.resize(font_attributes.size() - 2);

	return CreateString("'%s' [%s]", font_family.c_str(), font_attributes.c_str());
}

} // namespace Rml
