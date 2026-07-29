#pragma once

#include "RmlUi/Core/ObserverPtr.h"
#include <RmlUi/Core/Types.h>

namespace Rml {

template <typename T>
class ScriptPtr {
	public:
		static_assert(alignof(T) > 1 && alignof(Detail::ObserverPtrBlock) > 1, "ScriptPtr requires that alignof(T) and the ObserverPtrBlock must both be > 1");

		ScriptPtr(ElementPtr&& element) noexcept
			: ptr(element.release())
			, owner(true) {}

		ScriptPtr(T& valueIn) noexcept
				: ptr(nullptr)
				, owner(false) {
			auto observer = valueIn.GetObserverPtr();
			ptr = observer.block;
			observer.block = nullptr;
		}

		~ScriptPtr() noexcept {
			reset();
		}

		// Move
		ScriptPtr(ScriptPtr&& other) noexcept {
			*this = std::move(other);
		}

		ScriptPtr& operator=(ScriptPtr&& other) noexcept {
			if (this != &other) {
				reset();
				ptr = std::exchange(other.ptr, nullptr);
				owner = std::exchange(other.owner, false);
			}

			return *this;
		}

		// Copy
		ScriptPtr(const ScriptPtr&) = delete;
		void operator=(const ScriptPtr&) = delete;

		// Returns true if we can dereference the pointer.
		explicit operator bool() const noexcept {
			return is_owner() || block()->pointed_to_object;
		}

		T* get() const noexcept {
			return is_owner() ? value_owned() : value_observed();
		}

		T* operator->() const noexcept {
			return get();
		}

		T& operator*() const noexcept {
			return *get();
		}

		// Comparison operators return true when they point to the same object, or they are both nullptr or expired.
		bool operator==(const T* other) const noexcept { return get() == other; }
		bool operator==(const ScriptPtr& other) const noexcept { return get() == other.get(); }

		ElementPtr release_ownership() noexcept {
			RMLUI_ASSERT(is_owner());
			auto* payload = value_owned();
			auto observer = value_owned()->GetObserverPtr();
			ptr = observer.block;
			observer.block = nullptr;
			owner = false;
			return ElementPtr(payload);
		}

		ElementPtr release_from_parent() noexcept {
			if (is_owner()) {
				return release_ownership();
			}
			else {
				RMLUI_ASSERT(value_observed()->GetParentNode());
				auto* elem = value_observed();
				return elem->GetParentNode()->RemoveChild(elem);
			}
		}

		bool is_owner() const noexcept {
			return owner;
		}
	private:
		void* ptr{};
		bool owner{};

		Detail::ObserverPtrBlock* block() const noexcept {
			return reinterpret_cast<Detail::ObserverPtrBlock*>(ptr);
		}

		T* value_owned() const noexcept {
			return reinterpret_cast<T*>(ptr);
		}

		T* value_observed() const noexcept {
			return reinterpret_cast<T*>(block()->pointed_to_object);
		}

		void reset() noexcept {
			if (ptr && is_owner()) {
				Releaser<T>{}(value_owned());
			}
			else if (ptr) {
				block()->num_observers -= 1;
				Detail::DeallocateObserverPtrBlockIfEmpty(block());
			}

			ptr = nullptr;
		}
};

}
