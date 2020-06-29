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

#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"

namespace Rml {

PropertyDefinition::PropertyDefinition(PropertyId id, const String& _default_value, bool _inherited, bool _forces_layout) 
	: id(id), default_value(_default_value, Property::UNKNOWN), relative_target(RelativeTarget::None)
{
	inherited = _inherited;
	forces_layout = _forces_layout;
	default_value.definition = this;
}

PropertyDefinition::~PropertyDefinition()
{
}

// Registers a parser to parse values for this definition.
PropertyDefinition& PropertyDefinition::AddParser(const String& parser_name, const String& parser_parameters)
{
	ParserState new_parser;

	// Fetch the parser.
	new_parser.parser = StyleSheetSpecification::GetParser(parser_name);
	if (new_parser.parser == nullptr)
	{
		Log::Message(Log::LT_ERROR, "Property was registered with invalid parser '%s'.", parser_name.c_str());
		return *this;
	}

	// Split the parameter list, and set up the map.
	if (!parser_parameters.empty())
	{
		StringList parameter_list;
		StringUtilities::ExpandString(parameter_list, parser_parameters);
		for (size_t i = 0; i < parameter_list.size(); i++)
			new_parser.parameters[parameter_list[i]] = (int) i;
	}

	const int parser_index = (int)parsers.size();
	parsers.push_back(new_parser);

	// If the default value has not been parsed successfully yet, run it through the new parser.
	if (default_value.unit == Property::UNKNOWN)
	{
		String unparsed_value = default_value.value.Get< String >();
		if (new_parser.parser->ParseValue(default_value, unparsed_value, new_parser.parameters))
		{
			default_value.parser_index = parser_index;
		}
		else
		{
			default_value.value = unparsed_value;
			default_value.unit = Property::UNKNOWN;
		}
	}

	return *this;
}

// Called when parsing a RCSS declaration.
bool PropertyDefinition::ParseValue(Property& property, const String& value) const
{
	for (size_t i = 0; i < parsers.size(); i++)
	{
		if (parsers[i].parser->ParseValue(property, value, parsers[i].parameters))
		{
			property.definition = this;
			property.parser_index = (int) i;
			return true;
		}
	}

	property.unit = Property::UNKNOWN;
	return false;
}

// Called to convert a parsed property back into a value.
bool PropertyDefinition::GetValue(String& value, const Property& property) const
{
	value = property.value.Get< String >();

	switch (property.unit)
	{
		case Property::KEYWORD:
		{
			int parser_index = property.parser_index;
			if (parser_index < 0 || parser_index >= (int)parsers.size())
			{
				// Look for the keyword parser in the property's list of parsers
				const auto* keyword_parser = StyleSheetSpecification::GetParser("keyword");
				for(int i = 0; i < (int)parsers.size(); i++)
				{
					if (parsers[i].parser == keyword_parser)
					{
						parser_index = i;
						break;
					}
				}
				// If we couldn't find it, exit now
				if (parser_index < 0 || parser_index >= (int)parsers.size())
					return false;
			}

			int keyword = property.value.Get< int >();
			for (ParameterMap::const_iterator i = parsers[parser_index].parameters.begin(); i != parsers[parser_index].parameters.end(); ++i)
			{
				if ((*i).second == keyword)
				{
					value = (*i).first;
					break;
				}
			}

			return false;
		}
		break;

		case Property::COLOUR:
		{
			Colourb colour = property.value.Get< Colourb >();
			value = CreateString(32, "rgba(%d,%d,%d,%d)", colour.red, colour.green, colour.blue, colour.alpha);
		}
		break;

		case Property::PX:		value += "px"; break;
		case Property::DEG:		value += "deg"; break;
		case Property::RAD:		value += "rad"; break;
		case Property::DP:		value += "dp"; break;
		case Property::EM:		value += "em"; break;
		case Property::REM:		value += "rem"; break;
		case Property::PERCENT:	value += "%"; break;
		case Property::INCH:	value += "in"; break;
		case Property::CM:		value += "cm"; break;
		case Property::MM:		value += "mm"; break;
		case Property::PT:		value += "pt"; break;
		case Property::PC:		value += "pc"; break;
		default:					break;
	}

	return true;
}

// Returns true if this property is inherited from a parent to child elements.
bool PropertyDefinition::IsInherited() const
{
	return inherited;
}

// Returns true if this property forces a re-layout when changed.
bool PropertyDefinition::IsLayoutForced() const
{
	return forces_layout;
}

// Returns the default for this property.
const Property* PropertyDefinition::GetDefaultValue() const
{
	return &default_value;
}

/// Returns the default defined for this property.

RelativeTarget PropertyDefinition::GetRelativeTarget() const
{
	return relative_target;
}

PropertyId PropertyDefinition::GetId() const
{
	return id;
}

/// Set target for units with scaling percentages

PropertyDefinition & PropertyDefinition::SetRelativeTarget(RelativeTarget relative_target) 
{
	this->relative_target = relative_target;
	return *this;
}

} // namespace Rml
