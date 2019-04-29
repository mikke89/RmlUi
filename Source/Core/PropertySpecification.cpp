/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "precompiled.h"
#include "../../Include/Rocket/Core/PropertySpecification.h"
#include "PropertyShorthandDefinition.h"
#include "../../Include/Rocket/Core/Log.h"
#include "../../Include/Rocket/Core/PropertyDefinition.h"
#include "../../Include/Rocket/Core/PropertyDictionary.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"

namespace Rocket {
namespace Core {

PropertySpecification::PropertySpecification()
{
}

PropertySpecification::~PropertySpecification()
{
	for (PropertyMap::iterator iterator = properties.begin(); iterator != properties.end(); ++iterator)
		delete (*iterator).second;

	for (ShorthandMap::iterator iterator = shorthands.begin(); iterator != shorthands.end(); ++iterator)
		delete (*iterator).second;
}

// Registers a property with a new definition.
PropertyDefinition& PropertySpecification::RegisterProperty(PropertyId property_id, const String& default_value, bool inherited, bool forces_layout)
{
	// Create the property and validate the default value.
	PropertyDefinition* property_definition = new PropertyDefinition(default_value, inherited, forces_layout);

	// Delete any existing property.
	PropertyMap::iterator iterator = properties.find(property_id);
	if (iterator != properties.end())
	{
		delete (*iterator).second;
	}
	else
	{
		property_names.insert(property_id);
		if (inherited)
		{
			inherited_property_names.insert(property_id);
		}
	}

	properties[property_id] = property_definition;
	return *property_definition;
}

// Returns a property definition.
const PropertyDefinition* PropertySpecification::GetProperty(PropertyId property_id) const
{
	PropertyMap::const_iterator iterator = properties.find(property_id);
	if (iterator == properties.end())
		return NULL;

	return (*iterator).second;
}

// Fetches a list of the names of all registered property definitions.
const PropertyIdList& PropertySpecification::GetRegisteredProperties(void) const
{
	return property_names;
}

// Fetches a list of the names of all registered property definitions.
const PropertyIdList& PropertySpecification::GetRegisteredInheritedProperties(void) const
{
	return inherited_property_names;
}

// Registers a shorthand property definition.
bool PropertySpecification::RegisterShorthand(PropertyId shorthand_id, const PropertyIdList& property_ids, ShorthandType type)
{
	if (property_ids.empty())
		return false;

	// Construct the new shorthand definition and resolve its properties.
	PropertyShorthandDefinition* property_shorthand = new PropertyShorthandDefinition();
	for(PropertyId property_id : property_ids)
	{
		const PropertyDefinition* property = GetProperty(property_id);
		bool shorthand_found = false;

		if (property == NULL && type == RECURSIVE)
		{
			// See if recursive type points to another shorthand instead
			const PropertyShorthandDefinition* shorthand = GetShorthand(property_id);
			shorthand_found = (shorthand != NULL);
		}

		if (property == NULL && !shorthand_found)
		{
			auto& shorthand_name = GetName(shorthand_id);
			auto& property_name = GetName(property_id);
			Log::Message(Log::LT_ERROR, "Shorthand property '%s' was registered with invalid property '%s'.", shorthand_name.c_str(), property_name.c_str());
			delete property_shorthand;

			return false;
		}
		property_shorthand->properties.emplace_back(property_id, property);
	}

	if (type == AUTO)
	{
		StringList property_names;
		property_names.reserve(property_ids.size());
		for (PropertyId id : property_ids)
			property_names.push_back(GetName(id));

		if (property_names.size() == 4 &&
			property_names[0].find("-top") != String::npos &&
			property_names[1].find("-right") != String::npos &&
			property_names[2].find("-bottom") != String::npos &&
			property_names[3].find("-left") != String::npos)
			property_shorthand->type = BOX;
		else
			property_shorthand->type = FALL_THROUGH;
	}
	else
		property_shorthand->type = type;

	shorthands[shorthand_id] = property_shorthand;
	return true;
}

// Returns a shorthand definition.
const PropertyShorthandDefinition* PropertySpecification::GetShorthand(PropertyId shorthand_name) const
{
	ShorthandMap::const_iterator iterator = shorthands.find(shorthand_name);
	if (iterator == shorthands.end())
		return NULL;

	return (*iterator).second;
}

// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
bool PropertySpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, PropertyId property_id, const String& property_value, const String& source_file, int source_line_number) const
{
	// Attempt to parse as a single property.
	const PropertyDefinition* property_definition = GetProperty(property_id);

	StringList property_values;
	if (!ParsePropertyValues(property_values, property_value, property_definition == NULL) || property_values.size() == 0)
		return false;

	if (property_definition != NULL)
	{
		Property new_property;
		new_property.source = source_file;
		new_property.source_line_number = source_line_number;
		if (property_definition->ParseValue(new_property, property_values[0]))
		{
			dictionary[property_id] = new_property;
			return true;
		}

		return false;
	}

	// Try as a shorthand.
	const PropertyShorthandDefinition* shorthand_definition = GetShorthand(property_id);
	if (shorthand_definition != NULL)
	{
		// If this definition is a 'box'-style shorthand (x-top, x-right, x-bottom, x-left, etc) and there are fewer
		// than four values
		if (shorthand_definition->type == BOX &&
			property_values.size() < 4)
		{
			switch (property_values.size())
			{
				// Only one value is defined, so it is parsed onto all four sides.
				case 1:
				{
					for (int i = 0; i < 4; i++)
					{
						Property new_property;
						if (!shorthand_definition->properties[i].second->ParseValue(new_property, property_values[0]))
							return false;

						new_property.source = source_file;
						new_property.source_line_number = source_line_number;
						dictionary[shorthand_definition->properties[i].first] = new_property;
					}
				}
				break;

				// Two values are defined, so the first one is parsed onto the top and bottom value, the second onto
				// the left and right.
				case 2:
				{
					// Parse the first value into the top and bottom properties.
					Property new_property;
					new_property.source = source_file;
					new_property.source_line_number = source_line_number;

					if (!shorthand_definition->properties[0].second->ParseValue(new_property, property_values[0]))
						return false;
					dictionary[shorthand_definition->properties[0].first] = new_property;

					if (!shorthand_definition->properties[2].second->ParseValue(new_property, property_values[0]))
						return false;
					dictionary[shorthand_definition->properties[2].first] = new_property;

					// Parse the second value into the left and right properties.
					if (!shorthand_definition->properties[1].second->ParseValue(new_property, property_values[1]))
						return false;
					dictionary[shorthand_definition->properties[1].first] = new_property;

					if (!shorthand_definition->properties[3].second->ParseValue(new_property, property_values[1]))
						return false;
					dictionary[shorthand_definition->properties[3].first] = new_property;
				}
				break;

				// Three values are defined, so the first is parsed into the top value, the second onto the left and
				// right, and the third onto the bottom.
				case 3:
				{
					// Parse the first value into the top property.
					Property new_property;
					new_property.source = source_file;
					new_property.source_line_number = source_line_number;

					if (!shorthand_definition->properties[0].second->ParseValue(new_property, property_values[0]))
						return false;
					dictionary[shorthand_definition->properties[0].first] = new_property;

					// Parse the second value into the left and right properties.
					if (!shorthand_definition->properties[1].second->ParseValue(new_property, property_values[1]))
						return false;
					dictionary[shorthand_definition->properties[1].first] = new_property;

					if (!shorthand_definition->properties[3].second->ParseValue(new_property, property_values[1]))
						return false;
					dictionary[shorthand_definition->properties[3].first] = new_property;

					// Parse the third value into the bottom property.
					if (!shorthand_definition->properties[2].second->ParseValue(new_property, property_values[2]))
						return false;
					dictionary[shorthand_definition->properties[2].first] = new_property;
				}
				break;

				default:	break;
			}
		}
		else if (shorthand_definition->type == RECURSIVE)
		{
			bool result = false;

			for (size_t i = 0; i < shorthand_definition->properties.size(); i++)
			{
				const auto& property_name = shorthand_definition->properties[i].first;
				result |= ParsePropertyDeclaration(dictionary, property_name, property_value, source_file, source_line_number);
			}

			if (!result)
				return false;
		}
		else
		{
			size_t value_index = 0;
			size_t property_index = 0;

			for (; value_index < property_values.size() && property_index < shorthand_definition->properties.size(); property_index++)
			{
				Property new_property;
				new_property.source = source_file;
				new_property.source_line_number = source_line_number;

				if (!shorthand_definition->properties[property_index].second->ParseValue(new_property, property_values[value_index]))
				{
					// This definition failed to parse; if we're falling through, try the next property. If there is no
					// next property, then abort!
					if (shorthand_definition->type == FALL_THROUGH)
					{
						if (property_index + 1 < shorthand_definition->properties.size())
							continue;
					}
					return false;
				}

				dictionary[shorthand_definition->properties[property_index].first] = new_property;

				// Increment the value index, unless we're replicating the last value and we're up to the last value.
				if (shorthand_definition->type != REPLICATE ||
					value_index < property_values.size() - 1)
					value_index++;
			}
		}

		return true;
	}

	// Can't find it! Store as an unknown string value.
	Property new_property(property_value, Property::UNKNOWN);
	new_property.source = source_file;
	new_property.source_line_number = source_line_number;
	dictionary[property_id] = new_property;

	return true;
}

// Sets all undefined properties in the dictionary to their defaults.
void PropertySpecification::SetPropertyDefaults(PropertyDictionary& dictionary) const
{
	for (auto& [id, value] : properties)
		if (auto it = dictionary.find(id); it != dictionary.end())
			it->second = *value->GetDefaultValue();
}

bool PropertySpecification::ParsePropertyValues(StringList& values_list, const String& values, bool split_values) const
{
	String value;

	enum ParseState { VALUE, VALUE_PARENTHESIS, VALUE_QUOTE };
	ParseState state = VALUE;
	int open_parentheses = 0;

	size_t character_index = 0;
	char previous_character = 0;
	while (character_index < values.size())
	{
		char character = values[character_index];
		character_index++;

		switch (state)
		{
			case VALUE:
			{
				if (character == ';')
				{
					value = StringUtilities::StripWhitespace(value);
					if (value.size() > 0)
					{
						values_list.push_back(value);
						value.clear();
					}
				}
				else if (StringUtilities::IsWhitespace(character))
				{
					if (split_values)
					{
						value = StringUtilities::StripWhitespace(value);
						if (value.size() > 0)
						{
							values_list.push_back(value);
							value.clear();
						}
					}
					else
						value += character;
				}
				else if (character == '"')
				{
					if (split_values)
					{
						value = StringUtilities::StripWhitespace(value);
						if (value.size() > 0)
						{
							values_list.push_back(value);
							value.clear();
						}
						state = VALUE_QUOTE;
					}
					else
					{
						value += ' ';
						state = VALUE_QUOTE;
					}
				}
				else if (character == '(')
				{
					open_parentheses = 1;
					value += character;
					state = VALUE_PARENTHESIS;
				}
				else
				{
					value += character;
				}
			}
			break;

			case VALUE_PARENTHESIS:
			{
				if (previous_character == '/')
				{
					if (character == ')' || character == '(')
						value += character;
					else
					{
						value += '/';
						value += character;
					}
				}
				else
				{
					if (character == '(')
					{
						open_parentheses++;
						value += character;
					}
					else if (character == ')')
					{
						open_parentheses--;
						value += character;
						if (open_parentheses == 0)
							state = VALUE;
					}
					else if (character != '/')
					{
						value += character;
					}
				}
			}
			break;

			case VALUE_QUOTE:
			{
				if (previous_character == '/')
				{
					if (character == '"')
						value += character;
					else
					{
						value += '/';
						value += character;
					}
				}
				else
				{
					if (character == '"')
					{
						if (split_values)
						{
							value = StringUtilities::StripWhitespace(value);
							if (value.size() > 0)
							{
								values_list.push_back(value);
								value.clear();
							}
						}
						else
							value += ' ';
						state = VALUE;
					}
					else if (character != '/')
					{
						value += character;
					}
				}
			}
		}

		previous_character = character;
	}

	if (state == VALUE)
	{
		value = StringUtilities::StripWhitespace(value);
		if (value.size() > 0)
			values_list.push_back(value);
	}

	return true;
}

}
}
