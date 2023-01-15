// itlib-flat-map v1.07
//
// std::map-like class with an underlying vector
//
// SPDX-License-Identifier: MIT
// MIT License:
// Copyright(c) 2016-2019 Chobolabs Inc.
// Copyright(c) 2020-2023 Borislav Stanimirov
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
//  1.07 (2023-01-14) Inherit from Compare to enable empty base optimization
//  1.06 (2023-01-09) Fixed transparency for std::string_view
//  1.05 (2022-09-17) upper_bound and equal_range
//  1.04 (2022-07-07) Transparent lookups (C++14 style)
//                    Transparent construction
//  1.03 (2022-04-14) Noxcept move construct and assign
//  1.02 (2021-09-28) Fixed construction from std::initializer_list which
//                    allowed duplicate keys to find their wey in the map
//  1.01 (2021-09-15) Constructors from std::initializer_list
//  1.00 (2020-10-14) Rebranded release from chobo-flat-map
//
//
//                  DOCUMENTATION
//
// Simply include this file wherever you need.
// It defines the class itlib::flat_map, which is an almsot drop-in replacement
// of std::map. Flat map has an optional underlying container which by default
// is std::vector. Thus the items in the map are in a continuous block of
// memory. Thus iterating over the map is cache friendly, at the cost of
// O(n) for insert and erase.
//
// The elements inside (like in std::map) are kept in an order sorted by key.
// Getting a value by key is O(log2 n)
//
// It generally performs much faster than std::map for smaller sets of elements
//
// The difference with std::map, which makes flat_map an not-exactly-drop-in
// replacement is the last template argument:
// * std::map has <key, value, compare, allocator>
// * itlib::flat_map has <key, value, compare, container>
// The container must be an std::vector compatible type (itlib::static_vector
// is, for example, viable). The container value type must be
// std::pair<key, value>.
//
//                  Changing the allocator.
//
// If you want to change the allocator of flat map, you'll have to provide a
// container with the appropriate one. Example:
//
// itlib::flat_map<
//      string,
//      int,
//      less<string>,
//      std::vector<pair<string, int>, MyAllocator<pair<string, int>>
//  > mymap
//
//
//                  Configuration
//
// Throw
// Whether to throw exceptions: when `at` is called with a non-existent key.
// By default, like std::map, it throws an std::out_of_range exception. If you define
// ITLIB_FLAT_MAP_NO_THROW before including this header, the exception will
// be substituted by an assertion.
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

#if !defined(ITLIB_FLAT_MAP_NO_THROW)
#   include <stdexcept>
#   define I_ITLIB_THROW_FLAT_MAP_OUT_OF_RANGE() throw std::out_of_range("itlib::flat_map out of range")
#else
#   include <cassert>
#   define I_ITLIB_THROW_FLAT_MAP_OUT_OF_RANGE() assert(false && "itlib::flat_map out of range")
#endif

namespace itlib
{

namespace fmimpl
{
struct less
{
    template <typename T, typename U>
    auto operator()(const T& t, const U& u) const -> decltype(t < u)
    {
        return t < u;
    }
};

template <typename Key, typename T, typename Compare>
struct pair_compare : public Compare
{
    using value_type = std::pair<Key, T>;
    pair_compare() = default;
    pair_compare(const Compare& kc) : Compare(kc) {}
    bool operator()(const value_type& a, const value_type& b) const { return Compare::operator()(a.first, b.first); }
    template <typename K> bool operator()(const value_type& a, const K& b) const { return Compare::operator()(a.first, b); }
    template <typename K> bool operator()(const K& a, const value_type& b) const { return Compare::operator()(a, b.first); }
};
}

template <typename Key, typename T, typename Compare = fmimpl::less, typename Container = std::vector<std::pair<Key, T>>>
class flat_map : private fmimpl::pair_compare<Key, T, Compare>
{
    Container m_container;
    using pair_compare = fmimpl::pair_compare<Key, T, Compare>;
    pair_compare& cmp() { return *this; }
    const pair_compare& cmp() const { return *this; }
public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef std::pair<Key, T> value_type;
    typedef Container container_type;
    typedef Compare key_compare;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef typename container_type::allocator_type allocator_type;
    typedef typename std::allocator_traits<allocator_type>::pointer pointer;
    typedef typename std::allocator_traits<allocator_type>::pointer const_pointer;
    typedef typename container_type::iterator iterator;
    typedef typename container_type::const_iterator const_iterator;
    typedef typename container_type::reverse_iterator reverse_iterator;
    typedef typename container_type::const_reverse_iterator const_reverse_iterator;
    typedef typename container_type::difference_type difference_type;
    typedef typename container_type::size_type size_type;

