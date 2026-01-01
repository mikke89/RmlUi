#pragma once

#include "Header.h"
#include "Types.h"
#include <algorithm>
#include <type_traits>

namespace Rml {

/**
    A vector-like container that returns stable indices to refer to entries.

    The indices are only invalidated when the element is erased. Pointers on the other hand are invalidated just like for a
    vector. The container is implemented as a vector with a separate list to track free slots.

    @note For simplicity, freed slots are simply replaced with value-initialized elements instead of being destroyed.
 */
template <typename T>
class StableVector {
public:
	StableVector() = default;

	StableVectorIndex insert(T value)
	{
		StableVectorIndex index;
		if (free_slots.empty())
		{
			index = StableVectorIndex(elements.size());
			elements.push_back(std::move(value));
		}
		else
		{
			const uint32_t numeric_index = free_slots.back();
			free_slots.pop_back();
			index = static_cast<StableVectorIndex>(numeric_index);
			elements[numeric_index] = std::move(value);
		}
		return index;
	}

	bool empty() const { return elements.size() == free_slots.size(); }
	size_t size() const { return elements.size() - free_slots.size(); }

	void reserve(size_t reserve_size) { elements.reserve(reserve_size); }

	void clear()
	{
		elements.clear();
		free_slots.clear();
	}
	T erase(StableVectorIndex index)
	{
		RMLUI_ASSERT(size_t(index) < elements.size() && std::find(free_slots.begin(), free_slots.end(), uint32_t(index)) == free_slots.end());
		free_slots.push_back(uint32_t(index));
		return std::exchange(elements[size_t(index)], T());
	}

	T& operator[](StableVectorIndex index)
	{
		RMLUI_ASSERT(size_t(index) < elements.size() && std::find(free_slots.begin(), free_slots.end(), uint32_t(index)) == free_slots.end());
		return elements[size_t(index)];
	}
	const T& operator[](StableVectorIndex index) const
	{
		RMLUI_ASSERT(size_t(index) < elements.size() && std::find(free_slots.begin(), free_slots.end(), uint32_t(index)) == free_slots.end());
		return elements[size_t(index)];
	}

	// Iterate over every item in the vector, skipping free slots. Complexity: O(n*log(n)) in number of free slots.
	template <typename Func>
	void for_each(Func&& func)
	{
		std::sort(free_slots.begin(), free_slots.end());

		size_t i_free_slots = 0;
		for (size_t i_elements = 0; i_elements < elements.size(); ++i_elements)
		{
			while (i_free_slots < free_slots.size() && free_slots[i_free_slots] < i_elements)
				i_free_slots += 1;

			if (i_free_slots < free_slots.size() && free_slots[i_free_slots] == i_elements)
				continue;

			func(elements[i_elements]);
		}
	}

private:
	// List of all active elements, including any free slots.
	Vector<T> elements;
	// Free slots as indices into 'elements'.
	Vector<uint32_t> free_slots;
};

} // namespace Rml
