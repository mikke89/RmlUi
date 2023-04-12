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
#ifndef RMLUI_CORE_PROPERTYIDSET_H
#define RMLUI_CORE_PROPERTYIDSET_H

#include "ID.h"
#include "Types.h"
#include <bitset>

namespace Rml {

class PropertyIdSetIterator;

/*
    PropertyIdSet is a 'set'-like container for PropertyIds.

    Implemented as a wrapper around bitset. It is cheap to construct and use, requiring no dynamic allocation.

    Supports union and intersection operations between two sets, as well as iteration through the IDs that are inserted.
*/

class RMLUICORE_API PropertyIdSet {
private:
	static constexpr size_t N = size_t(PropertyId::MaxNumIds);
	std::bitset<N> defined_ids;

public:
	PropertyIdSet() { static_assert((size_t)PropertyId::Invalid == 0, "PropertyIdSet makes an assumption that PropertyId::Invalid is zero."); }

	void Insert(PropertyId id)
	{
		RMLUI_ASSERT(size_t(id) < N);
		defined_ids.set((size_t)id);
	}

	void Clear() { defined_ids.reset(); }
	void Erase(PropertyId id)
	{
		RMLUI_ASSERT(size_t(id) < N);
		defined_ids.reset((size_t)id);
	}

	bool Empty() const { return defined_ids.none(); }
	bool Contains(PropertyId id) const { return defined_ids.test((size_t)id); }

	size_t Size() const { return defined_ids.count(); }

	// Union with another set
	PropertyIdSet& operator|=(const PropertyIdSet& other)
	{
		defined_ids |= other.defined_ids;
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
		return *this;
	}
	PropertyIdSet operator&(const PropertyIdSet& other) const
	{
		PropertyIdSet result;
		result.defined_ids = (defined_ids & other.defined_ids);
		return result;
	}

	// Iterator support. Iterates through all the PropertyIds that are set (contained).
	// @note: Only const_iterators are provided.
	inline PropertyIdSetIterator begin() const;
	inline PropertyIdSetIterator end() const;

	// Erases the property id represented by a valid iterator. Iterator must be in the range [begin, end).
	// @return A new iterator pointing to the next element or end().
	inline PropertyIdSetIterator Erase(PropertyIdSetIterator it);
};

class RMLUICORE_API PropertyIdSetIterator {
public:
	PropertyIdSetIterator() = default;
	PropertyIdSetIterator(const PropertyIdSet* container, size_t id_index) : container(container), id_index(id_index) { ProceedToNextValid(); }

	PropertyIdSetIterator& operator++()
	{
		++id_index;
		ProceedToNextValid();
		return *this;
	}

	bool operator==(const PropertyIdSetIterator& other) const { return container == other.container && id_index == other.id_index; }
	bool operator!=(const PropertyIdSetIterator& other) const { return !(*this == other); }
	PropertyId operator*() const { return static_cast<PropertyId>(id_index); }

private:
	inline void ProceedToNextValid()
	{
		for (; id_index < size_t(PropertyId::MaxNumIds); ++id_index)
		{
			if (container->Contains(static_cast<PropertyId>(id_index)))
				return;
		}
	}

	const PropertyIdSet* container = nullptr;
	size_t id_index = 0;

	friend PropertyIdSetIterator PropertyIdSet::Erase(PropertyIdSetIterator);
};

PropertyIdSetIterator PropertyIdSet::begin() const
{
	if (Empty())
		return end();
	return PropertyIdSetIterator(this, 1);
}

PropertyIdSetIterator PropertyIdSet::end() const
{
	return PropertyIdSetIterator(this, N);
}

PropertyIdSetIterator PropertyIdSet::Erase(PropertyIdSetIterator it)
{
	RMLUI_ASSERT(it.container == this && it.id_index < N);
	defined_ids.reset(it.id_index);
	++it;
	return it;
}

} // namespace Rml
#endif
