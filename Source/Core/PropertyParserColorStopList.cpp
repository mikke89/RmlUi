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
