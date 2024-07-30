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

#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/Debug.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "IdNameMap.h"
#include "PropertyShorthandDefinition.h"
#include <algorithm>
#include <limits.h>
#include <stdint.h>

namespace Rml {

PropertySpecification::PropertySpecification(size_t reserve_num_properties, size_t reserve_num_shorthands) :
	// Increment reserve numbers by one because the 'invalid' property occupies the first element
	properties(reserve_num_properties + 1), shorthands(reserve_num_shorthands + 1),
	property_map(MakeUnique<PropertyIdNameMap>(reserve_num_properties + 1)), shorthand_map(MakeUnique<ShorthandIdNameMap>(reserve_num_shorthands + 1))
{}

PropertySpecification::~PropertySpecification() {}

PropertyDefinition& PropertySpecification::RegisterProperty(const String& property_name, const String& default_value, bool inherited,
	bool forces_layout, PropertyId id)
{
	if (id == PropertyId::Invalid)
		id = property_map->GetOrCreateId(property_name);
	else
		property_map->AddPair(id, property_name);

	size_t index = (size_t)id;

	if (index >= size_t(PropertyId::MaxNumIds))
	{
		Log::Message(Log::LT_ERROR,
			"Fatal error while registering property '%s': Maximum number of allowed properties exceeded. Continuing execution may lead to crash.",
			property_name.c_str());
		RMLUI_ERROR;
		return *properties[0];
	}

	if (index < properties.size())
	{
		// We don't want to owerwrite an existing entry.
		if (properties[index])
		{
			Log::Message(Log::LT_ERROR, "While registering property '%s': The property is already registered.", property_name.c_str());
			return *properties[index];
		}
	}
	else
	{
		// Resize vector to hold the new index
		properties.resize((index * 3) / 2 + 1);
	}

	// Create and insert the new property
	properties[index] = MakeUnique<PropertyDefinition>(id, default_value, inherited, forces_layout);
	property_ids.Insert(id);
	if (inherited)
		property_ids_inherited.Insert(id);
	if (forces_layout)
		property_ids_forcing_layout.Insert(id);

	return *properties[index];
}

const PropertyDefinition* PropertySpecification::GetProperty(PropertyId id) const
{
	if (id == PropertyId::Invalid || (size_t)id >= properties.size())
		return nullptr;

	return properties[(size_t)id].get();
}

const PropertyDefinition* PropertySpecification::GetProperty(const String& property_name) const
{
	return GetProperty(property_map->GetId(property_name));
}

const PropertyIdSet& PropertySpecification::GetRegisteredProperties() const
{
	return property_ids;
}

const PropertyIdSet& PropertySpecification::GetRegisteredInheritedProperties() const
{
	return property_ids_inherited;
}

const PropertyIdSet& PropertySpecification::GetRegisteredPropertiesForcingLayout() const
{
	return property_ids_forcing_layout;
}

ShorthandId PropertySpecification::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type, ShorthandId id)
{
	if (id == ShorthandId::Invalid)
		id = shorthand_map->GetOrCreateId(shorthand_name);
	else
		shorthand_map->AddPair(id, shorthand_name);

	StringList property_list;
	StringUtilities::ExpandString(property_list, StringUtilities::ToLower(property_names));

	// Construct the new shorthand definition and resolve its properties.
	UniquePtr<ShorthandDefinition> property_shorthand(new ShorthandDefinition());

	for (const String& raw_name : property_list)
	{
		ShorthandItem item;
		bool optional = false;
		bool repeats = false;
		String name = raw_name;

		if (!raw_name.empty() && raw_name.back() == '?')
		{
			optional = true;
			name.pop_back();
		}
		if (!raw_name.empty() && raw_name.back() == '#')
		{
			repeats = true;
			name.pop_back();
		}

		PropertyId property_id = property_map->GetId(name);
		if (property_id != PropertyId::Invalid)
		{
			// We have a valid property
			if (const PropertyDefinition* property = GetProperty(property_id))
				item = ShorthandItem(property_id, property, optional, repeats);
		}
		else
		{
			// Otherwise, we must be a shorthand
			ShorthandId shorthand_id = shorthand_map->GetId(name);

			// Test for valid shorthand id. The recursive types (and only those) can hold other shorthands.
			if (shorthand_id != ShorthandId::Invalid && (type == ShorthandType::RecursiveRepeat || type == ShorthandType::RecursiveCommaSeparated))
			{
				if (const ShorthandDefinition* shorthand = GetShorthand(shorthand_id))
					item = ShorthandItem(shorthand_id, shorthand, optional, repeats);
			}
		}

		if (item.type == ShorthandItemType::Invalid)
		{
			Log::Message(Log::LT_ERROR, "Shorthand property '%s' was registered with invalid property '%s'.", shorthand_name.c_str(), name.c_str());
			return ShorthandId::Invalid;
		}
		property_shorthand->items.push_back(item);
	}

	property_shorthand->id = id;
	property_shorthand->type = type;

	const size_t index = (size_t)id;

	if (index >= size_t(ShorthandId::MaxNumIds))
	{
		Log::Message(Log::LT_ERROR, "Error while registering shorthand '%s': Maximum number of allowed shorthands exceeded.", shorthand_name.c_str());
		return ShorthandId::Invalid;
	}

	if (index < shorthands.size())
	{
		// We don't want to owerwrite an existing entry.
		if (shorthands[index])
		{
			Log::Message(Log::LT_ERROR, "The shorthand '%s' already exists, ignoring.", shorthand_name.c_str());
			return ShorthandId::Invalid;
		}
	}
	else
	{
		// Resize vector to hold the new index
		shorthands.resize((index * 3) / 2 + 1);
	}

	shorthands[index] = std::move(property_shorthand);
	return id;
}

