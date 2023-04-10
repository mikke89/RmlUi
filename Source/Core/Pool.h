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

#ifndef RMLUI_CORE_POOL_H
#define RMLUI_CORE_POOL_H

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

#endif
