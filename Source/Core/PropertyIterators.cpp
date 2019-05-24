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
#include "../../Include/Rocket/Core/PropertyIterators.h"
#include "ElementDefinition.h"

namespace Rocket {
namespace Core {


// Return the list of pseudo classes which defines the current property, possibly null
ElementDefinitionIterator::ElementDefinitionIterator() : pseudo_classes(nullptr) {}

ElementDefinitionIterator::ElementDefinitionIterator(const StringList& pseudo_classes, PropertyIt it_properties, PseudoIt it_pseudo_class_properties, PropertyIt it_properties_end, PseudoIt it_pseudo_class_properties_end)
	: pseudo_classes(&pseudo_classes), it_properties(it_properties), it_pseudo_class_properties(it_pseudo_class_properties), it_properties_end(it_properties_end), it_pseudo_class_properties_end(it_pseudo_class_properties_end)
{
	proceed_to_next_valid();
}

const PseudoClassList* ElementDefinitionIterator::pseudo_class_list() const
{
	if (it_properties != it_properties_end)
		return nullptr;
	return &it_pseudo_class_properties->second[i_pseudo_class].first;
}

ElementDefinitionIterator& ElementDefinitionIterator::operator++()
{
	// The iteration proceeds as follows:
	//  1. Iterate over all the default properties of the element (with no pseudo classes)
	//  2. Iterate over each pseudo class that has a definition for this property,
	//      testing each one to see if it matches the currently set pseudo classes.
	if (it_properties != it_properties_end)
	{
		++it_properties;
		proceed_to_next_valid();
		return *this;
	}
	++i_pseudo_class;
	proceed_to_next_valid();
	return *this;
}

void ElementDefinitionIterator::proceed_to_next_valid()
{
	if (it_properties == it_properties_end)
	{
		// Iterate over all the pseudo classes and match the applicable rules
		for (; it_pseudo_class_properties != it_pseudo_class_properties_end; ++it_pseudo_class_properties)
		{
			const PseudoClassPropertyList& pseudo_list = it_pseudo_class_properties->second;
			for (; i_pseudo_class < pseudo_list.size(); ++i_pseudo_class)
			{
				if (ElementDefinition::IsPseudoClassRuleApplicable(pseudo_list[i_pseudo_class].first, *pseudo_classes))
				{
					return;
				}
			}
			i_pseudo_class = 0;
		}
	}
}

ElementStyleIterator& ElementStyleIterator::operator++()
{
	// First, we iterate over the local properties
	if (it_properties != it_properties_end)
	{
		++it_properties;
		proceed_to_next_valid();
		return *this;
	}
	// Then, we iterate over the properties given by the element's definition
	++it_definition;
	proceed_to_next_valid();
	return *this;
}


void ElementStyleIterator::proceed_to_next_valid()
{
	// If we've reached the end of the local properties, continue iteration on the definition
	if (it_properties == it_properties_end)
	{
		for (; it_definition != it_definition_end; ++it_definition)
		{
			// Skip this property if it has been overridden by the element's local properties
			if (property_map && property_map->count((*it_definition).first))
				continue;
			return;
		}
	}
}


// Return the list of pseudo classes which defines the current property, possibly null

const PseudoClassList* ElementStyleIterator::pseudo_class_list() const
{
	if (it_properties != it_properties_end)
		return nullptr;
	return it_definition.pseudo_class_list();
}

ElementStyleIterator::ElementStyleIterator() : property_map(nullptr) {}

ElementStyleIterator::ElementStyleIterator(const PropertyMap* property_map, PropertyIt it_properties, DefinitionIt it_definition, PropertyIt it_properties_end, DefinitionIt it_definition_end)
	: property_map(property_map), it_properties(it_properties), it_definition(it_definition), it_properties_end(it_properties_end), it_definition_end(it_definition_end)
{
	proceed_to_next_valid();
}

}
}