    flat_map() = default;

    explicit flat_map(const key_compare& comp, const allocator_type& alloc = allocator_type())
        : pair_compare(comp)
        , m_container(alloc)
    {}

    flat_map(std::initializer_list<value_type> init, const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type())
        : pair_compare(comp)
        , m_container(std::move(init), alloc)
    {
        std::sort(m_container.begin(), m_container.end(), cmp());
        auto new_end = std::unique(m_container.begin(), m_container.end(), [this](const value_type& a, const value_type& b) {
            return !cmp()(a, b) && !cmp()(b, a);
        });
        m_container.erase(new_end, m_container.end());
    }

    flat_map(std::initializer_list<value_type> init, const allocator_type& alloc)
        : flat_map(std::move(init), key_compare(), alloc)
    {}

    flat_map(const flat_map& x) = default;
    flat_map& operator=(const flat_map& x) = default;

    flat_map(flat_map&& x) noexcept = default;
    flat_map& operator=(flat_map&& x) noexcept = default;

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

    template <typename K>
    iterator lower_bound(const K& k)
    {
        return std::lower_bound(m_container.begin(), m_container.end(), k, cmp());
    }

    template <typename K>
    const_iterator lower_bound(const K& k) const
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

    template <typename K>
    iterator find(const K& k)
    {
        auto i = lower_bound(k);
        if (i != end() && !cmp()(k, *i))
            return i;

        return end();
    }

    template <typename K>
    const_iterator find(const K& k) const
    {
        auto i = lower_bound(k);
        if (i != end() && !cmp()(k, *i))
            return i;

        return end();
    }

    template <typename K>
    size_t count(const K& k) const
    {
        return find(k) == end() ? 0 : 1;
    }

    template <typename P>
    std::pair<iterator, bool> insert(P&& val)
    {
        auto i = lower_bound(val.first);
        if (i != end() && !cmp()(val.first, *i))
        {
            return { i, false };
        }

        return{ m_container.emplace(i, std::forward<P>(val)), true };
    }

    std::pair<iterator, bool> insert(const value_type& val)
    {
        auto i = lower_bound(val.first);
        if (i != end() && !cmp()(val.first, *i))
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
        return m_container.erase(const_iterator(pos));
    }

    template <typename K>
    size_type erase(const K& k)
    {
        auto i = find(k);
        if (i == end())
        {
            return 0;
        }

        erase(i);
        return 1;
    }

    template <typename K>
    typename std::enable_if<std::is_constructible<key_type, K>::value,
    mapped_type&>::type operator[](K&& k)
    {
        auto i = lower_bound(k);
        if (i != end() && !cmp()(k, *i))
        {
            return i->second;
        }

        i = m_container.emplace(i, std::forward<K>(k), mapped_type());
        return i->second;
    }

    mapped_type& at(const key_type& k)
    {
        auto i = lower_bound(k);
        if (i == end() || cmp()(*i, k))
        {
            I_ITLIB_THROW_FLAT_MAP_OUT_OF_RANGE();
        }

        return i->second;
    }

    const mapped_type& at(const key_type& k) const
    {
        auto i = lower_bound(k);
        if (i == end() || cmp()(*i, k))
        {
            I_ITLIB_THROW_FLAT_MAP_OUT_OF_RANGE();
        }

        return i->second;
    }

    void swap(flat_map& x)
    {
        std::swap(cmp(), x.cmp());
        m_container.swap(x.m_container);
    }

    const container_type& container() const noexcept
    {
        return m_container;
    }

    // DANGER! If you're not careful with this function, you may irreversably break the map
    container_type& modify_container() noexcept
    {
        return m_container;
    }
};

template <typename Key, typename T, typename Compare, typename Container>
bool operator==(const flat_map<Key, T, Compare, Container>& a, const flat_map<Key, T, Compare, Container>& b)
{
    return a.container() == b.container();
}

template <typename Key, typename T, typename Compare, typename Container>
bool operator!=(const flat_map<Key, T, Compare, Container>& a, const flat_map<Key, T, Compare, Container>& b)
{
    return a.container() != b.container();
}

}
