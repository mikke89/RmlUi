/*
 * flat_handle_map
 *
 * A container which provides stable handles to inserted elements. Elements are accessed and erased
 * in constant time from handles. Insert and emplace occur in amortized constant time.
 *
 * Handle map operates similar to std::map<handle, T>, however, elements are stored in a vector-like
 * underlying container. Handles and iterators remain valid even when other elements are added or
 * erased, but valid elements are not necessarily stored contiguously. References to elements are
 * invalidated when inserting into the container, but not when erasing.
 *
 * Internally, a list of occupied elements are kept. When an element is erased, it is destroyed and
 * marked as available. During insertion, the first available element is re-used. If they are all
 * taken, the underlying container size is increased.
 *
 * MIT License
 *
 * Copyright (c) 2022 Michael R. P. Ragazzon
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

#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

template <typename T, class Alloc = std::allocator<T>>
struct flat_handle_map {
public:
	enum class handle : size_t {};

	class const_iterator {
	private:
		const flat_handle_map& container;
		size_t i;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;

		explicit const_iterator(const flat_handle_map& container, size_t i) : container(container), i(i) {}
		reference operator*() const { return *(container.data() + i); }
		bool operator==(const const_iterator& other) const { return i == other.i; }
		bool operator!=(const const_iterator& other) const { return !(*this == other); }
		const_iterator operator++(int)
		{
			const_iterator retval = *this;
			++(*this);
			return retval;
		}
		const_iterator& operator++()
		{
			++i;
			i = container.get_next_occupied(i);
			return *this;
		}
		handle get_handle() const { return static_cast<handle>(i); }
	};

	class iterator {
	private:
		flat_handle_map& container;
		size_t i;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		explicit iterator(flat_handle_map& container, size_t i) : container(container), i(i) {}
		reference operator*() const { return *(container.data() + i); }
		bool operator==(const iterator& other) const { return i == other.i; }
		bool operator!=(const iterator& other) const { return !(*this == other); }
		iterator operator++(int)
		{
			iterator retval = *this;
			++(*this);
			return retval;
		}
		iterator& operator++()
		{
			++i;
			i = container.get_next_occupied(i);
			return *this;
		}
		operator const_iterator() const { return const_iterator{container, i}; }
		handle get_handle() const { return static_cast<handle>(i); }
	};

	using AllocatorTraits = std::allocator_traits<Alloc>;
	using allocator_type = Alloc;
	using value_type = typename AllocatorTraits::value_type;
	using size_type = typename AllocatorTraits::size_type;
	using difference_type = typename AllocatorTraits::difference_type;
	using reference = typename value_type&;
	using const_reference = typename const value_type&;
	using pointer = typename AllocatorTraits::pointer;
	using const_pointer = typename AllocatorTraits::const_pointer;

	flat_handle_map() : flat_handle_map(Alloc()) {}

	flat_handle_map(const Alloc& alloc) : m_capacity(0), m_alloc(alloc) { m_begin = m_end = nullptr; }

	explicit flat_handle_map(size_t count, const T& value, const Alloc& alloc = Alloc()) : flat_handle_map(alloc) { assign_impl(count, value); }

	template <class InputIterator, typename = decltype(*std::declval<InputIterator>())>
	flat_handle_map(InputIterator first, InputIterator last, const Alloc& alloc = Alloc()) : flat_handle_map(alloc)
	{
		assign_impl(first, last);
	}

	flat_handle_map(std::initializer_list<T> l, const Alloc& alloc = Alloc()) : flat_handle_map(alloc) { assign_impl(l); }

	flat_handle_map(const flat_handle_map& v) :
		flat_handle_map(v, std::allocator_traits<Alloc>::select_on_container_copy_construction(v.get_allocator()))
	{}

	flat_handle_map(const flat_handle_map& v, const Alloc& alloc) : flat_handle_map(alloc) { assign_impl(v); }

	flat_handle_map(flat_handle_map&& v) :
		m_capacity(v.m_capacity), m_alloc(std::move(v.m_alloc)), m_begin(v.m_begin), m_end(v.m_end), m_occupied(std::move(v.m_occupied)),
		m_first_available(v.m_first_available)
	{
		v.m_begin = v.m_end = nullptr;
		v.m_capacity = 0;
		v.m_occupied.clear();
		v.m_first_available = invalid_index;
	}

	~flat_handle_map()
	{
		clear();

		if (m_begin)
		{
			AllocatorTraits::deallocate(m_alloc, m_begin, m_capacity);
		}
	}

	flat_handle_map& operator=(const flat_handle_map& v)
	{
		if (this == &v)
		{
			// prevent self usurp
			return *this;
		}

		clear();

		assign_impl(v);

		return *this;
	}

	flat_handle_map& operator=(flat_handle_map&& v)
	{
		clear();

		m_occupied = std::move(v.m_occupied);
		m_first_available = v.m_first_available;

		m_alloc = std::move(v.m_alloc);
		m_capacity = v.m_capacity;
		m_begin = v.m_begin;
		m_end = v.m_end;

		v.m_begin = v.m_end = nullptr;
		v.m_capacity = 0;
		v.m_occupied.clear();
		v.m_first_available = invalid_index;

		return *this;
	}

	allocator_type get_allocator() const noexcept { return m_alloc; }

	const_reference at(handle h) const noexcept { return *(m_begin + static_cast<size_t>(h)); }
	reference at(handle h) noexcept { return *(m_begin + static_cast<size_t>(h)); }
	const_reference operator[](handle h) const noexcept { return at(h); }
	reference operator[](handle h) noexcept { return at(h); }

	const_reference front() const noexcept { return at(handle{get_next_occupied(0)}); }
	reference front() noexcept { return at(handle{get_next_occupied(0)}); }
	const_reference back() const noexcept { return at(handle{get_previous_occupied(m_occupied.size() - 1)}); }
	reference back() noexcept { return at(handle{get_previous_occupied(m_occupied.size() - 1)}); }

	const_pointer data() const noexcept { return m_begin; }
	pointer data() noexcept { return m_begin; }

	// iterators
	iterator begin() noexcept { return iterator(*this, get_next_occupied(0)); }
	const_iterator begin() const noexcept { return const_iterator(*this, get_next_occupied(0)); }
	const_iterator cbegin() const noexcept { return const_iterator(*this, get_next_occupied(0)); }

	iterator end() noexcept { return iterator(*this, invalid_index); }
	const_iterator end() const noexcept { return const_iterator(*this, invalid_index); }
	const_iterator cend() const noexcept { return const_iterator(*this, invalid_index); }

	iterator iterate(handle h) noexcept { return iterator(*this, static_cast<size_t>(h)); }
	const_iterator iterate(handle h) const noexcept { return const_iterator(*this, static_cast<size_t>(h)); }

	// capacity
	bool empty() const noexcept { return m_begin == m_end || size() == 0; }
	size_t size() const noexcept { return std::count(m_occupied.begin(), m_occupied.end(), true); }
	size_t container_size() const noexcept { return m_end - m_begin; }

	void reserve(size_t desired_capacity)
	{
		if (desired_capacity <= m_capacity)
		{
			return;
		}

		m_occupied.reserve(desired_capacity);

		size_type new_cap = find_capacity(desired_capacity);

		auto new_buf = AllocatorTraits::allocate(m_alloc, new_cap);
		const auto s = container_size();

		// now we need to transfer the existing elements into the new buffer
		for (size_type i = 0; i < s; ++i)
		{
			if (m_occupied[i])
				AllocatorTraits::construct(m_alloc, new_buf + i, std::move(*(m_begin + i)));
		}

		if (m_begin)
		{
			// free old elements
			for (size_type i = 0; i < s; ++i)
			{
				if (m_occupied[i])
					AllocatorTraits::destroy(m_alloc, m_begin + i);
			}

			AllocatorTraits::deallocate(m_alloc, m_begin, m_capacity);
		}

		m_begin = new_buf;
		m_end = new_buf + s;
		m_capacity = new_cap;
	}

	size_t capacity() const noexcept { return m_capacity; }

	// modifiers
	void clear() noexcept
	{
		for (size_t i = 0; i < m_occupied.size(); i++)
		{
			if (m_occupied[i])
				AllocatorTraits::destroy(m_alloc, m_begin + i);
		}
		m_occupied.clear();
		m_first_available = invalid_index;
		m_end = m_begin;
	}

	handle insert(const value_type& val)
	{
		handle result = static_cast<handle>(invalid_index);
		if (m_first_available >= container_size())
		{
			result = static_cast<handle>(container_size());
			auto pos = grow_end(1);
			AllocatorTraits::construct(m_alloc, pos, val);
			m_occupied.push_back(true);
			m_first_available = invalid_index;
		}
		else
		{
			result = static_cast<handle>(m_first_available);
			AllocatorTraits::construct(m_alloc, m_begin + m_first_available, val);
			m_occupied[m_first_available] = true;
			m_first_available = get_next_available(m_first_available + 1);
		}
		return result;
	}

	handle insert(value_type&& val)
	{
		handle result{invalid_index};
		if (m_first_available >= container_size())
		{
			result = static_cast<handle>(container_size());
			auto pos = grow_end(1);
			AllocatorTraits::construct(m_alloc, pos, std::move(val));
			m_occupied.push_back(true);
			m_first_available = invalid_index;
		}
		else
		{
			result = static_cast<handle>(m_first_available);
			AllocatorTraits::construct(m_alloc, m_begin + m_first_available, std::move(val));
			m_occupied[m_first_available] = true;
			m_first_available = get_next_available(m_first_available + 1);
		}
		return result;
	}

	void insert(size_type count, const value_type& val)
	{
		reserve(size() + count);
		for (size_type i = 0; i < count; ++i)
			insert(val);
	}

	template <typename InputIterator, typename = decltype(*std::declval<InputIterator>())>
	void insert(InputIterator first, InputIterator last)
	{
		reserve(size() + last - first);
		for (auto p = first; p != last; ++p)
			insert(*p);
	}

	void insert(std::initializer_list<T> ilist)
	{
		reserve(size() + ilist.size());
		for (auto& elem : ilist)
			insert(elem);
	}

	template <typename... Args>
	handle emplace(Args&&... args)
	{
		handle result{invalid_index};
		if (m_first_available >= container_size())
		{
			result = static_cast<handle>(container_size());
			auto pos = grow_end(1);
			AllocatorTraits::construct(m_alloc, pos, std::forward<Args>(args)...);
			m_occupied.push_back(true);
			m_first_available = invalid_index;
		}
		else
		{
			result = static_cast<handle>(m_first_available);
			AllocatorTraits::construct(m_alloc, m_begin + m_first_available, std::forward<Args>(args)...);
			m_occupied[m_first_available] = true;
			m_first_available = get_next_available(m_first_available + 1);
		}
		return result;
	}

	iterator erase(handle h)
	{
		auto i = static_cast<size_t>(h);
		assert(i < m_occupied.size() && m_occupied[i]);
		AllocatorTraits::destroy(m_alloc, m_begin + i);
		m_occupied[i] = false;
		if (i < m_first_available)
			m_first_available = i;
		return iterator(*this, get_next_occupied(i + 1));
	}
	iterator erase(const_iterator it) { return erase(it.get_handle()); }

private:
	size_t get_next_available(size_t i) const noexcept
	{
		for (size_t j = i; j < m_occupied.size(); j++)
			if (!m_occupied[j])
				return j;
		return invalid_index;
	}
	size_t get_next_occupied(size_t i) const noexcept
	{
		for (size_t j = i; j < m_occupied.size(); j++)
			if (m_occupied[j])
				return j;
		return invalid_index;
	}
	size_t get_previous_occupied(size_t i) const noexcept
	{
		static_assert(size_t(0) < size_t(0) - 1, "size_t must underflow when decrementing from zero");
		for (size_t j = i; j < m_occupied.size(); j--)
			if (m_occupied[j])
				return j;
		return invalid_index;
	}

	size_type find_capacity(size_type desired_capacity) const
	{
		if (m_capacity == 0)
		{
			return desired_capacity;
		}
		else
		{
			auto new_cap = m_capacity;

			while (new_cap < desired_capacity)
			{
				// grow by roughly 1.5
				new_cap *= 3;
				++new_cap;
				new_cap /= 2;
			}

			return new_cap;
		}
	}

	T* grow_end(size_t num)
	{
		const auto s = container_size();
		reserve(s + num);
		m_end = m_begin + s + num;
		return m_begin + s;
	}

	// grows buffer only on empty
	void grow_empty(size_t capacity)
	{
		assert(m_begin == m_end);

		if (capacity <= m_capacity)
			return;

		if (m_begin)
		{
			AllocatorTraits::deallocate(m_alloc, m_begin, m_capacity);
		}

		m_capacity = find_capacity(capacity);
		m_begin = m_end = AllocatorTraits::allocate(m_alloc, m_capacity);
	}

	void assign_impl(size_type count, const T& value)
	{
		grow_empty(count);

		for (size_type i = 0; i < count; ++i)
		{
			insert(value);
		}
	}

	template <class InputIterator>
	void assign_impl(InputIterator first, InputIterator last)
	{
		auto isize = last - first;
		grow_empty(isize);

		for (auto p = first; p != last; ++p)
		{
			insert(*p);
		}
	}

	void assign_impl(std::initializer_list<T> ilist)
	{
		grow_empty(ilist.size());

		for (auto& elem : ilist)
		{
			insert(elem);
		}
	}

	void assign_impl(const flat_handle_map& v)
	{
		grow_empty(v.container_size());

		m_occupied = v.m_occupied;
		m_first_available = v.m_first_available;

		for (size_t i = 0; i < v.container_size(); i++)
			if (m_occupied[i])
				AllocatorTraits::construct(m_alloc, m_begin + i, *(v.m_begin + i));

		m_end = m_begin + v.container_size();
	}

	template <typename T, class Alloc>
	friend bool operator==(const flat_handle_map<T, Alloc>& a, const flat_handle_map<T, Alloc>& b);

	pointer m_begin;
	pointer m_end;

	size_t m_capacity;

	Alloc m_alloc;

	std::vector<bool> m_occupied;
	size_t m_first_available = invalid_index;

	static constexpr size_t invalid_index = size_t(-1);
};

template <typename T, class Alloc>
bool operator==(const flat_handle_map<T, Alloc>& a, const flat_handle_map<T, Alloc>& b)
{
	using handle = typename flat_handle_map<T, Alloc>::handle;

	if (a.size() != b.size())
	{
		return false;
	}

	for (size_t i = 0; i < a.m_occupied.size(); ++i)
	{
		auto h = static_cast<handle>(i);
		if (a.m_occupied[i] != b.m_occupied[i])
			return false;
		if (a.m_occupied[i] && (a[h] != b[h]))
			return false;
	}

	return true;
}

template <typename T, class Alloc>
bool operator!=(const flat_handle_map<T, Alloc>& a, const flat_handle_map<T, Alloc>& b)
{
	return !(a == b);
}

#ifdef HANDLEMAP_TEST

void test_stable_vector()
{
	flat_handle_map<string> m;
	flat_handle_map<string> m_copy1 = m;
	assert(m == m_copy1);

	auto h_hello = m.insert("hello");
	auto h_carl = m.insert("carl");
	auto h_wood = m.insert("wood");
	auto h_space = m.insert(" ");
	m.insert({"red", "green", "blue", "alpha", "none"});
	auto h_long = m.insert(
		"A REAAAALLY long string.........................................................:::::::::::::::::::::::...................................."s);
	auto h_world = m.insert("world");

	m.erase(h_long);
	m.insert("short");

	m.insert({"s", "p", "a", "m"});
	string s = "s";
	m.insert(s);
	m.emplace("p");
	m.insert("a");
	m.emplace("m");

	for (auto it = m.iterate(h_carl); it != m.end(); ++it)
	{
		if (*it == "blue")
			m.erase(it);
	}

	m.erase(h_carl);
	auto h_jane = m.insert("jane");
	auto h_black = m.insert("black");
	m.erase(h_wood);

	auto ss = MakeString();
	for (auto it = m.begin(); it != m.end(); ++it)
	{
		ss << *it << ", ";
	}
	string str = (string)ss;
	auto str2 = m[h_hello] + m[h_space] + m[h_world];

	assert(str == "hello, jane,  , red, green, black, alpha, none, short, world, s, p, a, m, s, p, a, m, ");
	assert(str2 == "hello world");

	flat_handle_map<string> m_copy2{m};
	assert(m == m_copy2);
	assert(m_copy1 != m_copy2);
}

#endif