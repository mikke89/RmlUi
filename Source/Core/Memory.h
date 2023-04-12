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

#ifndef RMLUI_CORE_MEMORY_H
#define RMLUI_CORE_MEMORY_H

#include "../../Include/RmlUi/Core/Traits.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

namespace Detail {

	/**
	    Basic stack allocator.

	    A very cheap allocator which only moves a pointer up and down during allocation and deallocation, respectively.
	    The allocator is initialized with some fixed memory. If it runs out, it falls back to malloc.

	    Warning: Using this is dangerous as deallocation must happen in exact reverse order of allocation.

	    Do not use this class directly.
	*/
	class BasicStackAllocator {
	public:
		BasicStackAllocator(size_t N);
		~BasicStackAllocator() noexcept;

		void* allocate(size_t alignment, size_t byte_size);
		void deallocate(void* obj) noexcept;

	private:
		const size_t N;
		byte* data;
		void* p;
	};

	BasicStackAllocator& GetGlobalBasicStackAllocator();

} /* namespace Detail */

/**
    Global stack allocator.

    Can very cheaply allocate memory using the global stack allocator. Memory will be allocated from the
    heap on the very first construction of a global stack allocator, and will persist and be re-used after.
    Falls back to malloc if there is not enough space left.

    Warning: Using this is dangerous as deallocation must happen in exact reverse order of allocation.
      Memory is shared between different global stack allocators. Should only be used for highly localized code,
      where memory is allocated and then quickly thrown away.
*/

template <typename T>
class GlobalStackAllocator {
public:
	using value_type = T;

	GlobalStackAllocator() = default;
	template <class U>
	constexpr GlobalStackAllocator(const GlobalStackAllocator<U>&) noexcept
	{}

	T* allocate(size_t num_objects) { return static_cast<T*>(Detail::GetGlobalBasicStackAllocator().allocate(alignof(T), num_objects * sizeof(T))); }

	void deallocate(T* ptr, size_t) noexcept { Detail::GetGlobalBasicStackAllocator().deallocate(ptr); }
};

template <class T, class U>
bool operator==(const GlobalStackAllocator<T>&, const GlobalStackAllocator<U>&)
{
	return true;
}
template <class T, class U>
bool operator!=(const GlobalStackAllocator<T>&, const GlobalStackAllocator<U>&)
{
	return false;
}

/**
    A poor man's dynamic array.

    Constructs N objects on initialization which are default initialized. Can not be resized.
*/

template <typename T, typename Alloc>
class DynamicArray : Alloc, NonCopyMoveable {
public:
	DynamicArray(size_t N) : N(N)
	{
		p = Alloc::allocate(N);
		for (size_t i = 0; i < N; i++)
			new (p + i) T;
	}
	~DynamicArray() noexcept
	{
		for (size_t i = 0; i < N; i++)
			p[i].~T();
		Alloc::deallocate(p, N);
	}

	T* data() noexcept { return p; }

	T& operator[](size_t i) noexcept { return p[i]; }

private:
	size_t N;
	T* p;
};

} // namespace Rml
#endif
