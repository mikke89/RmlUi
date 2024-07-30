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

#include "PropertyParserColour.h"
#include <string.h>

namespace Rml {

const PropertyParserColour::ColourMap PropertyParserColour::html_colours = {
	{"black", Colourb(0, 0, 0)},
	{"silver", Colourb(192, 192, 192)},
	{"gray", Colourb(128, 128, 128)},
	{"grey", Colourb(128, 128, 128)},
	{"white", Colourb(255, 255, 255)},
	{"maroon", Colourb(128, 0, 0)},
	{"red", Colourb(255, 0, 0)},
	{"orange", Colourb(255, 165, 0)},
	{"purple", Colourb(128, 0, 128)},
	{"fuchsia", Colourb(255, 0, 255)},
	{"green", Colourb(0, 128, 0)},
	{"lime", Colourb(0, 255, 0)},
	{"olive", Colourb(128, 128, 0)},
	{"yellow", Colourb(255, 255, 0)},
	{"navy", Colourb(0, 0, 128)},
	{"blue", Colourb(0, 0, 255)},
	{"teal", Colourb(0, 128, 128)},
	{"aqua", Colourb(0, 255, 255)},
	{"transparent", Colourb(0, 0, 0, 0)},
};

PropertyParserColour::PropertyParserColour() {}

PropertyParserColour::~PropertyParserColour() {}

bool PropertyParserColour::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	Colourb colour;
	if (!ParseColour(colour, value))
		return false;

	property.value = Variant(colour);
	property.unit = Unit::COLOUR;

	return true;
}

bool PropertyParserColour::ParseColour(Colourb& colour, const String& value)
{
	if (value.empty())
		return false;

	colour = {};

	// Check for a hex colour.
	if (value[0] == '#')
	{
		char hex_values[4][2] = {{'f', 'f'}, {'f', 'f'}, {'f', 'f'}, {'f', 'f'}};

		switch (value.size())
		{
		// Single hex digit per channel, RGB and alpha.
		case 5:
			hex_values[3][0] = hex_values[3][1] = value[4];
			//-fallthrough
		// Single hex digit per channel, RGB only.
		case 4:
			hex_values[0][0] = hex_values[0][1] = value[1];
			hex_values[1][0] = hex_values[1][1] = value[2];
			hex_values[2][0] = hex_values[2][1] = value[3];
			break;

		// Two hex digits per channel, RGB and alpha.
		case 9:
			hex_values[3][0] = value[7];
			hex_values[3][1] = value[8];
			//-fallthrough
		// Two hex digits per channel, RGB only.
		case 7: memcpy(hex_values, &value.c_str()[1], sizeof(char) * 6); break;

		default: return false;
		}

		// Parse each of the colour elements.
		for (int i = 0; i < 4; i++)
		{
			int tens = Math::HexToDecimal(hex_values[i][0]);
			int ones = Math::HexToDecimal(hex_values[i][1]);
			if (tens == -1 || ones == -1)
				return false;

			colour[i] = (byte)(tens * 16 + ones);
		}
	}
	else if (value.substr(0, 3) == "rgb")
	{
		StringList values;
		values.reserve(4);

		size_t find = value.find('(');
		if (find == String::npos)
			return false;

		size_t begin_values = find + 1;

		StringUtilities::ExpandString(values, value.substr(begin_values, value.rfind(')') - begin_values), ',');

		// Check if we're parsing an 'rgba' or 'rgb' colour declaration.
		if (value.size() > 3 && value[3] == 'a')
		{
			if (values.size() != 4)
				return false;
		}
		else
		{
			if (values.size() != 3)
				return false;

			values.push_back("255");
		}

		// Parse the three RGB values.
		for (int i = 0; i < 4; ++i)
		{
			int component;

			// We're parsing a percentage value.
			if (values[i].size() > 0 && values[i][values[i].size() - 1] == '%')
				component = int((float)atof(values[i].substr(0, values[i].size() - 1).c_str()) * (255.0f / 100.0f));
			// We're parsing a 0 -> 255 integer value.
			else
				component = atoi(values[i].c_str());

			colour[i] = (byte)(Math::Clamp(component, 0, 255));
		}
	}
	else
	{
		// Check for the specification of an HTML colour.
		ColourMap::const_iterator iterator = html_colours.find(StringUtilities::ToLower(value));
		if (iterator == html_colours.end())
			return false;
		else
			colour = (*iterator).second;
	}

	return true;
}

} // namespace Rml
