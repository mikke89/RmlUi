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

#ifndef RMLUI_CORE_SPAN_H
#define RMLUI_CORE_SPAN_H

#include "../Config/Config.h"
#include "Header.h"
#include <stdint.h>
#include <type_traits>

namespace Rml {

/**
    Basic implementation of a span, which refers to a contiguous sequence of objects.
 */

template <typename T>
class Span {
public:
	Span() = default;
	Span(T* data, size_t size) : m_data(data), m_size(size) { RMLUI_ASSERT(data != nullptr || size == 0); }

	Span(const Vector<typename std::remove_const_t<T>>& container) : Span(container.data(), container.size()) {}
	Span(Vector<T>& container) : Span(container.data(), container.size()) {}

	T& operator[](size_t index) const
	{
		RMLUI_ASSERT(index < m_size);
		return m_data[index];
	}

	T* data() const { return m_data; }
	size_t size() const { return m_size; }
	bool empty() const { return m_size == 0; }

	T* begin() const { return m_data; }
	T* end() const { return m_data + m_size; }

private:
	T* m_data = nullptr;
	size_t m_size = 0;
};

} // namespace Rml
#endif
