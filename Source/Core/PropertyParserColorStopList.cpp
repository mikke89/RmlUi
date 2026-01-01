#include "PropertyParserColorStopList.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include <string.h>

namespace Rml {

PropertyParserColorStopList::PropertyParserColorStopList(PropertyParser* parser_color) :
	parser_color(parser_color), parser_length_percent_angle(Unit::LENGTH_PERCENT | Unit::ANGLE, Unit::PERCENT)
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

	const Unit accepted_units = (parameters.count("angle") ? (Unit::ANGLE | Unit::PERCENT) : Unit::LENGTH_PERCENT);

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
			if (!parser_length_percent_angle.ParseValue(p_position, values[i], empty_parameter_map))
				return false;

			if (Any(p_position.unit & accepted_units))
				color_stop.position = NumericValue(p_position.Get<float>(), p_position.unit);
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
