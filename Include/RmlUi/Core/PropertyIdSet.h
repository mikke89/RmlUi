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
#ifndef RMLUICOREPROPERTYIDSET_H
#define RMLUICOREPROPERTYIDSET_H

#include "Types.h"
#include "ID.h"
#include <bitset>

namespace Rml {
namespace Core {

class PropertyIdSetIterator;


/*
	PropertyIdSet is a 'set'-like container for PropertyIds. 
	
	It is quite cheap to construct and use, requiring no dynamic allocation for the library-defined IDs as they are based around a bitset.
	Custom IDs on the other hand need to use a more trafitional set, and are thus more expensive to insert. 

	Supports union and intersection operations between two sets, as well as iteration through the IDs that are inserted.
*/


class RMLUICORE_API PropertyIdSet {
private:
	static constexpr size_t N = (size_t)PropertyId::NumDefinedIds;
	std::bitset<N> defined_ids;
	SmallOrderedSet<PropertyId> custom_ids;

public:
	PropertyIdSet() {
		static_assert((size_t)PropertyId::Invalid == 0, "PropertyIdSet makes an assumption that PropertyId::Invalid is zero.");
	}

	void Insert(PropertyId id) {
		if ((size_t)id < N)
			defined_ids.set((size_t)id);
		else
			custom_ids.insert(id);
	}

	void Clear() {
		defined_ids.reset();
		custom_ids.clear();
	}
	void Erase(PropertyId id) {
		if ((size_t)id < N)
			defined_ids.reset((size_t)id);
		else
			custom_ids.erase(id);
	}

	bool Empty() const {
		return defined_ids.none() && custom_ids.empty();
	}
	bool Contains(PropertyId id) const {
		if ((size_t)id < N)
			return defined_ids.test((size_t)id);
		else
			return custom_ids.count(id) == 1;
	}

	size_t Size() const {
		return defined_ids.count() + custom_ids.size();
	}

	// Union with another set
	PropertyIdSet& operator|=(const PropertyIdSet& other)
	{
		defined_ids |= other.defined_ids;
		custom_ids.insert(other.custom_ids.begin(), other.custom_ids.end());
		return *this;
	}

	PropertyIdSet operator|(const PropertyIdSet& other) const
	{
		PropertyIdSet result = *this;
		result |= other;
		return result;
	}

	// Intersection with another set
	PropertyIdSet& operator&=(const PropertyIdSet& other)
	{
		defined_ids &= other.defined_ids;
		if (custom_ids.size() > 0 && other.custom_ids.size() > 0)
		{
			for (auto it = custom_ids.begin(); it != custom_ids.end();)
				if (other.custom_ids.count(*it) == 0)
					it = custom_ids.erase(it);
				else
					++it;
		}
		else
		{
			custom_ids.clear();
		}
		return *this;
	}
	
	PropertyIdSet operator&(const PropertyIdSet& other) const
	{
		PropertyIdSet result;
		result.defined_ids = (defined_ids & other.defined_ids);
		if (custom_ids.size() > 0 && other.custom_ids.size() > 0)
		{
			for (PropertyId id : custom_ids)
				if (other.custom_ids.count(id) == 1)
					result.custom_ids.insert(id);
		}
		return result;
	}

	// Iterator support. Iterates through all the PropertyIds that are set (contained).
	// @note: Modifying the container invalidates the iterators. Only const_iterators are provided.
	inline PropertyIdSetIterator begin() const;
	inline PropertyIdSetIterator end() const;

	// Erases the property id represented by a valid iterator. Invalidates any previous iterators.
	// @return A new valid iterator pointing to the next element or end().
	inline PropertyIdSetIterator Erase(const PropertyIdSetIterator& it);
};



class RMLUICORE_API PropertyIdSetIterator
{
public:
	using CustomIdsIt = SmallOrderedSet<PropertyId>::const_iterator;

	PropertyIdSetIterator() : container(nullptr), defined_ids_index(0), custom_ids_iterator() {}
	PropertyIdSetIterator(const PropertyIdSet* container, size_t defined_ids_index, CustomIdsIt custom_ids_iterator)
		: container(container), defined_ids_index(defined_ids_index), custom_ids_iterator(custom_ids_iterator) 
	{
		ProceedToNextValid();
	}
	
	PropertyIdSetIterator& operator++() {
		if (defined_ids_index < N)
			++defined_ids_index;
		else
			++custom_ids_iterator;
		ProceedToNextValid();
		return *this;
	}

	bool operator==(const PropertyIdSetIterator& other) const {
		return container == other.container && defined_ids_index == other.defined_ids_index && custom_ids_iterator == other.custom_ids_iterator;
	}
	bool operator!=(const PropertyIdSetIterator& other) const { 
		return !(*this == other); 
	}

	PropertyId operator*() const { 
		if (defined_ids_index < N)
			return static_cast<PropertyId>(defined_ids_index);
		else
			return *custom_ids_iterator;
	}

private:

	inline void ProceedToNextValid()
	{
		for (; defined_ids_index < N; ++defined_ids_index)
		{
			if (container->Contains( static_cast<PropertyId>(defined_ids_index) ))
				return;
		}
	}

	static constexpr size_t N = (size_t)PropertyId::NumDefinedIds;
	const PropertyIdSet* container;
	size_t defined_ids_index;
	CustomIdsIt custom_ids_iterator;

	friend PropertyIdSetIterator PropertyIdSet::Erase(const PropertyIdSetIterator&);
};



PropertyIdSetIterator PropertyIdSet::begin() const {
	return PropertyIdSetIterator(this, 1, custom_ids.begin());
}

PropertyIdSetIterator PropertyIdSet::end() const {
	return PropertyIdSetIterator(this, N, custom_ids.end());
}

PropertyIdSetIterator PropertyIdSet::Erase(const PropertyIdSetIterator& it_in) {
	RMLUI_ASSERT(it_in.container == this);
	PropertyIdSetIterator it = it_in;
	if (it.defined_ids_index < N)
	{
		defined_ids.reset(it.defined_ids_index);
		++it;
	}
	else
	{
		it.custom_ids_iterator = custom_ids.erase(it.custom_ids_iterator);
	}
	return it;
}

}
}

#endif