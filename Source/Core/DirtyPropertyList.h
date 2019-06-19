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
#ifndef ROCKETCOREDIRTYPROPERTYLIST_H
#define ROCKETCOREDIRTYPROPERTYLIST_H

#include "../../Include/Rocket/Core/Types.h"
#include "../../Include/Rocket/Core/ID.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"
#include <bitset>

namespace Rocket {
namespace Core {


class DirtyPropertyList {
private:
	static constexpr size_t N = (size_t)PropertyId::NumDefinedIds;
	std::bitset<N> dirty_set;
	PropertyNameList dirty_custom_properties;

public:
	DirtyPropertyList(bool all_dirty = false)
	{
		static_assert((size_t)PropertyId::Invalid == 0, "DirtyPropertyList makes an assumption that PropertyId::Invalid is zero.");
		if (all_dirty)
			DirtyAll();
	}

	void Insert(PropertyId id) {
		if ((size_t)id < N)
			dirty_set.set((size_t)id);
		else
			dirty_custom_properties.insert(id);
	}
	void Insert(const PropertyNameList& properties) {
		if (dirty_set.test(0)) return;
		for (auto id : properties)
			Insert(id);
	}
	void DirtyAll() {
		// We are using PropertyId::Invalid (0) as a flag for all properties dirty.
		// However, we set the whole set so that we don't have to do the extra check
		// for the first bit in Contains().
		dirty_set.set();
	}
	void Clear() {
		dirty_set.reset();
		dirty_custom_properties.clear();
	}
	void Remove(PropertyId id) {
		if ((size_t)id < N)
			dirty_set.reset((size_t)id);
		else
			dirty_custom_properties.erase(id);
	}

	bool Empty() const {
		return dirty_set.none() && dirty_custom_properties.empty();
	}
	bool Contains(PropertyId id) const {
		if ((size_t)id < N)
			return dirty_set.test((size_t)id);
		else if (dirty_set.test(0))
			return true;
		else
			return dirty_custom_properties.count(id) == 1;
	}
	bool IsAllDirty() const {
		return dirty_set.test(0);
	}

	PropertyNameList ToPropertyList() const {
		if (IsAllDirty())
			return StyleSheetSpecification::GetRegisteredProperties();

		PropertyNameList property_list = dirty_custom_properties;
		property_list.reserve(dirty_set.count());
		for (size_t i = 1; i < N; i++)
			if (dirty_set.test(i))
				property_list.insert((PropertyId)i);
		return property_list;
	}
};


}
}

#endif