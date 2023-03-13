// itlib-flat-set v1.06
//
// std::set-like class with an underlying vector
//
// SPDX-License-Identifier: MIT
// MIT License:
// Copyright(c) 2021-2023 Borislav Stanimirov
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files(the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and / or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions :
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//
//                  VERSION HISTORY
//
//  1.06 (2023-01-14) Fixed initialization with custom Compare when equivalence
//                    is not the same as `==`.
//                    Inherit from Compare to enable empty base optimization
//  1.05 (2022-09-17) upper_bound and equal_range
//  1.04 (2022-06-23) Transparent lookups (C++14 style)
//  1.03 (2022-04-14) Noxcept move construct and assign
//  1.02 (2021-09-28) Fixed construction from std::initializer_list which
//                    allowed duplicate elements to find their wey in the set
//  1.01 (2021-09-15) Constructors from std::initializer_list
//  1.00 (2021-08-10) Initial-release
//
//
//                  DOCUMENTATION
//
// Simply include this file wherever you need.
// It defines the class itlib::flat_set, which is an almsot drop-in replacement
// of std::set. Flat set has an optional underlying container which by default
// is std::vector. Thus the items in the set are in a continuous block of
// memory. Thus iterating over the set is cache friendly, at the cost of
// O(n) for insert and erase.
//
// The elements inside (like in std::set) are kept in an order sorted by key.
// Getting a value by key is O(log2 n)
//
// It generally performs much faster than std::set for smaller sets of elements
//
// The difference with std::set, which makes flat_set an not-exactly-drop-in
// replacement is the last template argument:
// * std::set has <key, compare, allocator>
// * itlib::flat_set has <key, compare, container>
// The container must be an std::vector compatible type (itlib::static_vector
// is, for example, viable). The container value type must be `key`
//
//                  Changing the allocator.
//
// If you want to change the allocator of flat set, you'll have to provide a
// container with the appropriate one. Example:
//
// itlib::flat_set<
//      string,
//      less<string>,
//      std::vector<string, MyAllocator<string>>
//  > myset
//
//
//                  TESTS
//
// You can find unit tests for static_vector in its official repo:
// https://github.com/iboB/itlib/blob/master/test/
//
#pragma once

#include <vector>
#include <algorithm>
#include <type_traits>

namespace itlib
{

namespace fsimpl
{
struct less // so as not to clash with map_less
{
    template <typename T, typename U>
    auto operator()(const T& t, const U& u) const -> decltype(t < u)
    {
        return t < u;
    }
};
}

template <typename Key, typename Compare = fsimpl::less, typename Container = std::vector<Key>>
class flat_set : private /*EBO*/ Compare
{
    Container m_container;
    Compare& cmp() { return *this; }
    const Compare& cmp() const { return *this; }
public:
    using key_type = Key;
    using value_type = Key;
    using container_type = Container;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type& ;
    using allocator_type = typename container_type::allocator_type;
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::pointer;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;
    using difference_type = typename container_type::difference_type;
    using size_type = typename container_type::size_type;

    flat_set()
    {}

    explicit flat_set(const key_compare& comp, const allocator_type& alloc = allocator_type())
        : Compare(comp)
        , m_container(alloc)
    {}

    explicit flat_set(container_type container, const key_compare& comp = key_compare())
        : Compare(comp)
        , m_container(std::move(container))
    {
        std::sort(m_container.begin(), m_container.end(), cmp());
        auto new_end = std::unique(m_container.begin(), m_container.end(), [this](const value_type& a, const value_type& b) -> bool {
            return !cmp()(a, b) && !cmp()(b, a);
        });
        m_container.erase(new_end, m_container.end());
    }

    flat_set(std::initializer_list<value_type> init, const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type())
        : flat_set(container_type(std::move(init), alloc), comp)
    {}

    flat_set(std::initializer_list<value_type> init, const allocator_type& alloc)
        : flat_set(std::move(init), key_compare(), alloc)
    {}

    flat_set(const flat_set& x) = default;
    flat_set& operator=(const flat_set& x) = default;

    flat_set(flat_set&& x) noexcept = default;
    flat_set& operator=(flat_set&& x) noexcept = default;

    key_compare key_comp() const { return *this; }

    iterator begin() noexcept { return m_container.begin(); }
    const_iterator begin() const noexcept { return m_container.begin(); }
    iterator end() noexcept { return m_container.end(); }
    const_iterator end() const noexcept { return m_container.end(); }
    reverse_iterator rbegin() noexcept { return m_container.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return m_container.rbegin(); }
    reverse_iterator rend() noexcept { return m_container.rend(); }
    const_reverse_iterator rend() const noexcept { return m_container.rend(); }
    const_iterator cbegin() const noexcept { return m_container.cbegin(); }
    const_iterator cend() const noexcept { return m_container.cend(); }

    bool empty() const noexcept { return m_container.empty(); }
    size_type size() const noexcept { return m_container.size(); }
    size_type max_size() const noexcept { return m_container.max_size(); }

    void reserve(size_type count) { return m_container.reserve(count); }
    size_type capacity() const noexcept { return m_container.capacity(); }

    void clear() noexcept { m_container.clear(); }

    template <typename F>
    iterator lower_bound(const F& k)
    {
        return std::lower_bound(m_container.begin(), m_container.end(), k, cmp());
    }

    template <typename F>
    const_iterator lower_bound(const F& k) const
    {
        return std::lower_bound(m_container.begin(), m_container.end(), k, cmp());
    }

    template <typename K>
    iterator upper_bound(const K& k)
    {
        return std::upper_bound(m_container.begin(), m_container.end(), k, cmp());
    }

    template <typename K>
    const_iterator upper_bound(const K& k) const
    {
        return std::upper_bound(m_container.begin(), m_container.end(), k, cmp());
    }

    template <typename K>
    std::pair<iterator, iterator> equal_range(const K& k)
    {
        return std::equal_range(m_container.begin(), m_container.end(), k, cmp());
    }

    template <typename K>
    std::pair<const_iterator, const_iterator> equal_range(const K& k) const
    {
        return std::equal_range(m_container.begin(), m_container.end(), k, cmp());
    }

    template <typename F>
    iterator find(const F& k)
    {
        auto i = lower_bound(k);
        if (i != end() && !cmp()(k, *i))
            return i;

        return end();
    }

    template <typename F>
    const_iterator find(const F& k) const
    {
        auto i = lower_bound(k);
        if (i != end() && !cmp()(k, *i))
            return i;

        return end();
    }

    template <typename F>
    size_t count(const F& k) const
    {
        return find(k) == end() ? 0 : 1;
    }

    template <typename P>
    std::pair<iterator, bool> insert(P&& val)
    {
        auto i = lower_bound(val);
        if (i != end() && !cmp()(val, *i))
        {
            return { i, false };
        }

        return{ m_container.emplace(i, std::forward<P>(val)), true };
    }

    std::pair<iterator, bool> insert(const value_type& val)
    {
        auto i = lower_bound(val);
        if (i != end() && !cmp()(val, *i))
        {
            return { i, false };
        }

        return{ m_container.emplace(i, val), true };
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        value_type val(std::forward<Args>(args)...);
        return insert(std::move(val));
    }

    iterator erase(const_iterator pos)
    {
        return m_container.erase(pos);
    }

    iterator erase(iterator pos)
    {
        return erase(const_iterator(pos));
    }

    template <typename F>
    size_type erase(const F& k)
    {
        auto i = find(k);
        if (i == end())
        {
            return 0;
        }

        erase(i);
        return 1;
    }

    void swap(flat_set& x)
    {
        std::swap(cmp(), x.cmp());
        m_container.swap(x.m_container);
    }

    const container_type& container() const noexcept
    {
        return m_container;
    }

    // DANGER! If you're not careful with this function, you may irreversably break the set
    container_type& modify_container() noexcept
    {
        return m_container;
    }
};

template <typename Key, typename Compare, typename Container>
bool operator==(const flat_set<Key, Compare, Container>& a, const flat_set<Key, Compare, Container>& b)
{
    return a.container() == b.container();
}

template <typename Key, typename Compare, typename Container>
bool operator!=(const flat_set<Key, Compare, Container>& a, const flat_set<Key, Compare, Container>& b)
{
    return a.container() != b.container();
}

}
