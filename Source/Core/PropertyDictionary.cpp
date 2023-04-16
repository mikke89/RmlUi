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

#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/ID.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"

namespace Rml {

PropertyDictionary::PropertyDictionary() {}

bool PropertyDictionary::Empty() const
{
	return properties.empty() && variables.empty();
}

// Sets a property on the dictionary. Any existing property with the same id will be overwritten.
void PropertyDictionary::SetProperty(PropertyId id, const Property& property)
{
	RMLUI_ASSERT(id != PropertyId::Invalid);
	properties[id] = property;
}

void PropertyDictionary::RemoveProperty(PropertyId id)
{
	RMLUI_ASSERT(id != PropertyId::Invalid);
	properties.erase(id);
}

const Property* PropertyDictionary::GetProperty(PropertyId id) const
{
	PropertyMap::const_iterator iterator = properties.find(id);
	if (iterator == properties.end())
		return nullptr;

	return &(*iterator).second;
}

int PropertyDictionary::GetNumProperties() const
{
	return (int)properties.size();
}

const PropertyMap& PropertyDictionary::GetProperties() const
{
	return properties;
}

void PropertyDictionary::SetPropertyVariable(String const& name, const Property &property)
{
	variables[name] = property;
}

void PropertyDictionary::RemovePropertyVariable(String const& name)
{
	variables.erase(name);
}

const Property *PropertyDictionary::GetPropertyVariable(String const& name) const
{
	PropertyVariableMap::const_iterator iterator = variables.find(name);
	if (iterator == variables.end())
		return nullptr;
	
	return &(*iterator).second;
}

const PropertyVariableTerm* PropertyDictionary::GetDependentShorthand(ShorthandId id) const
{
	DependentShorthandMap::const_iterator iter = dependent_shorthands.find(id);
	if (iter == dependent_shorthands.end())
		return nullptr;
	return &iter->second;
}

void PropertyDictionary::SetDependent(ShorthandId shorthand_id, const PropertyVariableTerm &term)
{
	dependent_shorthands[shorthand_id] = term;
	
	// Mark dependent properties as pending
	for (auto id : StyleSheetSpecification::GetShorthandUnderlyingProperties(shorthand_id))
		properties[id] = Property();
}

void PropertyDictionary::RemoveDependent(ShorthandId shorthand_id)
{
	dependent_shorthands.erase(shorthand_id);
}

int PropertyDictionary::GetNumPropertyVariables() const
{
	return (int)variables.size();
}

const PropertyVariableMap &PropertyDictionary::GetPropertyVariables() const
{
	return variables;
}

const DependentShorthandMap &PropertyDictionary::GetDependentShorthands() const
{
	return dependent_shorthands;
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
		SetPropertyVariable(pair.first, pair.second, property_specificity > 0 ? property_specificity : pair.second.specificity);

	for (const auto& pair : other.dependent_shorthands)
		SetDependent(pair.first, pair.second);
}

void PropertyDictionary::Merge(const PropertyDictionary& other, int specificity_offset)
{
	for (const auto& pair : other.properties)
	{
		const PropertyId id = pair.first;
		const Property& property = pair.second;
		SetProperty(id, property, property.specificity + specificity_offset);
	}

	for (const auto& pair : other.variables)
		SetPropertyVariable(pair.first, pair.second, pair.second.specificity + specificity_offset);
		
	for (const auto& pair : other.dependent_shorthands)
		SetDependent(pair.first, pair.second);
}

void PropertyDictionary::SetSourceOfAllProperties(const SharedPtr<const PropertySource>& property_source)
{
	for (auto& p : properties)
		p.second.source = property_source;
	for (auto& p : variables)
		p.second.source = property_source;
}

void PropertyDictionary::SetProperty(PropertyId id, const Property& property, int specificity)
{
	PropertyMap::iterator iterator = properties.find(id);
	if (iterator != properties.end() && iterator->second.specificity > specificity)
		return;

	Property& new_property = (properties[id] = property);
	new_property.specificity = specificity;
}

void PropertyDictionary::SetPropertyVariable(String const& name, const Property &variable, int specificity)
{
	PropertyVariableMap::iterator iterator = variables.find(name);
	if (iterator != variables.end() &&
		iterator->second.specificity > specificity)
		return;

	Property& new_property = (variables[name] = variable);
	new_property.specificity = specificity;
}

} // namespace Rml
