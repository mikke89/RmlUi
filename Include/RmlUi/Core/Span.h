#pragma once

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
