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
#ifndef ROCKETCOREPROPERTYITERATORS_H
#define ROCKETCOREPROPERTYITERATORS_H

#include "Types.h"

namespace Rocket {
namespace Core {

/**
	Provides iterators for properties defined in the element's style or definition.
	@author Michael R. P. Ragazzon
 */


// Iterates over the properties of an element definition matching the given pseudo classes.
// Note: Modifying the underlying definition invalidates the iterator.
// Note: 'pseudo_classes' must outlive the iterator.
class ROCKETCORE_API ElementDefinitionIterator {
public:
	using difference_type = std::ptrdiff_t;
	using value_type = std::pair<PropertyId, const Property&>;
	using pointer = value_type*;
	using reference = value_type&;
	using iterator_category = std::input_iterator_tag;

	using PropertyIt = PropertyMap::const_iterator;
	using PseudoIt = PseudoClassPropertyDictionary::const_iterator;

	ElementDefinitionIterator();
	ElementDefinitionIterator(const StringList& pseudo_classes, PropertyIt it_properties, PseudoIt it_pseudo_class_properties, PropertyIt it_properties_end, PseudoIt it_pseudo_class_properties_end);
	ElementDefinitionIterator& operator++();
	bool operator==(const ElementDefinitionIterator& other) const { return pseudo_classes == other.pseudo_classes && it_properties == other.it_properties && it_pseudo_class_properties == other.it_pseudo_class_properties && i_pseudo_class == other.i_pseudo_class; }
	bool operator!=(const ElementDefinitionIterator& other) const { return !(*this == other); }
	
	value_type operator*() const 
	{
		if (it_properties != it_properties_end)
			return { it_properties->first, it_properties->second };
		return { it_pseudo_class_properties->first,  it_pseudo_class_properties->second[i_pseudo_class].second };
	}

	// Return the list of pseudo classes which defines the current property, possibly null
	const PseudoClassList* pseudo_class_list() const;

private:
	const StringList* pseudo_classes;
	PropertyIt it_properties, it_properties_end;
	PseudoIt it_pseudo_class_properties, it_pseudo_class_properties_end;
	size_t i_pseudo_class = 0;

	void proceed_to_next_valid();
};



// An iterator for local properties defined on an element.
// Note: Modifying the underlying style invalidates the iterator.
class ROCKETCORE_API ElementStyleIterator {
public:
	using difference_type = std::ptrdiff_t;
	using value_type = std::pair<PropertyId, const Property&>;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::input_iterator_tag;

	using PropertyIt = PropertyMap::const_iterator;
	using DefinitionIt = ElementDefinitionIterator;

	ElementStyleIterator();
	ElementStyleIterator(const PropertyMap* property_map, PropertyIt it_properties, DefinitionIt it_definition, PropertyIt it_properties_end, DefinitionIt it_definition_end);

	ElementStyleIterator& operator++();
	bool operator==(const ElementStyleIterator& other) const { return property_map == other.property_map && it_properties == other.it_properties && it_definition == other.it_definition; }
	bool operator!=(const ElementStyleIterator& other) const { return !(*this == other); }

	value_type operator*() const 
	{
		if (it_properties != it_properties_end)
			return { it_properties->first, it_properties->second };
		return *it_definition;
	}

	// Return the list of pseudo classes which defines the current property, possibly null
	const PseudoClassList* pseudo_class_list() const;

private:
	const PropertyMap* property_map;
	PropertyIt it_properties, it_properties_end;
	DefinitionIt it_definition, it_definition_end;

	void proceed_to_next_valid();
};


}
}

#endif