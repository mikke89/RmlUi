#pragma once

#include "Types.h"

namespace Rml {

/**
	Script pointer.

	Holds a unique, potentially-owning reference to an ObserverPtr compatible object. It is meant to be
	held by a scripting environment to ensure safety and uniformity when an Element hierarchy acquires
	or releases ownership of the object. When releasing ownership, the object is immediately demoted to
	acting like an ObserverPtr. Unlike other types of smart pointer, ScriptPtr's payload must be non-null.
	Therefore it is safe to assume the object is safe to access at all times, regardless of ownership.

	Usage: See ObserverPtr<T>: Derive from EnableObserverPtr<T> and ensure compatibility with ElementPtr.

	Acquiring ownership:
	ElementPtr ownedRef = ...;
	ScriptPtr<T> newOwner = std::move(ownedRef);

	Releasing ownership:
	ScriptPtr<T> scriptPtrOwner = ...;
	ElementPtr newOwner = scriptPtrOwner.ReleaseElementPtr();

	Releasing ownership from the contained object's parent:
	ElementPtr newOwner = scriptPtrOwner.ReleaseFromParent();
 */
template <typename T>
class ScriptPtr {
	public:
		static_assert(alignof(T) > 1 && alignof(Detail::ObserverPtrBlock) > 1,
				"ScriptPtr requires that alignof(T) and the ObserverPtrBlock must both be > 1");

		ScriptPtr(ElementPtr&& element) noexcept : ptr(MakeOwnedPtr(element.release()))
		{
			RMLUI_ASSERT(value_owned())
		}

		ScriptPtr(T& valueIn) noexcept : ptr(nullptr)
		{
			auto observer = valueIn.GetObserverPtr();
			ptr = observer.block;
			observer.block = nullptr;
		}

		~ScriptPtr() noexcept
		{
			reset();
		}

		// Move
		ScriptPtr(ScriptPtr&& other) noexcept
		{
			*this = std::move(other);
		}

		ScriptPtr& operator=(ScriptPtr&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				ptr = std::exchange(other.ptr, nullptr);
			}

			return *this;
		}

		// Copy
		ScriptPtr(const ScriptPtr&) = delete;
		void operator=(const ScriptPtr&) = delete;

		// Returns true if we can dereference the pointer.
		explicit operator bool() const noexcept
		{
			return IsOwner() || block()->pointed_to_object;
		}

		T* get() const noexcept
		{
			return IsOwner() ? value_owned() : value_observed();
		}

		T* operator->() const noexcept
		{
			return get();
		}

		T& operator*() const noexcept
		{
			return *get();
		}

		// Comparison operators return true when they point to the same object, or they are both nullptr or expired.
		bool operator==(const T* other) const noexcept { return get() == other; }
		bool operator==(const ScriptPtr& other) const noexcept { return get() == other.get(); }

		/** Releases ownership of the underlying object and converts to observing the object. Must currently
		own the object. */
		ElementPtr ReleaseElementPtr() noexcept
		{
			RMLUI_ASSERT(IsOwner());
			auto* payload = value_owned();
			auto observer = value_owned()->GetObserverPtr();
			ptr = observer.block;
			observer.block = nullptr;
			return ElementPtr(payload);
		}

		/** Releases ownership of the underlying object and converts to observing the object. If the
		ScriptPtr does not currently own the object, it is assumed that the object's parent is its owner
		and the object is unparented. */
		ElementPtr ReleaseFromParent() noexcept
		{
			if (IsOwner())
				return ReleaseElementPtr();
			else
			{
				RMLUI_ASSERT(value_observed()->GetParentNode());
				auto* elem = value_observed();
				return elem->GetParentNode()->RemoveChild(elem);
			}
		}

		bool IsOwner() const noexcept
		{
			return reinterpret_cast<uintptr_t>(ptr) & static_cast<uintptr_t>(1);
		}
	private:
		void* ptr{};

		Detail::ObserverPtrBlock* block() const noexcept
		{
			return reinterpret_cast<Detail::ObserverPtrBlock*>(ptr);
		}

		T* value_owned() const noexcept
		{
			return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) & ~static_cast<uintptr_t>(1));
		}

		T* value_observed() const noexcept {
			return reinterpret_cast<T*>(block()->pointed_to_object);
		}

		void reset() noexcept 
		{
			if (ptr && IsOwner())
				Releaser<T>{}(value_owned());
			else if (ptr)
			{
				block()->num_observers -= 1;
				Detail::DeallocateObserverPtrBlockIfEmpty(block());
			}

			ptr = nullptr;
		}

		static void* MakeOwnedPtr(void* obj) noexcept
		{
			return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) | static_cast<uintptr_t>(1));
		}
};

}
