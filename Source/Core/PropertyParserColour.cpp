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
#include "ControlledLifetimeResource.h"
#include <algorithm>
#include <cmath>
#include <string.h>
#include <utility>

namespace Rml {

// Helper function for hsl->rgb conversion.
static float HSL_f(float h, float s, float l, float n)
{
	float k = std::fmod((n + h * (1.0f / 30.0f)), 12.0f);
	float a = s * std::min(l, 1.0f - l);
	return l - a * std::max(-1.0f, std::min({k - 3.0f, 9.0f - k, 1.0f}));
}

// Reference: https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB_alternative
static void HSLAToRGBA(Array<float, 4>& vals)
{
	if (vals[1] == 0.0f)
	{
		vals[0] = vals[1] = vals[2];
	}
	else
	{
		float h = std::fmod(vals[0], 360.0f);
		if (h < 0)
			h += 360.0f;
		float s = vals[1];
		float l = vals[2];
		vals[0] = HSL_f(h, s, l, 0.0f);
		vals[1] = HSL_f(h, s, l, 8.0f);
		vals[2] = HSL_f(h, s, l, 4.0f);
	}
}

// Reference: https://en.wikipedia.org/wiki/SRGB#Definition
static float InverseSRGBNonlinearTransfer(float channel)
{
	return channel > 0.0031308f ? 1.055f * std::pow(channel, 1.0f / 2.4f) - 0.055f : 12.92f * channel;
}

// References: https://en.wikipedia.org/wiki/Oklab_color_space#Conversions_between_color_spaces and https://bottosson.github.io/posts/oklab/
static void OklabToRGBA(Array<float, 4>& values)
{
	static constexpr Array<Array<float, 3>, 3> oklab_to_lms_prime_matrix{
		Array<float, 3>{+1.0f, +0.3963377774f, +0.2158037573f},
		Array<float, 3>{+1.0f, -0.1055613458f, -0.0638541728f},
		Array<float, 3>{+1.0f, -0.0894841775f, -1.2914855480f},
	};

	float lightness = values[0];
	float a_axis = values[1];
	float b_axis = values[2];

	float l_prime = oklab_to_lms_prime_matrix[0][0] * lightness + oklab_to_lms_prime_matrix[0][1] * a_axis + oklab_to_lms_prime_matrix[0][2] * b_axis;
	float m_prime = oklab_to_lms_prime_matrix[1][0] * lightness + oklab_to_lms_prime_matrix[1][1] * a_axis + oklab_to_lms_prime_matrix[1][2] * b_axis;
	float s_prime = oklab_to_lms_prime_matrix[2][0] * lightness + oklab_to_lms_prime_matrix[2][1] * a_axis + oklab_to_lms_prime_matrix[2][2] * b_axis;

	float l = l_prime * l_prime * l_prime;
	float m = m_prime * m_prime * m_prime;
	float s = s_prime * s_prime * s_prime;

	static constexpr Array<Array<float, 3>, 3> lms_to_srgb_matrix{
		Array<float, 3>{+4.0767416621f, -3.3077115913f, +0.2309699292f},
		Array<float, 3>{-1.2684380046f, +2.6097574011f, -0.3413193965f},
		Array<float, 3>{-0.0041960863f, -0.7034186147f, +1.7076147010f},
	};

	float r = lms_to_srgb_matrix[0][0] * l + lms_to_srgb_matrix[0][1] * m + lms_to_srgb_matrix[0][2] * s;
	float g = lms_to_srgb_matrix[1][0] * l + lms_to_srgb_matrix[1][1] * m + lms_to_srgb_matrix[1][2] * s;
	float b = lms_to_srgb_matrix[2][0] * l + lms_to_srgb_matrix[2][1] * m + lms_to_srgb_matrix[2][2] * s;

	values[0] = Math::Clamp(InverseSRGBNonlinearTransfer(r), 0.0f, 1.0f);
	values[1] = Math::Clamp(InverseSRGBNonlinearTransfer(g), 0.0f, 1.0f);
	values[2] = Math::Clamp(InverseSRGBNonlinearTransfer(b), 0.0f, 1.0f);
}

struct PropertyParserColourData {
	const UnorderedMap<String, Colourb> html_colours = {
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
};

ControlledLifetimeResource<PropertyParserColourData> PropertyParserColour::parser_data;

void PropertyParserColour::Initialize()
{
	parser_data.Initialize();
}
void PropertyParserColour::Shutdown()
{
	parser_data.Shutdown();
}

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

	if (value[0] == '#')
	{
		if (!ParseHexColour(colour, value))
			return false;
	}
	else if (value.substr(0, 3) == "rgb")
	{
		if (!ParseRGBColour(colour, value))
			return false;
	}
	else if (value.substr(0, 3) == "hsl")
	{
		if (!ParseHSLColour(colour, value))
			return false;
	}
	else if (value.substr(0, 5) == "oklab" || value.substr(0, 5) == "oklch")
	{
		if (!ParseOklabColour(colour, value))
			return false;
	}
	else
	{
		// Check for the specification of an HTML colour.
		auto it = parser_data->html_colours.find(StringUtilities::ToLower(value));
		if (it == parser_data->html_colours.end())
			return false;
		else
			colour = it->second;
	}

	return true;
}

bool PropertyParserColour::ParseHexColour(Colourb& colour, const String& value)
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

	return true;
}

bool PropertyParserColour::ParseRGBColour(Colourb& colour, const String& value)
{
	StringList values;
	values.reserve(4);
	if (!GetColourFunctionValues(values, value, true))
		return false;

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

	// Parse the RGBA values.
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

	return true;
}

