#include "ComputeProperty.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "ControlledLifetimeResource.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "CalculationResolver.h"
#endif

namespace Rml {

struct ComputedPropertyData {
	const Style::ComputedValues computed{nullptr};
};
static ControlledLifetimeResource<ComputedPropertyData> computed_property_data;

const Style::ComputedValues& DefaultComputedValues()
{
	return computed_property_data->computed;
}

void InitializeComputeProperty()
{
	computed_property_data.Initialize();
}

void ShutdownComputeProperty()
{
	computed_property_data.Shutdown();
}

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

float ComputeFontsize(const Property* property, const Style::ComputedValues& values, const Style::ComputedValues* parent_values,
	const Style::ComputedValues* document_values, float dp_ratio, Vector2f vp_dimensions)
{
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		const float parent_font_size = parent_values ? parent_values->font_size() : 0.f;
		const float root_font_size =
			(!document_values || &values == document_values) ? DefaultComputedValues().font_size() : document_values->font_size();
		CalculationResolverContext context{values.font_size(), parent_font_size, root_font_size, values.line_height().value, vp_dimensions, dp_ratio,
			property->definition ? property->definition->GetRelativeTarget() : RelativeTarget::ParentFontSize};
		ResolvedCalculation resolved;
		const CalculationPtr calculation = property->value.Get<CalculationPtr>();
		if (calculation && ResolveCalculation(*calculation, context, resolved) && resolved.is_constant && resolved.unit == Unit::PX)
			return resolved.value;
		return values.font_size();
	}
#endif

	NumericValue value = property->GetNumericValue();
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
				return value.number * DefaultComputedValues().font_size();

			// Otherwise it is relative to the document font size.
			return value.number * document_values->font_size();
		default: break;
		}
	}

	// Font-relative lengths handled above, other lengths should be handled as normal.
	return ComputeLength(value, 0.f, 0.f, dp_ratio, vp_dimensions);
}

bool ComputeDefiniteLength(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions, float& result)
{
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		CalculationResolverContext context{font_size, font_size, document_font_size, 0.f, vp_dimensions, dp_ratio,
			property->definition ? property->definition->GetRelativeTarget() : RelativeTarget::None};
		ResolvedCalculation resolved;
		const CalculationPtr calculation = property->value.Get<CalculationPtr>();
		if (!calculation || !ResolveCalculation(*calculation, context, resolved) || !resolved.is_constant || resolved.unit != Unit::PX)
			return false;
		result = resolved.value;
		return true;
	}
#endif
	result = ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions);
	return true;
}

bool ComputeDefiniteNumber(const Property* property, float font_size, float document_font_size, float dp_ratio, Vector2f vp_dimensions, float& result)
{
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		CalculationResolverContext context{font_size, font_size, document_font_size, 0.f, vp_dimensions, dp_ratio,
			property->definition ? property->definition->GetRelativeTarget() : RelativeTarget::None};
		ResolvedCalculation resolved;
		const CalculationPtr calculation = property->value.Get<CalculationPtr>();
		if (!calculation || !ResolveCalculation(*calculation, context, resolved) || !resolved.is_constant || resolved.unit != Unit::NUMBER)
			return false;
		result = resolved.value;
		return true;
	}
#else
	(void)font_size;
	(void)document_font_size;
	(void)dp_ratio;
	(void)vp_dimensions;
#endif
	result = property->Get<float>();
	return true;
}

String ComputeFontFamily(String font_family)
{
	return StringUtilities::ToLower(std::move(font_family));
}

Style::Clip ComputeClip(const Property* property)
{
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		float value = 0.f;
		if (ComputeDefiniteNumber(property, 0.f, 0.f, 1.f, Vector2f(1.f), value))
			return Style::Clip(Style::Clip::Type::Number, static_cast<int8_t>(value));
		return Style::Clip();
	}
#endif
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
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		CalculationResolverContext context{font_size, font_size, document_font_size, 0.f, vp_dimensions, dp_ratio,
			property->definition ? property->definition->GetRelativeTarget() : RelativeTarget::FontSize};
		ResolvedCalculation resolved;
		const CalculationPtr calculation = property->value.Get<CalculationPtr>();
		if (calculation && ResolveCalculation(*calculation, context, resolved) && resolved.is_constant)
		{
			if (resolved.unit == Unit::NUMBER)
				return Style::LineHeight(font_size * resolved.value, Style::LineHeight::Number, resolved.value);
			if (resolved.unit == Unit::PX)
				return Style::LineHeight(resolved.value, Style::LineHeight::Length, resolved.value);
		}
		return Style::LineHeight();
	}
#endif
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
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		CalculationResolverContext context{font_size, font_size, document_font_size, line_height, vp_dimensions, dp_ratio,
			property->definition ? property->definition->GetRelativeTarget() : RelativeTarget::LineHeight};
		ResolvedCalculation resolved;
		const CalculationPtr calculation = property->value.Get<CalculationPtr>();
		if (calculation && ResolveCalculation(*calculation, context, resolved) && resolved.is_constant && resolved.unit == Unit::PX)
			return Style::VerticalAlign(resolved.value);
		return Style::VerticalAlign();
	}
#endif
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
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		CalculationResolverContext context{font_size, font_size, document_font_size, 0.f, vp_dimensions, dp_ratio,
			property->definition ? property->definition->GetRelativeTarget() : RelativeTarget::None};
		ResolvedCalculation resolved;
		const CalculationPtr calculation = property->value.Get<CalculationPtr>();
		if (calculation && ResolveCalculation(*calculation, context, resolved))
		{
			if (resolved.is_constant && resolved.unit == Unit::PX)
				return LengthPercentage(LengthPercentage::Length, resolved.value);
			if (resolved.is_constant && resolved.unit == Unit::PERCENT)
				return LengthPercentage(LengthPercentage::Percentage, resolved.value);
			if (resolved.residual)
				return LengthPercentage(std::move(resolved.residual));
		}
		return LengthPercentage();
	}
#endif
	if (property->unit == Unit::PERCENT)
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	return LengthPercentage(LengthPercentage::Length,
		ComputeLength(property->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
}

Style::LengthPercentageAuto ComputeLengthPercentageAuto(const Property* property, float font_size, float document_font_size, float dp_ratio,
	Vector2f vp_dimensions)
{
	using namespace Style;
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		const LengthPercentage value = ComputeLengthPercentage(property, font_size, document_font_size, dp_ratio, vp_dimensions);
		if (value.type == LengthPercentage::Calculation)
			return LengthPercentageAuto(value.calculation);
		if (value.type == LengthPercentage::Percentage)
			return LengthPercentageAuto(LengthPercentageAuto::Percentage, value.value);
		return LengthPercentageAuto(LengthPercentageAuto::Length, value.value);
	}
#endif
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
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
		return ComputeLengthPercentage(property, font_size, document_font_size, dp_ratio, vp_dimensions);
#endif
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
#ifdef RMLUI_MATH_EXPRESSIONS
	if (property->unit == Unit::CALCULATION)
	{
		LengthPercentage value = ComputeLengthPercentage(property, font_size, document_font_size, dp_ratio, vp_dimensions);
		if (value.type == LengthPercentage::Length && value.value < 0.f)
			value.value = FLT_MAX;
		return value;
	}
#endif
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
