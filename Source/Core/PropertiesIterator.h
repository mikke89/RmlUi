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
#ifndef ROCKETCOREPROPERTIESITERATOR_H
#define ROCKETCOREPROPERTIESITERATOR_H

#include "../../Include/Rocket/Core/Types.h"
#include "DirtyPropertyList.h"
#include "ElementDefinition.h"

namespace Rocket {
namespace Core {



// An iterator for local properties defined on an element.
// Note: Modifying the underlying style invalidates the iterator.
class PropertiesIterator {
public:
	using ValueType = std::pair<PropertyId, const Property&>;
	using PropertyIt = PropertyMap::const_iterator;
	using PseudoIt = PseudoClassPropertyDictionary::const_iterator;

	PropertiesIterator(const PseudoClassList& element_pseudo_classes, PropertyIt it_style, PropertyIt it_style_end, PseudoIt it_pseudo, PseudoIt it_pseudo_end, PropertyIt it_base, PropertyIt it_base_end)
		: element_pseudo_classes(&element_pseudo_classes), it_style(it_style), it_style_end(it_style_end), it_pseudo(it_pseudo), it_pseudo_end(it_pseudo_end), it_base(it_base), it_base_end(it_base_end)
	{
		if (this->element_pseudo_classes->empty())
			this->it_pseudo = this->it_pseudo_end;
		ProceedToNextValid();
	}

	PropertiesIterator& operator++() {
		if (it_style != it_style_end)
			// First, we iterate over the local properties
			++it_style;
		else if (it_pseudo != it_pseudo_end)
			// Then, we iterate over the properties given by the pseudo classes in the element's definition
			++i_pseudo_class;
		else
			// Finally, we iterate over the base properties given by the element's definition
			++it_base;
		// If we reached the end of one of the iterator pairs, we need to continue iteration on the next pair.
		ProceedToNextValid();
		return *this;
	}

	ValueType operator*() const
	{
		if (it_style != it_style_end)
			return { it_style->first, it_style->second };
		if (it_pseudo != it_pseudo_end)
			return { it_pseudo->first, it_pseudo->second[i_pseudo_class].second };
		return { it_base->first, it_base->second };
	}

	bool AtEnd() const {
		return at_end;
	}

	// Return the list of pseudo classes which defines the current property, possibly null.
	const PseudoClassList* GetPseudoClassList() const
	{
		if (it_style == it_style_end && it_pseudo != it_pseudo_end)
			return &it_pseudo->second[i_pseudo_class].first;
		return nullptr;
	}

private:
	const PseudoClassList* element_pseudo_classes;
	DirtyPropertyList iterated_properties;
	PropertyIt it_style, it_style_end;
	PseudoIt it_pseudo, it_pseudo_end;
	PropertyIt it_base, it_base_end;
	size_t i_pseudo_class = 0;
	bool at_end = false;

	inline bool IsDirtyRemove(PropertyId id)
	{
		if (!iterated_properties.Contains(id))
		{
			iterated_properties.Insert(id);
			return true;
		}
		return false;
	}

	inline void ProceedToNextValid() {
		for (; it_style != it_style_end; ++it_style)
		{
			if (IsDirtyRemove(it_style->first))
				return;
		}

		// Iterate over all the pseudo classes and match the applicable rules
		for (; it_pseudo != it_pseudo_end; ++it_pseudo)
		{
			const PseudoClassPropertyList& pseudo_list = it_pseudo->second;
			for (; i_pseudo_class < pseudo_list.size(); ++i_pseudo_class)
			{
				if (ElementDefinition::IsPseudoClassRuleApplicable(pseudo_list[i_pseudo_class].first, *element_pseudo_classes))
				{
					if (IsDirtyRemove(it_pseudo->first))
						return;
				}
			}
			i_pseudo_class = 0;
		}

		for (; it_base != it_base_end; ++it_base)
		{
			if (IsDirtyRemove(it_base->first))
				return;
		}

		// All iterators are now at the end
		at_end = true;
	}
};


}
}

#endif