const ShorthandDefinition* PropertySpecification::GetShorthand(ShorthandId id) const
{
	if (id == ShorthandId::Invalid || (size_t)id >= shorthands.size())
		return nullptr;

	return shorthands[(size_t)id].get();
}

const ShorthandDefinition* PropertySpecification::GetShorthand(const String& shorthand_name) const
{
	return GetShorthand(shorthand_map->GetId(shorthand_name));
}

bool PropertySpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value) const
{
	RMLUI_ZoneScoped;

	// Try as a property first
	PropertyId property_id = property_map->GetId(property_name);
	if (property_id != PropertyId::Invalid)
		return ParsePropertyDeclaration(dictionary, property_id, property_value);

	// Then, as a shorthand
	ShorthandId shorthand_id = shorthand_map->GetId(property_name);
	if (shorthand_id != ShorthandId::Invalid)
		return ParseShorthandDeclaration(dictionary, shorthand_id, property_value);

	return false;
}

bool PropertySpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, PropertyId property_id, const String& property_value) const
{
	// Parse as a single property.
	const PropertyDefinition* property_definition = GetProperty(property_id);
	if (!property_definition)
		return false;

	StringList property_values;
	if (!ParsePropertyValues(property_values, property_value, SplitOption::None) || property_values.empty())
		return false;

	Property new_property;
	if (!property_definition->ParseValue(new_property, property_values[0]))
		return false;

	dictionary.SetProperty(property_id, new_property);
	return true;
}

