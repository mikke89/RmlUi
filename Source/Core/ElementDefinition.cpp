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
	for (PseudoClassList::const_iterator i = volatile_pseudo_classes.begin(); i != volatile_pseudo_classes.end(); ++i)
		pseudo_class_volatility[*i] = STRUCTURE_VOLATILE;


	// Merge the default (non-pseudo-class) properties.
	for (size_t i = 0; i < style_sheet_nodes.size(); ++i)
		properties.Merge(style_sheet_nodes[i]->GetProperties());


	// Merge the pseudo-class properties.
	PseudoClassPropertyMap merged_pseudo_class_properties;
	for (size_t i = 0; i < style_sheet_nodes.size(); ++i)
	{
		// Merge all the pseudo-classes.
		PseudoClassPropertyMap node_properties;
		style_sheet_nodes[i]->GetPseudoClassProperties(node_properties);
		for (PseudoClassPropertyMap::iterator j = node_properties.begin(); j != node_properties.end(); ++j)
		{
			// Merge the property maps into one uber-map; for the decorators.
			PseudoClassPropertyMap::iterator k = merged_pseudo_class_properties.find((*j).first);
			if (k == merged_pseudo_class_properties.end())
				merged_pseudo_class_properties[(*j).first] = (*j).second;
			else
				(*k).second.Merge((*j).second);

			// Search through all entries in this dictionary; we'll insert each one into our optimised list of
			// pseudo-class properties.
			for (PropertyMap::const_iterator k = (*j).second.GetProperties().begin(); k != (*j).second.GetProperties().end(); ++k)
			{
				PropertyId property_id = (*k).first;
				const Property& property = (*k).second;

				// Skip this property if its specificity is lower than the base property's, as in
				// this case it will never be used.
				const Property* default_property = properties.GetProperty(property_id);
				if (default_property != NULL &&
					default_property->specificity >= property.specificity)
					continue;

				PseudoClassPropertyDictionary::iterator l = pseudo_class_properties.find(property_id);
				if (l == pseudo_class_properties.end())
					pseudo_class_properties[property_id] = PseudoClassPropertyList(1, PseudoClassProperty((*j).first, property));
				else
				{
					// Find the location to insert this entry in the map, based on property priorities.
					int index = 0;
					while (index < (int) (*l).second.size() &&
						   (*l).second[index].second.specificity > property.specificity)
						index++;

					(*l).second.insert((*l).second.begin() + index, PseudoClassProperty((*j).first, property));
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

	for (PseudoClassPropertyDictionary::const_iterator i = pseudo_class_properties.begin(); i != pseudo_class_properties.end(); ++i)
	{
		// If this property is already in the default dictionary, don't bother checking for it here.
		if (property_names.find((*i).first) != property_names.end())
			continue;

		const PseudoClassPropertyList& property_list = (*i).second;

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
			property_names.insert((*i).first);
	}
}

// Returns the list of properties this element definition has explicit definitions for involving the given
// pseudo-class.
void ElementDefinition::GetDefinedProperties(PropertyNameList& property_names, const PseudoClassList& pseudo_classes, const String& pseudo_class) const
{
	for (PseudoClassPropertyDictionary::const_iterator i = pseudo_class_properties.begin(); i != pseudo_class_properties.end(); ++i)
	{
		// If this property has already been found, don't bother checking for it again.
		if (property_names.find((*i).first) != property_names.end())
			continue;

		const PseudoClassPropertyList& property_list = (*i).second;

		bool property_defined = false;
		for (size_t j = 0; j < property_list.size(); ++j)
		{
			bool rule_valid = true;
			bool found_toggled_pseudo_class = false;

			const StringList& rule_pseudo_classes = property_list[j].first;
			for (size_t j = 0; j < rule_pseudo_classes.size(); ++j)
			{
				if (rule_pseudo_classes[j] == pseudo_class)
				{
					found_toggled_pseudo_class = true;
					continue;
				}

				if (std::find(pseudo_classes.begin(), pseudo_classes.end(), rule_pseudo_classes[j]) == pseudo_classes.end())
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
			property_names.insert((*i).first);
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
bool ElementDefinition::IsPseudoClassRuleApplicable(const StringList& rule_pseudo_classes, const PseudoClassList& element_pseudo_classes)
{
	for (StringList::size_type i = 0; i < rule_pseudo_classes.size(); ++i)
	{
		if (std::find(element_pseudo_classes.begin(), element_pseudo_classes.end(), rule_pseudo_classes[i]) == element_pseudo_classes.end())
			return false;
	}

	return true;
}

}
}
