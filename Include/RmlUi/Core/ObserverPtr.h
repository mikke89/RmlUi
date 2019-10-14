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

#ifndef RMLUIOBSERVERPTR_H
#define RMLUIOBSERVERPTR_H

#include "Types.h"

namespace Rml {
namespace Core {

/**
	Observer pointer.

	Holds a weak reference to something owned by someone else. Will return false if the pointed to object was destroyed.

	Note: Not thread safe.
 */

template<typename T>
class ObserverPtr {
public:
	ObserverPtr(std::weak_ptr<uintptr_t> ptr) : ptr(ptr) {}

	explicit operator bool() const { return !ptr.expired(); }

	T* get() {
		// TODO: Locking it here here does nothing really, but required because we are basing this on a std::shared_ptr for now.
		auto lock = ptr.lock();
		if (!lock)
			return nullptr;
		return reinterpret_cast<T*>(*lock);
	}

	T* operator->() { return get(); }

private:
	WeakPtr<uintptr_t> ptr;
};


template<typename T>
class EnableObserverPtr {
public:
	ObserverPtr<T> GetObserverPtr() const { return ObserverPtr<T>(self_reference); }

protected:
	EnableObserverPtr() : self_reference(std::make_shared<uintptr_t>(reinterpret_cast<uintptr_t>(static_cast<T*>(this))))
	{}

private:
	SharedPtr<uintptr_t> self_reference;
};


struct ObserverPtrBlock {
	int num_observer_pointers = 0;
	uintptr_t pointed_to_object = 0;
};


}
}

#endif