bool PropertySpecification::ParseShorthandDeclaration(PropertyDictionary& dictionary, ShorthandId shorthand_id, const String& property_value) const
{
	const ShorthandDefinition* shorthand_definition = GetShorthand(shorthand_id);
	if (!shorthand_definition)
		return false;

	const SplitOption split_option =
		(shorthand_definition->type == ShorthandType::RecursiveCommaSeparated ? SplitOption::Comma : SplitOption::Whitespace);

	StringList property_values;
	if (!ParsePropertyValues(property_values, property_value, split_option) || property_values.empty())
		return false;

	// Handle the special behavior of the flex shorthand first, otherwise it acts like 'FallThrough'.
	if (shorthand_definition->type == ShorthandType::Flex && !property_values.empty())
	{
		RMLUI_ASSERT(shorthand_definition->items.size() == 3);
		if (property_values[0] == "none")
		{
			property_values = {"0", "0", "auto"};
		}
		else
		{
			// Default values when omitted from the 'flex' shorthand is specified here. These defaults are special
			// for this shorthand only, otherwise each underlying property has a different default value.
			const char* default_omitted_values[] = {"1", "1", "0"}; // flex-grow, flex-shrink, flex-basis
			Property new_property;
			bool result = true;
			for (int i = 0; i < 3; i++)
			{
				auto& item = shorthand_definition->items[i];
				result &= item.property_definition->ParseValue(new_property, default_omitted_values[i]);
				dictionary.SetProperty(item.property_id, new_property);
			}
			(void)result;
			RMLUI_ASSERT(result);
		}
	}

	// If this definition is a 'box'-style shorthand (x-top x-right x-bottom x-left) that needs replication.
	if (shorthand_definition->type == ShorthandType::Box && property_values.size() < 4)
	{
		// This array tells which property index each side is parsed from
		Array<int, 4> box_side_to_value_index = {0, 0, 0, 0};
		switch (property_values.size())
		{
		case 1:
			// Only one value is defined, so it is parsed onto all four sides.
			box_side_to_value_index = {0, 0, 0, 0};
			break;
		case 2:
			// Two values are defined, so the first one is parsed onto the top and bottom value, the second onto
			// the left and right.
			box_side_to_value_index = {0, 1, 0, 1};
			break;
		case 3:
			// Three values are defined, so the first is parsed into the top value, the second onto the left and
			// right, and the third onto the bottom.
			box_side_to_value_index = {0, 1, 2, 1};
			break;
		default: RMLUI_ERROR; break;
		}

		for (int i = 0; i < 4; i++)
		{
			RMLUI_ASSERT(shorthand_definition->items[i].type == ShorthandItemType::Property);
			Property new_property;
			int value_index = box_side_to_value_index[i];
			if (!shorthand_definition->items[i].property_definition->ParseValue(new_property, property_values[value_index]))
				return false;

			dictionary.SetProperty(shorthand_definition->items[i].property_definition->GetId(), new_property);
		}
	}
	else if (shorthand_definition->type == ShorthandType::RecursiveRepeat)
	{
		bool result = true;

		for (size_t i = 0; i < shorthand_definition->items.size(); i++)
		{
			const ShorthandItem& item = shorthand_definition->items[i];
			if (item.type == ShorthandItemType::Property)
				result &= ParsePropertyDeclaration(dictionary, item.property_id, property_value);
			else if (item.type == ShorthandItemType::Shorthand)
				result &= ParseShorthandDeclaration(dictionary, item.shorthand_id, property_value);
			else
				result = false;
		}

		if (!result)
			return false;
	}
	else if (shorthand_definition->type == ShorthandType::RecursiveCommaSeparated)
	{
		size_t num_optional = 0;
		for (auto& item : shorthand_definition->items)
			if (item.optional)
				num_optional += 1;

		if (property_values.size() + num_optional < shorthand_definition->items.size())
		{
			// Not enough subvalues declared.
			return false;
		}

		size_t subvalue_i = 0;
		String temp_subvalue;
		for (size_t i = 0; i < shorthand_definition->items.size() && subvalue_i < property_values.size(); i++)
		{
			bool result = false;

			const String* subvalue = &property_values[subvalue_i];

			const ShorthandItem& item = shorthand_definition->items[i];
			if (item.repeats)
			{
				property_values.erase(property_values.begin(), property_values.begin() + subvalue_i);
				temp_subvalue.clear();
				StringUtilities::JoinString(temp_subvalue, property_values);
				subvalue = &temp_subvalue;
			}

			if (item.type == ShorthandItemType::Property)
				result = ParsePropertyDeclaration(dictionary, item.property_id, *subvalue);
			else if (item.type == ShorthandItemType::Shorthand)
				result = ParseShorthandDeclaration(dictionary, item.shorthand_id, *subvalue);

			if (result)
				subvalue_i += 1;
			else if (item.repeats || !item.optional)
				return false;

			if (item.repeats)
				break;
		}
	}
	else
	{
		RMLUI_ASSERT(shorthand_definition->type == ShorthandType::Box || shorthand_definition->type == ShorthandType::FallThrough ||
			shorthand_definition->type == ShorthandType::Replicate || shorthand_definition->type == ShorthandType::Flex);

		// Abort over-specified shorthand values.
		if (property_values.size() > shorthand_definition->items.size())
			return false;

		size_t value_index = 0;
		size_t property_index = 0;

		for (; value_index < property_values.size() && property_index < shorthand_definition->items.size(); property_index++)
		{
			Property new_property;

			if (!shorthand_definition->items[property_index].property_definition->ParseValue(new_property, property_values[value_index]))
			{
				// This definition failed to parse; if we're falling through, try the next property. If there is no
				// next property, then abort!
				if (shorthand_definition->type == ShorthandType::FallThrough || shorthand_definition->type == ShorthandType::Flex)
				{
					if (property_index + 1 < shorthand_definition->items.size())
						continue;
				}
				return false;
			}

			dictionary.SetProperty(shorthand_definition->items[property_index].property_id, new_property);

			// Increment the value index, unless we're replicating the last value and we're up to the last value.
			if (shorthand_definition->type != ShorthandType::Replicate || value_index < property_values.size() - 1)
				value_index++;
		}

		// Abort if we still have values left to parse but no more properties to pass them to.
		if (shorthand_definition->type != ShorthandType::Replicate && value_index < property_values.size() &&
			property_index >= shorthand_definition->items.size())
			return false;
	}

	return true;
}

