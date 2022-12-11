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

#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/ID.h"

namespace Rml {

PropertyDictionary::PropertyDictionary()
{
}

// Sets a property on the dictionary. Any existing property with a similar name will be overwritten.
void PropertyDictionary::SetProperty(PropertyId id, const Property& property)
{
	RMLUI_ASSERT(id != PropertyId::Invalid);
	
	if (property.unit == Property::VARIABLETERM)
	{
		dependentProperties.Insert(id);
		RebuildDependencies();			
	}
	else
	{
		if (dependentProperties.Contains(id))
		{
			dependentProperties.Erase(id);
			RebuildDependencies();			
		}
	}
	
	properties[id] = property;
}

// Removes a property from the dictionary, if it exists.
void PropertyDictionary::RemoveProperty(PropertyId id)
{
	RMLUI_ASSERT(id != PropertyId::Invalid);
	properties.erase(id);
	
	if (dependentProperties.Contains(id))
	{
		dependentProperties.Erase(id);
		RebuildDependencies();
	}
}

// Returns the value of the property with the requested name, if one exists.
const Property* PropertyDictionary::GetProperty(PropertyId id) const
{
	PropertyMap::const_iterator iterator = properties.find(id);
	if (iterator == properties.end())
		return nullptr;

	return &(*iterator).second;
}

// Returns the number of properties in the dictionary.
int PropertyDictionary::GetNumProperties() const
{
	return (int)properties.size();
}

// Returns the map of properties in the dictionary.
const PropertyMap& PropertyDictionary::GetProperties() const
{
	return properties;
}

void PropertyDictionary::SetVariable(VariableId id, const Property &property)
{
	variables.insert_or_assign(id, property);
	
	// TODO: variables with dependencies
}

void PropertyDictionary::RemoveVariable(VariableId id)
{
	variables.erase(id);
	
	// TODO: variables with dependencies
}

const Property *PropertyDictionary::GetVariable(VariableId id) const
{
	VariableMap::const_iterator iterator = variables.find(id);
	if (iterator == variables.end())
		return nullptr;
	
	return &(*iterator).second;
}

void PropertyDictionary::SetDependent(ShorthandId shorthand_id, const VariableTerm &term)
{
  // TODO shorthands with depenencies
}

void PropertyDictionary::RemoveDependent(ShorthandId shorthand_id)
{
	// TODO shorthands with depenencies
}

int PropertyDictionary::GetNumVariables() const
{
	return (int)variables.size();
}

const VariableMap &PropertyDictionary::GetVariables() const
{
	return variables;
}

// Imports potentially un-specified properties into the dictionary.
void PropertyDictionary::Import(const PropertyDictionary& other, int property_specificity)
{
	for (const auto& pair : other.properties)
	{
		const PropertyId id = pair.first;
		const Property& property = pair.second;
		SetProperty(id, property, property_specificity > 0 ? property_specificity : property.specificity);
	}

	for (const auto& pair : other.variables)
	{
		SetVariable(pair.first, pair.second, property_specificity > 0 ? property_specificity : pair.second.specificity);
	}
}

// Merges the contents of another fully-specified property dictionary with this one.
void PropertyDictionary::Merge(const PropertyDictionary& other, int specificity_offset)
{
	for (const auto& pair : other.properties)
	{
		const PropertyId id = pair.first;
		const Property& property = pair.second;
		SetProperty(id, property, property.specificity + specificity_offset);
	}

	for (const auto& pair : other.variables)
	{
		SetVariable(pair.first, pair.second, pair.second.specificity + specificity_offset);
	}
}

void PropertyDictionary::SetSourceOfAllProperties(const SharedPtr<const PropertySource>& property_source)
{
	for (auto& p : properties)
		p.second.source = property_source;
	for (auto& p : variables)
		p.second.source = property_source;
}

// Sets a property on the dictionary and its specificity.
void PropertyDictionary::SetProperty(PropertyId id, const Property& property, int specificity)
{
	PropertyMap::iterator iterator = properties.find(id);
	if (iterator != properties.end() &&
		iterator->second.specificity > specificity)
		return;

	Property& new_property = (properties[id] = property);
	new_property.specificity = specificity;
}

void PropertyDictionary::SetVariable(VariableId id, const Property &variable, int specificity)
{
	VariableMap::iterator iterator = variables.find(id);
	if (iterator != variables.end() &&
		iterator->second.specificity > specificity)
		return;

	Property& new_property = (variables[id] = variable);
	new_property.specificity = specificity;
}

void PropertyDictionary::RebuildDependencies()
{
	dependencies.clear();
	
	for (auto id : dependentProperties)
	{
		auto const& term = properties.at(id).Get<VariableTerm>();
		for (auto const& atom : term)
		{
			if (atom.variable != static_cast<VariableId>(0))
			{
				dependencies.insert(std::make_pair(atom.variable, DependentId(id)));
			}
		}
	}
}

PropertyDictionary::DependentId::DependentId(PropertyId property_id) : type(Type::Property) {
	id.property = property_id;
}

PropertyDictionary::DependentId::DependentId(ShorthandId shorthand_id) : type(Type::Shorthand) {
	id.shorthand = shorthand_id;
}

PropertyDictionary::DependentId::DependentId(VariableId variable_id) : type(Type::Variable) {
	id.variable = variable_id;
}

} // namespace Rml
