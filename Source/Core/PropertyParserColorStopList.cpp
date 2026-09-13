#include "PropertyParserColorStopList.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include <string.h>

namespace Rml {

PropertyParserColorStopList::PropertyParserColorStopList(PropertyParser* parser_color) :
	parser_color(parser_color),
#ifdef RMLUI_MATH_EXPRESSIONS
	parser_length_percent(Unit::LENGTH_PERCENT, Unit::PERCENT,
		MakeCalculationParseTarget(CalculationFinalType::Length, CalculationPercentageHint::Length)),
	parser_angle_percent(Unit::ANGLE | Unit::PERCENT, Unit::PERCENT,
		MakeCalculationParseTarget(CalculationFinalType::Angle, CalculationPercentageHint::Angle))
#else
	parser_length_percent(Unit::LENGTH_PERCENT, Unit::PERCENT), parser_angle_percent(Unit::ANGLE | Unit::PERCENT, Unit::PERCENT)
#endif
{
	RMLUI_ASSERT(parser_color);
}

PropertyParserColorStopList::~PropertyParserColorStopList() {}

bool PropertyParserColorStopList::ParseValue(Property& property, const String& value, const ParameterMap& parameters) const
{
	const ParameterMap empty_parameter_map;

	if (value.empty())
		return false;

	StringList color_stop_str_list;
	StringUtilities::ExpandString(color_stop_str_list, value, ',', '(', ')');

	if (color_stop_str_list.empty())
		return false;

	const bool angle_positions = parameters.count("angle") != 0;
	const Unit accepted_units = (angle_positions ? (Unit::ANGLE | Unit::PERCENT) : Unit::LENGTH_PERCENT);
	const PropertyParserNumber& position_parser = (angle_positions ? parser_angle_percent : parser_length_percent);

	ColorStopList color_stops;
	color_stops.reserve(color_stop_str_list.size());

	for (const String& color_stop_str : color_stop_str_list)
	{
		StringList values;
		StringUtilities::ExpandString(values, color_stop_str, ' ', '(', ')', true);

		if (values.empty() || values.size() > 3)
			return false;

		Property p_color;
		if (!parser_color->ParseValue(p_color, values[0], empty_parameter_map))
			return false;

		ColorStop color_stop = {};
		color_stop.color = p_color.Get<Colourb>().ToPremultiplied();

		if (values.size() <= 1)
			color_stops.push_back(color_stop);

		for (size_t i = 1; i < values.size(); i++)
		{
			Property p_position(Style::LengthPercentageAuto::Auto);
			if (!position_parser.ParseValue(p_position, values[i], empty_parameter_map))
				return false;

			if (Any(p_position.unit & accepted_units))
				color_stop.position = NumericValue(p_position.Get<float>(), p_position.unit);
#ifdef RMLUI_MATH_EXPRESSIONS
			else if (p_position.unit == Unit::CALCULATION)
			{
				color_stop.position = NumericValue(0.f, angle_positions ? Unit::RAD : Unit::PX);
				color_stop.position_calculation = p_position.value.Get<CalculationPtr>();
				if (!color_stop.position_calculation)
					return false;
			}
#endif
			else if (p_position.unit != Unit::KEYWORD)
				return false;

			color_stops.push_back(color_stop);
		}
	}

	property.value = Variant(std::move(color_stops));
	property.unit = Unit::COLORSTOPLIST;

	return true;
}
} // namespace Rml
