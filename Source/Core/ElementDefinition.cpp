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
#include "ElementDefinition.h"
#include "StyleSheetNode.h"
#include "../../Include/Rocket/Core/Log.h"
#include "../../Include/Rocket/Core/PropertyIterators.h"

namespace Rocket {
namespace Core {

ElementDefinition::ElementDefinition()
{
	structurally_volatile = false;
}

ElementDefinition::~ElementDefinition()
{
}

// Initialises the element definition from a list of style sheet nodes.
void ElementDefinition::Initialise(const std::vector< const StyleSheetNode* >& style_sheet_nodes, const PseudoClassList& volatile_pseudo_classes, bool _structurally_volatile)
{
	// Set the volatile structure flag.
	structurally_volatile = _structurally_volatile;

	// Mark all the volatile pseudo-classes as structurally volatile.
	for (const String& pseudo_name : volatile_pseudo_classes)
		pseudo_class_volatility[pseudo_name] = STRUCTURE_VOLATILE;


	// Merge the default (non-pseudo-class) properties.
	for (size_t i = 0; i < style_sheet_nodes.size(); ++i)
		properties.Merge(style_sheet_nodes[i]->GetProperties());


	// Merge the pseudo-class properties.
	for (size_t i = 0; i < style_sheet_nodes.size(); ++i)
	{
		PseudoClassPropertyMap pseudo_properties_map;
		style_sheet_nodes[i]->GetPseudoClassProperties(pseudo_properties_map);
		for (auto& pseudo_properties_pair : pseudo_properties_map)
		{
			const PseudoClassList& pseudo_classes = pseudo_properties_pair.first;
			PropertyDictionary& pseudo_properties = pseudo_properties_pair.second;

			// Search through all entries in this dictionary; we'll insert each one into our optimised list of
			// pseudo-class properties.
			for (auto& property_pair : pseudo_properties.GetProperties())
			{
				PropertyId property_id = property_pair.first;
				const Property& property = property_pair.second;

				// Skip this property if its specificity is lower than the base property's, as in
				// this case it will never be used.
				const Property* default_property = properties.GetProperty(property_id);
				if (default_property && (default_property->specificity >= property.specificity))
					continue;

				auto it = pseudo_class_properties.find(property_id);
				if (it == pseudo_class_properties.end())
					pseudo_class_properties[property_id] = PseudoClassPropertyList(1, PseudoClassProperty(pseudo_classes, property));
				else
				{
					PseudoClassPropertyList& pseudo_property_list = it->second;

					// Find the location to insert this entry in the map, based on property priorities.
					int index = 0;
					while (index < (int)pseudo_property_list.size() && pseudo_property_list[index].second.specificity > property.specificity)
						index++;

					pseudo_property_list.insert(pseudo_property_list.begin() + index, PseudoClassProperty(pseudo_classes, property));
				}
			}
		}
	}
}

// Returns a specific property from the element definition's base properties.
const Property* ElementDefinition::GetProperty(PropertyId id, const PseudoClassList& pseudo_classes) const
{
	// Find a pseudo-class override for this property.
	if(pseudo_class_properties.size() > 0 && pseudo_classes.size() > 0)
	{
		PseudoClassPropertyDictionary::const_iterator property_iterator = pseudo_class_properties.find(id);
		if (property_iterator != pseudo_class_properties.end())
		{
			const PseudoClassPropertyList& property_list = (*property_iterator).second;
			for (size_t i = 0; i < property_list.size(); ++i)
			{
				if (!IsPseudoClassRuleApplicable(property_list[i].first, pseudo_classes))
					continue;

				return &property_list[i].second;
			}
		}
	}

	return properties.GetProperty(id);
}

// Returns the list of properties this element definition defines for an element with the given set of pseudo-classes.
void ElementDefinition::GetDefinedProperties(PropertyNameList& property_names, const PseudoClassList& pseudo_classes) const
{
	for (PropertyMap::const_iterator i = properties.GetProperties().begin(); i != properties.GetProperties().end(); ++i)
		property_names.insert((*i).first);

	for (const auto& pseudo_class_properties_pair : pseudo_class_properties)
	{
		PropertyId property_id = pseudo_class_properties_pair.first;

		// If this property is already in the default dictionary, don't bother checking for it here.
		if (property_names.find(property_id) != property_names.end())
			continue;

		const PseudoClassPropertyList& property_list = pseudo_class_properties_pair.second;

		// Search through all the pseudo-class combinations that have a definition for this property; if the calling
		// element matches at least one of them, then add it to the list.
		bool property_defined = false;
		for (size_t j = 0; j < property_list.size(); ++j)
		{
			if (IsPseudoClassRuleApplicable(property_list[j].first, pseudo_classes))
			{
				property_defined = true;
				break;
			}
		}

		if (property_defined)
			property_names.insert(property_id);
	}
}

// Returns the list of properties this element definition has explicit definitions for involving the given
// pseudo-class.
void ElementDefinition::GetDefinedProperties(PropertyNameList& property_names, const PseudoClassList& pseudo_classes, const String& pseudo_class) const
{
	for (const auto& pseudo_class_properties_pair : pseudo_class_properties)
	{
		PropertyId property_id = pseudo_class_properties_pair.first;

		// If this property has already been found, don't bother checking for it again.
		if (property_names.find(property_id) != property_names.end())
			continue;

		const PseudoClassPropertyList& property_list = pseudo_class_properties_pair.second;

		bool property_defined = false;
		for (size_t j = 0; j < property_list.size(); ++j)
		{
			bool rule_valid = true;
			bool found_toggled_pseudo_class = false;

			const PseudoClassList& rule_pseudo_classes = property_list[j].first;
			for (const String& rule_pseudo_class : rule_pseudo_classes)
			{
				if (rule_pseudo_class == pseudo_class)
				{
					found_toggled_pseudo_class = true;
					continue;
				}

				if (pseudo_classes.count(rule_pseudo_class) == 0)
				{			
					rule_valid = false;
					break;
				}
			}

			if (rule_valid &&
				found_toggled_pseudo_class)
			{
				property_defined = true;
				break;
			}
		}

		if (property_defined)
			property_names.insert(property_id);
	}
}

// Returns the volatility of a pseudo-class.
ElementDefinition::PseudoClassVolatility ElementDefinition::GetPseudoClassVolatility(const String& pseudo_class) const
{
	PseudoClassVolatilityMap::const_iterator i = pseudo_class_volatility.find(pseudo_class);
	if (i == pseudo_class_volatility.end())
		return STABLE;
	else
		return i->second;
}

// Returns true if this definition is built from nodes using structural selectors.
bool ElementDefinition::IsStructurallyVolatile() const
{
	return structurally_volatile;
}

ElementDefinitionIterator ElementDefinition::begin(const PseudoClassList& pseudo_classes) const {
	return ElementDefinitionIterator(pseudo_classes, properties.GetProperties().begin(), pseudo_class_properties.begin(), properties.GetProperties().end(), pseudo_class_properties.end());
}

ElementDefinitionIterator ElementDefinition::end(const PseudoClassList& pseudo_classes) const {
	return ElementDefinitionIterator(pseudo_classes, properties.GetProperties().end(), pseudo_class_properties.end(), properties.GetProperties().end(), pseudo_class_properties.end());
}

// Destroys the definition.
void ElementDefinition::OnReferenceDeactivate()
{
	delete this;
}

// Returns true if the pseudo-class requirement of a rule is met by a list of an element's pseudo-classes.
bool ElementDefinition::IsPseudoClassRuleApplicable(const PseudoClassList& rule_pseudo_classes, const PseudoClassList& element_pseudo_classes)
{
	// The rule pseudo classes must be a subset of the element pseudo classes
	// Note, requires PseudoClassList to be an @ordered container.
	bool result = std::includes(element_pseudo_classes.begin(), element_pseudo_classes.end(), rule_pseudo_classes.begin(), rule_pseudo_classes.end());
	return result;
}

}
}
