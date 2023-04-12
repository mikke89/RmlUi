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

#include "Memory.h"
#include <memory>
#include <stdint.h>
#include <stdlib.h>

namespace Rml {

namespace Detail {

	inline void* rmlui_align(size_t alignment, size_t size, void*& ptr, size_t& space)
	{
#if defined(_MSC_VER)
		return std::align(alignment, size, ptr, space);
#else
		// std::align replacement to support compilers missing this feature.
		// From https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57350

		uintptr_t pn = reinterpret_cast<uintptr_t>(ptr);
		uintptr_t aligned = (pn + alignment - 1) & -alignment;
		size_t padding = aligned - pn;
		if (space < size + padding)
			return nullptr;
		space -= padding;
		return ptr = reinterpret_cast<void*>(aligned);
#endif
	}

	BasicStackAllocator::BasicStackAllocator(size_t N) : N(N), data((byte*)malloc(N)), p(data) {}

	BasicStackAllocator::~BasicStackAllocator() noexcept
	{
		RMLUI_ASSERT(p == data);
		free(data);
	}

	void* BasicStackAllocator::allocate(size_t alignment, size_t byte_size)
	{
		size_t available_space = N - ((byte*)p - data);

		if (rmlui_align(alignment, byte_size, p, available_space))
		{
			void* result = p;
			p = (byte*)p + byte_size;
			return result;
		}

		// Fall back to malloc
		return malloc(byte_size);
	}

	void BasicStackAllocator::deallocate(void* obj) noexcept
	{
		if (obj < data || obj >= data + N)
		{
			free(obj);
			return;
		}
		p = obj;
	}

	BasicStackAllocator& GetGlobalBasicStackAllocator()
	{
		static BasicStackAllocator stack_allocator(10 * 1024);
		return stack_allocator;
	}

} // namespace Detail

} // namespace Rml
