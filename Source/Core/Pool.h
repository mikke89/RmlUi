#pragma once

#include "../../Include/RmlUi/Core/Debug.h"
#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Traits.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

template <typename PoolType>
class Pool {
private:
	static constexpr size_t N = sizeof(PoolType);
	static constexpr size_t A = alignof(PoolType);

	class PoolNode : public NonCopyMoveable {
	public:
		alignas(A) unsigned char object[N];
		PoolNode* previous;
		PoolNode* next;
	};

	class PoolChunk : public NonCopyMoveable {
	public:
		PoolNode* chunk;
		PoolChunk* next;
	};

public:
	/**
	    Iterator objects are used for safe traversal of the allocated
	    members of a pool.
	 */
	class Iterator {
		friend class Rml::Pool<PoolType>;

	public:
		/// Increments the iterator to reference the next node in the
		/// linked list. It is an error to call this function if the
		/// node this iterator references is invalid.
		inline void operator++()
		{
			RMLUI_ASSERT(node != nullptr);
			node = node->next;
		}
		/// Returns true if it is OK to deference or increment this
		/// iterator.
		explicit inline operator bool() { return (node != nullptr); }

		/// Returns the object referenced by the iterator's current
		/// node.
		inline PoolType& operator*() { return *reinterpret_cast<PoolType*>(node->object); }
		/// Returns a pointer to the object referenced by the
		/// iterator's current node.
		inline PoolType* operator->() { return reinterpret_cast<PoolType*>(node->object); }

	private:
		// Constructs an iterator referencing the given node.
		inline Iterator(PoolNode* node) { this->node = node; }

		PoolNode* node;
	};

	Pool(int chunk_size = 0, bool grow = false);
	~Pool();

	/// Initialises the pool to a given size.
	void Initialise(int chunk_size, bool grow = false);

	/// Returns the head of the linked list of allocated objects.
	inline Iterator Begin();

	/// Attempts to allocate an object into a free slot in the memory pool and construct it using the given arguments.
	/// If the process is successful, the newly constructed object is returned. Otherwise, if the process fails due to
	/// no free objects being available, nullptr is returned.
	template <typename... Args>
	inline PoolType* AllocateAndConstruct(Args&&... args);

	/// Deallocates the object pointed to by the given iterator.
	inline void DestroyAndDeallocate(Iterator& iterator);
	/// Deallocates the given object.
	inline void DestroyAndDeallocate(PoolType* object);

	/// Returns the number of objects in the pool.
	inline int GetSize() const;
	/// Returns the number of object chunks in the pool.
	inline int GetNumChunks() const;
	/// Returns the number of allocated objects in the pool.
	inline int GetNumAllocatedObjects() const;

private:
	// Creates a new pool chunk and appends its nodes to the beginning of the free list.
	void CreateChunk();

	int chunk_size;
	bool grow;

	PoolChunk* pool;

	// The heads of the two linked lists.
	PoolNode* first_allocated_node;
	PoolNode* first_free_node;

	int num_allocated_objects;

#ifdef RMLUI_DEBUG
	int max_num_allocated_objects = 0;
#endif
};

} // namespace Rml

#include "Pool.inl"