void PropertySpecification::SetPropertyDefaults(PropertyDictionary& dictionary) const
{
	for (const auto& property : properties)
	{
		if (property && dictionary.GetProperty(property->GetId()) == nullptr)
			dictionary.SetProperty(property->GetId(), *property->GetDefaultValue());
	}
}

String PropertySpecification::PropertiesToString(const PropertyDictionary& dictionary, bool include_name, char delimiter) const
{
	const PropertyMap& properties = dictionary.GetProperties();

	// For determinism we print the strings in order of increasing property ids.
	Vector<PropertyId> ids;
	ids.reserve(properties.size());
	for (auto& pair : properties)
		ids.push_back(pair.first);

	std::sort(ids.begin(), ids.end());

	String result;
	for (PropertyId id : ids)
	{
		const Property& p = properties.find(id)->second;
		if (include_name)
			result += property_map->GetName(id) + ": ";
		result += p.ToString() + delimiter;
	}

	if (!result.empty())
		result.pop_back();

	return result;
}

bool PropertySpecification::ParsePropertyValues(StringList& values_list, const String& values, const SplitOption split_option) const
{
	const bool split_values = (split_option != SplitOption::None);
	const bool split_by_comma = (split_option == SplitOption::Comma);
	const bool split_by_whitespace = (split_option == SplitOption::Whitespace);

	String value;

	auto SubmitValue = [&]() {
		value = StringUtilities::StripWhitespace(value);
		if (value.size() > 0)
		{
			values_list.push_back(value);
			value.clear();
		}
	};

	enum ParseState { VALUE, VALUE_PARENTHESIS, VALUE_QUOTE, VALUE_QUOTE_ESCAPE_NEXT };
	ParseState state = VALUE;
	int open_parentheses = 0;
	size_t character_index = 0;

	while (character_index < values.size())
	{
		const char character = values[character_index];
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
			else if (split_by_comma ? (character == ',') : StringUtilities::IsWhitespace(character))
			{
				if (split_values)
					SubmitValue();
				else
					value += character;
			}
			else if (character == '"')
			{
				state = VALUE_QUOTE;
				if (split_by_whitespace)
					SubmitValue();
				else
					value += (split_by_comma ? '"' : ' ');
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
			if (character == '(')
			{
				open_parentheses++;
			}
			else if (character == ')')
			{
				open_parentheses--;
				if (open_parentheses == 0)
					state = VALUE;
			}
			else if (character == '"')
			{
				state = VALUE_QUOTE;
			}

			value += character;
		}
		break;
		case VALUE_QUOTE:
		{
			if (character == '"')
			{
				if (open_parentheses == 0)
				{
					state = VALUE;
					if (split_by_whitespace)
						SubmitValue();
					else
						value += (split_by_comma ? '"' : ' ');
				}
				else
				{
					state = VALUE_PARENTHESIS;
					value += character;
				}
			}
			else if (character == '\\')
			{
				state = VALUE_QUOTE_ESCAPE_NEXT;
			}
			else
			{
				value += character;
			}
		}
		break;
		case VALUE_QUOTE_ESCAPE_NEXT:
		{
			if (character == '"' || character == '\\')
			{
				value += character;
			}
			else
			{
				value += '\\';
				value += character;
			}
			state = VALUE_QUOTE;
		}
		break;
		}
	}

	if (state == VALUE)
		SubmitValue();

	return true;
}

} // namespace Rml
