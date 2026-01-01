#pragma once

#include "../../Include/RmlUi/Core/NumericValue.h"
#include "../../Include/RmlUi/Core/StyleTypes.h"

namespace Rml {

class Property;

// Note that numbers and percentages are not lengths, they have to be resolved elsewhere.
float ComputeLength(NumericValue value, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions);

float ComputeAngle(NumericValue value);

float ComputeFontsize(NumericValue value, const Style::ComputedValues& values, const Style::ComputedValues* parent_values,
	const Style::ComputedValues* document_values, float dp_ratio, Vector2f vp_dimensions);

String ComputeFontFamily(String font_family);

Style::Clip ComputeClip(const Property* property);

Style::LineHeight ComputeLineHeight(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions);

Style::VerticalAlign ComputeVerticalAlign(const Property* property, float line_height, float font_size, float document_font_size, float dp_ratio,
	Vector2f vp_dimensions);

Style::LengthPercentage ComputeLengthPercentage(const Property* property, float font_size, float document_font_size, float dp_ratio,
	Vector2f vp_dimensions);

Style::LengthPercentageAuto ComputeLengthPercentageAuto(const Property* property, float font_size, float document_font_size, float dp_ratio,
	Vector2f vp_dimensions);

Style::LengthPercentage ComputeOrigin(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions);

Style::LengthPercentage ComputeMaxSize(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions);

uint16_t ComputeBorderWidth(float computed_length);

String GetFontFaceDescription(const String& font_family, Style::FontStyle style, Style::FontWeight weight);

const Style::ComputedValues& DefaultComputedValues();

void InitializeComputeProperty();
void ShutdownComputeProperty();

} // namespace Rml
