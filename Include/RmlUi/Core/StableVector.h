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

#ifndef RMLUI_CORE_STABLEVECTOR_H
#define RMLUI_CORE_STABLEVECTOR_H

#include "Header.h"
#include "Types.h"
#include <algorithm>
#include <type_traits>

namespace Rml {

/**
    A vector-like container that returns stable indices to refer to entries.

    The indices are only invalidated when the element is erased. Pointers on the other hand are invalidated just like for a
    vector. The container is implemented as a vector with a separate bit mask to track free slots.

    @note For simplicity, freed slots are simply replaced with value-initialized elements instead of being destroyed.
 */
template <typename T>
class StableVector {
public:
	StableVector() = default;

	StableVectorIndex insert(T value)
	{
		const auto it_free = std::find(free_slots.begin(), free_slots.end(), true);
		StableVectorIndex index;
		if (it_free == free_slots.end())
		{
			index = StableVectorIndex(elements.size());
			elements.push_back(std::move(value));
			free_slots.push_back(false);
		}
		else
		{
			const size_t numeric_index = static_cast<size_t>(it_free - free_slots.begin());
			index = static_cast<StableVectorIndex>(numeric_index);
			elements[numeric_index] = std::move(value);
			*it_free = false;
		}
		return index;
	}

	bool empty() const { return elements.size() == count_free_slots(); }
	size_t size() const { return elements.size() - count_free_slots(); }

	void reserve(size_t reserve_size)
	{
		elements.reserve(reserve_size);

#if !defined(__GNUC__) || defined(__llvm__) || (__GNUC__ < 13)
		// Skipped on GCC 13.1+, as this emits a curious warning in some situations.
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110498
		free_slots.reserve(reserve_size);
#endif
	}

	void clear()
	{
		elements.clear();
		free_slots.clear();
	}
	T erase(StableVectorIndex index)
	{
		RMLUI_ASSERT(size_t(index) < elements.size() && !free_slots[size_t(index)]);
		free_slots[size_t(index)] = true;
		return std::exchange(elements[size_t(index)], T());
	}

	T& operator[](StableVectorIndex index)
	{
		RMLUI_ASSERT(size_t(index) < elements.size() && !free_slots[size_t(index)]);
		return elements[size_t(index)];
	}
	const T& operator[](StableVectorIndex index) const
	{
		RMLUI_ASSERT(size_t(index) < elements.size() && !free_slots[size_t(index)]);
		return elements[size_t(index)];
	}

	// Iterate over every item in the vector, skipping free slots.
	template <typename Func>
	void for_each(Func&& func)
	{
		for (size_t i = 0; i < elements.size(); i++)
		{
			if (!free_slots[i])
				func(elements[i]);
		}
	}

private:
	size_t count_free_slots() const { return std::count(free_slots.begin(), free_slots.end(), true); }

	// List of all active elements, including any free slots.
	Vector<T> elements;
	// Declares free slots in 'elements'.
	Vector<bool> free_slots;
};

} // namespace Rml
#endif
