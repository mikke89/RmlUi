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

#ifndef RMLUIOBSERVERPTR_H
#define RMLUIOBSERVERPTR_H

#include "Header.h"
#include <type_traits>
#include <utility>

namespace Rml {

struct RMLUICORE_API ObserverPtrBlock {
	int num_observers;
	void* pointed_to_object;
};
RMLUICORE_API ObserverPtrBlock* AllocateObserverPtrBlock();
RMLUICORE_API void DeallocateObserverPtrBlockIfEmpty(ObserverPtrBlock* block);

template <typename T>
class EnableObserverPtr;

/**
    Observer pointer.

    Holds a weak reference to an object owned by someone else. Can tell whether or not the pointed to object has been destroyed.

    Usage: Given a class T, derive from EnableObserverPtr<T>. Then, we can use the observer pointer as follows:

        auto object = MakeUnique<T>();
        ObserverPtr<T> observer_ptr = object->GetObserverPtr();
        // ...
        if(obserer_ptr) {
            // Will only enter if object is still alive.
            observer_ptr->do_a_thing();
        }

    Note: Not thread safe.
 */

template <typename T>
class RMLUICORE_API ObserverPtr {
public:
	ObserverPtr() noexcept : block(nullptr) {}
	ObserverPtr(std::nullptr_t) noexcept : block(nullptr) {}
	~ObserverPtr() noexcept { reset(); }

	// Copy
	ObserverPtr(const ObserverPtr<T>& other) noexcept : block(other.block)
	{
		if (block)
			block->num_observers += 1;
	}
	ObserverPtr<T>& operator=(const ObserverPtr<T>& other) noexcept
	{
		if (this == &other)
			return *this;
		reset();
		block = other.block;
		if (block)
			block->num_observers += 1;
		return *this;
	}

	// Move
	ObserverPtr(ObserverPtr<T>&& other) noexcept : block(std::exchange(other.block, nullptr)) {}
	ObserverPtr<T>& operator=(ObserverPtr<T>&& other) noexcept
	{
		if (this == &other)
			return *this;
		reset();
		block = std::exchange(other.block, nullptr);
		return *this;
	}

	// Returns true if we can dereference the pointer.
	explicit operator bool() const noexcept { return block && block->pointed_to_object; }

	// Comparison operators return true when they point to the same object, or they are both nullptr or expired.
	bool operator==(const T* other) const noexcept { return get() == other; }
	bool operator==(const ObserverPtr<T>& other) const noexcept { return get() == other.get(); }

	// Retrieve the pointer to the observed object if we have one and it's still alive.
	T* get() const noexcept { return block ? static_cast<T*>(block->pointed_to_object) : nullptr; }
	// Dereference the pointed to object.
	T* operator->() const noexcept { return static_cast<T*>(block->pointed_to_object); }

	// Reset the pointer so that it does not point to anything.
	// When the pointed to object and all observer pointers to it have been destroyed, it will deallocate the block.
	void reset() noexcept
	{
		if (block)
		{
			block->num_observers -= 1;
			DeallocateObserverPtrBlockIfEmpty(block);
			block = nullptr;
		}
	}

private:
	friend class Rml::EnableObserverPtr<T>;

	explicit ObserverPtr(ObserverPtrBlock* block) noexcept : block(block)
	{
		if (block)
			block->num_observers += 1;
	}

	ObserverPtrBlock* block;
};

template <typename T>
class RMLUICORE_API EnableObserverPtr {
public:
	ObserverPtr<T> GetObserverPtr()
	{
		InitializeBlock();
		return ObserverPtr<T>(block);
	}

protected:
	EnableObserverPtr() noexcept { static_assert(std::is_base_of<EnableObserverPtr<T>, T>::value, "T must derive from EnableObserverPtr<T>."); }

	~EnableObserverPtr() noexcept
	{
		if (block)
		{
			block->pointed_to_object = nullptr;
			DeallocateObserverPtrBlockIfEmpty(block);
		}
	}

	EnableObserverPtr(const EnableObserverPtr<T>&) noexcept
	{
		// Do not copy or modify the block, it should always point to the same object.
	}
	EnableObserverPtr<T>& operator=(const EnableObserverPtr<T>&) noexcept
	{
		// Assignment should not do anything, the block must point to the initially constructed object.
		return *this;
	}

	EnableObserverPtr(EnableObserverPtr<T>&&) noexcept
	{
		// Do not move or modify the block, it should always point to the same object.
	}
	EnableObserverPtr<T>& operator=(EnableObserverPtr<T>&&) noexcept
	{
		// Assignment should not do anything, the block must point to the initially constructed object.
		return *this;
	}

private:
	inline void InitializeBlock()
	{
		if (!block)
		{
			block = AllocateObserverPtrBlock();
			block->num_observers = 0;
			block->pointed_to_object = static_cast<void*>(static_cast<T*>(this));
		}
	}

	ObserverPtrBlock* block = nullptr;
};

} // namespace Rml
#endif
