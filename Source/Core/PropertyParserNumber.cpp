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

#include "PropertyParserNumber.h"
#include <stdlib.h>

namespace Rml {

static const UnorderedMap<String, Property::Unit> g_property_unit_string_map =
{
	{"", Property::NUMBER},
	{"%", Property::PERCENT},
	{"px", Property::PX},
	{"dp", Property::DP},
	{"x", Property::X},
	{"vw", Property::VW},
	{"vh", Property::VH},
	{"em", Property::EM},
	{"rem", Property::REM},
	{"in", Property::INCH},
	{"cm", Property::CM},
	{"mm", Property::MM},
	{"pt", Property::PT},
	{"pc", Property::PC},
	{"deg", Property::DEG},
	{"rad", Property::RAD},
};

PropertyParserNumber::PropertyParserNumber(int units, Property::Unit zero_unit)
	: units(units), zero_unit(zero_unit)
{}

PropertyParserNumber::~PropertyParserNumber()
{}

// Called to parse a RCSS number declaration.
bool PropertyParserNumber::ParseValue(Property& property, const String& value, const ParameterMap& RMLUI_UNUSED_PARAMETER(parameters)) const
{
	RMLUI_UNUSED(parameters);

	// Find the beginning of the unit string in 'value'.
	size_t unit_pos = 0;
	for (size_t i = value.size(); i--;)
	{
		const char c = value[i];
		if ((c >= '0' && c <= '9') || StringUtilities::IsWhitespace(c))
		{
			unit_pos = i + 1;
			break;
		}
	}

	String str_number = value.substr(0, unit_pos);
	String str_unit = StringUtilities::ToLower(value.substr(unit_pos));

	char* str_end = nullptr;
	float float_value = strtof(str_number.c_str(), &str_end);
	if (str_number.c_str() == str_end)
	{
		// Number conversion failed
		return false;
	}

	const auto it = g_property_unit_string_map.find(str_unit);
	if (it == g_property_unit_string_map.end())
	{
		// Invalid unit name
		return false;
	}

	const Property::Unit unit = it->second;

	if (unit & units)
	{
		property.value = float_value;
		property.unit = unit;
		return true;
	}

	// Detected unit not allowed.
	// However, we allow a value of "0" if zero_unit is set and no unit specified (that is, unit is a pure NUMBER).
	if (unit == Property::NUMBER)
	{
		if (zero_unit != Property::UNKNOWN && float_value == 0.0f)
		{
			property.unit = zero_unit;
			property.value = Variant(0.0f);
			return true;
		}
	}

	return false;
}

} // namespace Rml