bool PropertyParserColour::ParseHSLColour(Colourb& colour, const String& value)
{
	StringList values;
	values.reserve(4);
	if (!GetColourFunctionValues(values, value, true))
		return false;

	// Check if we're parsing an 'hsla' or 'hsl' colour declaration.
	if (value.size() > 3 && value[3] == 'a')
	{
		if (values.size() != 4)
			return false;
	}
	else
	{
		if (values.size() != 3)
			return false;

		values.push_back("1.0");
	}

	// Parse the HSLA values.
	Array<float, 4> vals;
	// H is a number in degrees, A is a number between 0.0 and 1.0.
	for (int i : {0, 3})
		vals[i] = (float)atof(values[i].c_str());
	// S and L are percentage values.
	for (int i : {1, 2})
		if (values[i].size() > 0 && values[i][values[i].size() - 1] == '%')
			vals[i] = (float)atof(values[i].substr(0, values[i].size() - 1).c_str()) * (1.0f / 100.0f);
		else
			return false;

	HSLAToRGBA(vals);
	for (int i = 0; i < 4; ++i)
		colour[i] = (byte)(Math::Clamp((int)(vals[i] * 255.0f), 0, 255));

	return true;
}

bool PropertyParserColour::ParseOklabColour(Colourb& colour, const String& value)
{
	StringList values;
	values.reserve(5);
	if (!GetColourFunctionValues(values, value, false))
		return false;

	// Check if we have an alpha component.
	if (values.size() == 5)
	{
		if (values[3] != "/")
			return false;

		values[3] = std::move(values[4]);
		values.pop_back();
	}
	else
	{
		if (values.size() != 3)
			return false;

		values.push_back("1.0");
	}

	Array<float, 4> oklab_values;

	// Parse lightness and alpha (same for both Oklab and Oklch).
	for (int i : {0, 3})
	{
		// Value can either be 'none' (representing 0.0), a percentage between 0% and 100%, or a number between 0.0 and 1.0.
		if (values[i] == "none")
			oklab_values[i] = 0.0f;
		else if (values[i][values[i].size() - 1] == '%')
			oklab_values[i] = (float)atof(values[i].substr(0, values[i].size() - 1).c_str()) / 100.0f;
		else
			oklab_values[i] = (float)atof(values[i].c_str());

		oklab_values[i] = Math::Clamp(oklab_values[i], 0.0f, 1.0f);
	}

	// Determine if colour is in Oklab or Oklch space.
	if (value.substr(0, 5) == "oklab")
	{
		// Parse A-axis (green-to-red) and B-axis (blue-to-yellow).
		for (int i : {1, 2})
		{
			// Value can either be 'none' (representing 0.0), a percentage between -100% and +100% (representing -0.4 to +0.4), or a number.
			if (values[i] == "none")
				oklab_values[i] = 0.0f;
			else if (values[i][values[i].size() - 1] == '%')
			{
				static constexpr float oklab_axis_percentage_bound = 0.4f;
				oklab_values[i] = (float)atof(values[i].substr(0, values[i].size() - 1).c_str()) / 100.0f * oklab_axis_percentage_bound;
			}
			else
				oklab_values[i] = (float)atof(values[i].c_str());

			// Whilst the axis values are theoretically unbounded, in practice, they only exist between -0.5 and +0.5.
			static constexpr float oklab_axis_bound_limit = 0.5f;
			oklab_values[i] = Math::Clamp(oklab_values[i], -oklab_axis_bound_limit, +oklab_axis_bound_limit);
		}
	}
	else
	{
		// Parse chroma; value can either be 'none' (representing 0.0), a percentage between 0% and 100% (representing 0.0 to 0.4), or a number.
		float chroma = 0.0f;
		if (values[1] == "none")
			chroma = 0.0f;
		else if (values[1][values[1].size() - 1] == '%')
		{
			static constexpr float oklch_maximum_percentage_chroma = 0.4f;
			chroma = (float)atof(values[1].substr(0, values[1].size() - 1).c_str()) / 100.0f * oklch_maximum_percentage_chroma;
		}
		else
			chroma = (float)atof(values[1].c_str());

		// Whilst the chroma is theoretically unbounded, in practice, it does not exceed 0.5.
		static constexpr float oklch_maximum_chroma = 0.5f;
		chroma = Math::Clamp(chroma, 0.0f, oklch_maximum_chroma);

		// Parse hue; value can either be 'none' (representing 0.0), or an angle.
		float hue = 0.0f;
		if (values[2] == "none")
			hue = 0.0f;
		else
			hue = (float)atof(values[2].c_str());

		// Clamp hue between 0.0 and 360.0 degrees.
		hue = std::fmod(hue, 360.0f);
		if (hue < 0)
			hue += 360.0f;

		// Convert Oklch polar coordinates to Oklab Cartesian coordinates.
		oklab_values[1] = chroma * Math::Cos(Math::DegreesToRadians(hue));
		oklab_values[2] = chroma * Math::Sin(Math::DegreesToRadians(hue));
	}

	OklabToRGBA(oklab_values);
	for (int i = 0; i < 4; ++i)
		colour[i] = (byte)(Math::Clamp((int)(oklab_values[i] * 255.0f), 0, 255));

	return true;
}

bool PropertyParserColour::GetColourFunctionValues(StringList& values, const String& value, bool isCommaSeparated)
{
	size_t find = value.find('(');
	if (find == String::npos)
		return false;

	size_t begin_values = find + 1;

	if (isCommaSeparated)
		StringUtilities::ExpandString(values, value.substr(begin_values, value.rfind(')') - begin_values), ',');
	else
		StringUtilities::ExpandString(values, value.substr(begin_values, value.rfind(')') - begin_values), ' ', '\"', '\"', true);

	return true;
}

} // namespace Rml
