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

#include "PropertyParserBoxShadow.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"

namespace Rml {

PropertyParserBoxShadow::PropertyParserBoxShadow(PropertyParser* parser_color, PropertyParser* parser_length) :
	parser_color(parser_color), parser_length(parser_length)
{
	RMLUI_ASSERT(parser_color && parser_length);
}

bool PropertyParserBoxShadow::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	if (value.empty() || value == "none")
	{
		property.value = Variant();
		property.unit = Unit::UNKNOWN;
		return true;
	}

	StringList shadows_string_list;
	{
		auto lowercase_value = StringUtilities::ToLower(value);
		StringUtilities::ExpandString(shadows_string_list, lowercase_value, ',', '(', ')');
	}

	if (shadows_string_list.empty())
		return false;

	const ParameterMap empty_parameter_map;

	BoxShadowList shadow_list;
	shadow_list.reserve(shadows_string_list.size());

	for (const String& shadow_str : shadows_string_list)
	{
		StringList arguments;
		StringUtilities::ExpandString(arguments, shadow_str, ' ', '(', ')');
		if (arguments.empty())
			return false;

		shadow_list.push_back({});
		BoxShadow& shadow = shadow_list.back();

		int length_argument_index = 0;

		for (const String& argument : arguments)
		{
			if (argument.empty())
				continue;

			Property prop;
			if (parser_length->ParseValue(prop, argument, empty_parameter_map))
			{
				switch (length_argument_index)
				{
				case 0: shadow.offset_x = prop.GetNumericValue(); break;
				case 1: shadow.offset_y = prop.GetNumericValue(); break;
				case 2: shadow.blur_radius = prop.GetNumericValue(); break;
				case 3: shadow.spread_distance = prop.GetNumericValue(); break;
				default: return false;
				}
				length_argument_index += 1;
			}
			else if (argument == "inset")
			{
				shadow.inset = true;
			}
			else if (parser_color->ParseValue(prop, argument, empty_parameter_map))
			{
				shadow.color = prop.Get<Colourb>().ToPremultiplied();
			}
			else
			{
				return false;
			}
		}

		if (length_argument_index < 2)
			return false;
	}

	property.unit = Unit::BOXSHADOWLIST;
	property.value = Variant(std::move(shadow_list));

	return true;
}

} // namespace Rml
