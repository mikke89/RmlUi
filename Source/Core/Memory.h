#pragma once

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
