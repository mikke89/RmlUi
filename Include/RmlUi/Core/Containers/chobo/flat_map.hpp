// chobo-flat-map v1.01
//
// std::map-like class with an underlying vector
//
// MIT License:
// Copyright(c) 2016 Chobolabs Inc.
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
//  1.01 (2016-09-27) Fix for keys with no operator==. Clean up of assignment.
//                    Added swap method.
//  1.00 (2016-09-23) First public release
//
//
//                  DOCUMENTATION
//
// Simply include this file wherever you need.
// It defines the class chobo::flat_map, which is an almsot drop-in replacement
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
// * chobo::flat_map has <key, value, compare, container>
// The container must be an std::vector compatible type (chobo::static_vector
// and chobo::vector_ptr are, for example, viable). The container value type
// must be std::pair<key, value>.
//
//                  Changing the allocator.
//
// If you want to change the allocator of flat map, you'll have to provide a
// container with the appriate one. Example:
//
// chobo::flat_map<
//      string,
//      int,
//      less<string>,
//      std::vector<pair<string, int>, MyAllocator<pair<string, int>>
//  > mymap
//
//
//                  Configuration
//
// chobo::flat_map has two configurable settings:
//
// 1. Throw
// Whether to throw exceptions: when `at` is called with a non-existent key.
// By default, like std::map, it throws an std::out_of_range exception. If you define
// CHOBO_FLAT_MAP_NO_THROW before including this header, the exception will
// be substituted by an assertion.
//
// 2. const char* overloads
// By default chobo::flat_map provides overloads for the access methods
// (at, operator[], find, lower_bound, count) for const char* for cases when
// std::string is the key, so that no allocations happen when accessing with
// a C-string of a string literal.
// However if const char* or any other class with implicit conversion from
// const char* is the key, they won't compile.
// If you plan on using flat_map with such keys, you'll need to define
// CHOBO_FLAT_MAP_NO_CONST_CHAR_OVERLOADS before including the header
//
//
//                  TESTS
//
// The tests are included in the header file and use doctest (https://github.com/onqtam/doctest).
// To run them, define CHOBO_FLAT_MAP_TEST_WITH_DOCTEST before including
// the header in a file which has doctest.h already included.
//
// Additionally if chobo::static_vector is also available you may define
// CHOBO_FLAT_MAP_TEST_STATIC_VECTOR_WITH_DOCTEST to test flat_map with an
// unrelying static_vector
//
// Additionally if chobo::vector_ptr is also available you may define
// CHOBO_FLAT_MAP_TEST_VECTOR_PTR_WITH_DOCTEST to test flat_map with an
// unrelying vector_ptr
//
#pragma once

#include <vector>
#include <algorithm>
#include <type_traits>

#if !defined(CHOBO_FLAT_MAP_NO_CONST_CHAR_OVERLOADS)
#include <cstring>
#endif

#if !defined(CHOBO_FLAT_MAP_NO_THROW)
#   include <stdexcept>
#   define _CHOBO_THROW_FLAT_MAP_OUT_OF_RANGE() throw std::out_of_range("chobo::flat_map out of range")
#else
#   include <cassert>
#   define _CHOBO_THROW_FLAT_MAP_OUT_OF_RANGE() assert(false && "chobo::flat_map out of range")
#endif

namespace chobo
{

template <typename Key, typename T, typename Compare = std::less<Key>, typename Container = std::vector<std::pair<Key, T>>>
class flat_map
{
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

    flat_map()
    {}

    explicit flat_map(const key_compare& comp, const allocator_type& alloc = allocator_type())
        : m_cmp(comp)
        , m_container(alloc)
    {}

    flat_map(const flat_map& x) = default;
    flat_map(flat_map&& x) = default;

    flat_map(std::initializer_list<value_type> ilist) : m_cmp(Compare())
    {
        m_container.reserve(ilist.size());
        for (auto&& il : ilist)
            emplace(il);
    }

    flat_map& operator=(const flat_map& x)
    {
        m_cmp = x.m_cmp;
        m_container = x.m_container;
        return *this;
    }
    flat_map& operator=(flat_map&& x)
    {
        m_cmp = std::move(x.m_cmp);
        m_container = std::move(x.m_container);
        return *this;
    }

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

    iterator lower_bound(const key_type& k)
    {
        return std::lower_bound(m_container.begin(), m_container.end(), k, m_cmp);
    }

    const_iterator lower_bound(const key_type& k) const
    {
        return std::lower_bound(m_container.begin(), m_container.end(), k, m_cmp);
    }

    iterator find(const key_type& k)
    {
        auto i = lower_bound(k);
        if (i != end() && !m_cmp(k, *i))
            return i;

        return end();
    }

    const_iterator find(const key_type& k) const
    {
        auto i = lower_bound(k);
        if (i != end() && !m_cmp(k, *i))
            return i;

        return end();
    }

    size_t count(const key_type& k) const
    {
        return find(k) == end() ? 0 : 1;
    }

    template <typename P>
    std::pair<iterator, bool> insert(P&& val)
    {
        auto i = lower_bound(val.first);
        if (i != end() && !m_cmp(val.first, *i))
        {
            return { i, false };
        }

        return{ m_container.emplace(i, std::forward<P>(val)), true };
    }

    std::pair<iterator, bool> insert(const value_type& val)
    {
        auto i = lower_bound(val.first);
        if (i != end() && !m_cmp(val.first, *i))
        {
            return { i, false };
        }

        return{ m_container.emplace(i, val), true };
    }

    template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        value_type val(args...);
        return insert(std::move(val));
    }

    iterator erase(const_iterator pos)
    {
        return m_container.erase(pos);
    }

    size_type erase(const key_type& k)
    {
        auto i = find(k);
        if (i == end())
        {
            return 0;
        }

        erase(i);
        return 1;
    }

    mapped_type& operator[](const key_type& k)
    {
        auto i = lower_bound(k);
        if (i != end() && !m_cmp(k, *i))
        {
            return i->second;
        }

        i = m_container.emplace(i, k, mapped_type());
        return i->second;
    }

    mapped_type& operator[](key_type&& k)
    {
        auto i = lower_bound(k);
        if (i != end() && !m_cmp(k, *i))
        {
            return i->second;
        }

        i = m_container.emplace(i, std::forward<key_type>(k), mapped_type());
        return i->second;
    }

    mapped_type& at(const key_type& k)
    {
        auto i = lower_bound(k);
        if (i == end() || m_cmp(*i, k))
        {
            _CHOBO_THROW_FLAT_MAP_OUT_OF_RANGE();
        }

        return i->second;
    }

    const mapped_type& at(const key_type& k) const
    {
        auto i = lower_bound(k);
        if (i == end() || m_cmp(*i, k))
        {
            _CHOBO_THROW_FLAT_MAP_OUT_OF_RANGE();
        }

        return i->second;
    }

    void swap(flat_map& x)
    {
        std::swap(m_cmp, x.m_cmp);
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

#if !defined(CHOBO_FLAT_MAP_NO_CONST_CHAR_OVERLOADS)
    ///////////////////////////////////////////////////////////////////////////////////
    // const char* overloads for maps with an std::string key to avoid allocs
    iterator lower_bound(const char* k)
    {
        static_assert(std::is_same<std::string, key_type>::value, "flat_map::lower_bound(const char*) works only for std::strings");
        static_assert(std::is_same<std::less<std::string>, key_compare>::value, "flat_map::lower_bound(const char*) works only for std::string-s, compared with std::less<std::string>");
        return std::lower_bound(m_container.begin(), m_container.end(), k, [](const value_type& a, const char* b) -> bool
        {
            return strcmp(a.first.c_str(), b) < 0;
        });
    }

    const_iterator lower_bound(const char* k) const
    {
        static_assert(std::is_same<std::string, key_type>::value, "flat_map::lower_bound(const char*) works only for std::strings");
        static_assert(std::is_same<std::less<std::string>, key_compare>::value, "flat_map::lower_bound(const char*) works only for std::string-s, compared with std::less<std::string>");
        return std::lower_bound(m_container.begin(), m_container.end(), k, [](const value_type& a, const char* b) -> bool
        {
            return strcmp(a.first.c_str(), b) < 0;
        });
    }

    mapped_type& operator[](const char* k)
    {
        auto i = lower_bound(k);
        if (i != end() && i->first == k)
        {
            return i->second;
        }

        i = m_container.emplace(i, k, mapped_type());
        return i->second;
    }

    mapped_type& at(const char* k)
    {
        auto i = lower_bound(k);
        if (i == end() || i->first != k)
        {
            _CHOBO_THROW_FLAT_MAP_OUT_OF_RANGE();
        }

        return i->second;
    }

    const mapped_type& at(const char* k) const
    {
        auto i = lower_bound(k);
        if (i == end() || i->first != k)
        {
            _CHOBO_THROW_FLAT_MAP_OUT_OF_RANGE();
        }

        return i->second;
    }

    iterator find(const char* k)
    {
        auto i = lower_bound(k);
        if (i != end() && i->first == k)
            return i;

        return end();
    }

    const_iterator find(const char* k) const
    {
        auto i = lower_bound(k);
        if (i != end() && i->first == k)
            return i;

        return end();
    }

    size_t count(const char* k) const
    {
        return find(k) == end() ? 0 : 1;
    }

#endif // !defined(CHOBO_FLAT_MAP_NO_CONST_CHAR_OVERLOADS)

private:
    struct pair_compare
    {
        pair_compare() = default;
        pair_compare(const key_compare& kc) : kcmp(kc) {}
        bool operator()(const value_type& a, const key_type& b) const
        {
            return kcmp(a.first, b);
        }

        bool operator()(const key_type& a, const value_type& b) const
        {
            return kcmp(a, b.first);
        }

        key_compare kcmp;
    };
    pair_compare m_cmp;
    container_type m_container;
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
template <typename Key, typename T, typename Compare, typename Container>
bool operator<(const flat_map<Key, T, Compare, Container>& a, const flat_map<Key, T, Compare, Container>& b)
{
	return a.container() < b.container();
}

}

#if defined(CHOBO_FLAT_MAP_TEST_WITH_DOCTEST)

#include <string>

namespace chobo_flat_map_test
{

// struct with no operator==
struct int_wrap
{
    int_wrap() = default;
    int_wrap(int i) : val(i) {}
    int val;

    struct compare
    {
        bool operator()(const int_wrap& a, const int_wrap& b) const
        {
            return a.val < b.val;
        }
    };
};

}

TEST_CASE("[flat_map] test")
{
    using namespace chobo;
    using namespace chobo_flat_map_test;

    flat_map<int, float> ifmap;
    CHECK(ifmap.empty());
    CHECK(ifmap.size() == 0);
    CHECK(ifmap.capacity() == 0);
    CHECK(ifmap.begin() == ifmap.end());

    ifmap[1] = 3.2f;
    CHECK(ifmap.size() == 1);

    auto ifit = ifmap.begin();
    CHECK(ifit->first == 1);
    CHECK(ifit->second == 3.2f);
    CHECK(ifmap[1] == 3.2f);
    CHECK(ifmap.at(1) == 3.2f);
    CHECK(ifmap.count(1) == 1);
    CHECK(ifmap.count(5) == 0);

    ++ifit;
    CHECK(ifit == ifmap.end());

    auto res = ifmap.insert(std::make_pair(6, 3.14f));
    CHECK(res.second);
    CHECK(res.first == ifmap.begin() + 1);

    res = ifmap.emplace(3, 5.5f);
    CHECK(res.second);
    CHECK(res.first == ifmap.begin() + 1);

    res = ifmap.emplace(6, 8.f);
    CHECK(!res.second);
    CHECK(res.first == ifmap.begin() + 2);

    ifmap[2] = 5;
    ifmap[52] = 15;
    ifmap[12] = 1;
    CHECK(ifmap.size() == 6);

    auto cmp = [](const flat_map<int, float>::value_type& a, const flat_map<int, float>::value_type& b) -> bool
    {
        return a.first < b.first;
    };

    CHECK(std::is_sorted(ifmap.begin(), ifmap.end(), cmp));

    ifmap.erase(12);
    CHECK(ifmap.size() == 5);

    CHECK(std::is_sorted(ifmap.begin(), ifmap.end(), cmp));

    ifit = ifmap.find(12);
    CHECK(ifit == ifmap.end());

    ifit = ifmap.find(6);
    CHECK(ifit != ifmap.end());
    ifmap.erase(ifit);

    CHECK(ifmap.size() == 4);
    CHECK(std::is_sorted(ifmap.begin(), ifmap.end(), cmp));
    ifit = ifmap.find(6);
    CHECK(ifit == ifmap.end());

    //

    flat_map<std::string, int> simap;

    CHECK(simap["123"] == 0);

    CHECK(simap.begin()->first.c_str() == "123");

    ++simap["asd"];

    auto siit = simap.find("asd");
    CHECK(siit != simap.end());
    CHECK(siit->second == 1);
    CHECK(siit == simap.begin() + 1);

    CHECK(simap.count("bababa") == 0);
    CHECK(simap.count("asd") == 1);

    std::string asd = "asd";
    CHECK(simap.at(asd) == simap.at("asd"));

    simap["0The quick brown fox jumps over the lazy dog"] = 555;
    CHECK(simap.begin()->first[1] == 'T');
    const void* cstr = simap.begin()->first.c_str();

    auto simap2 = std::move(simap);
    CHECK(simap.empty());
    CHECK(simap2.begin()->first.c_str() == cstr);

    simap = std::move(simap2);
    CHECK(simap2.empty());
    CHECK(simap.begin()->first.c_str() == cstr);

    CHECK(simap2 != simap);
    simap2 = simap;
    CHECK(simap2 == simap);

    // no == comparable tests
    flat_map<int_wrap, int, int_wrap::compare> iwmap;
    iwmap[5] = 1;
    iwmap[20] = 15;
    iwmap[10] = 5;

    auto iwi = iwmap.emplace(3, 4);
    CHECK(iwi.second == true);
    CHECK(iwi.first == iwmap.begin());

    CHECK(iwmap.begin()->first.val == 3);
    CHECK(iwmap.begin()->second == 4);
    CHECK(iwmap.rbegin()->first.val == 20);
    CHECK(iwmap.rbegin()->second == 15);
    CHECK(iwmap.at(10) == 5);

    iwi = iwmap.insert(std::pair<int_wrap, int>(11, 6));
    CHECK(iwi.second == true);
    CHECK(iwi.first + 2 == iwmap.end());

    CHECK(iwmap[11] == 6);

    iwi = iwmap.emplace(10, 55);
    CHECK(iwi.second == false);
    CHECK(iwi.first->second == 5);

    CHECK(iwmap.find(18) == iwmap.end());
    CHECK(iwmap.find(11) != iwmap.end());

    const auto ciwmap = iwmap;

    CHECK(ciwmap.begin()->first.val == 3);
    CHECK(ciwmap.begin()->second == 4);
    CHECK(ciwmap.rbegin()->first.val == 20);
    CHECK(ciwmap.rbegin()->second == 15);
    CHECK(ciwmap.at(10) == 5);

    CHECK(ciwmap.find(18) == ciwmap.end());
    CHECK(ciwmap.find(11) != ciwmap.end());

    // swap
    flat_map<int, int> m1, m2;
    m1.reserve(10);
    m1[1] = 2;
    m1[2] = 5;
    auto m1c = m1.capacity();

    CHECK(m2.capacity() == 0);
    m1.swap(m2);

    CHECK(m2.size() == 2);
    CHECK(m2.capacity() == m1c);
    CHECK(m1.capacity() == 0);

    // self usurp
    m2 = m2;
    CHECK(m2.size() == 2);
    CHECK(m2.capacity() == m1c);
}

#if defined(CHOBO_FLAT_MAP_TEST_STATIC_VECTOR_WITH_DOCTEST)

TEST_CASE("[flat_map] static_vector test")
{
    using namespace chobo;

    flat_map<int, char, std::less<int>, static_vector<std::pair<int, char>, 10>> smap;
    CHECK(smap.empty());
    CHECK(smap.size() == 0);
    CHECK(smap.capacity() == 10);
    CHECK(smap.begin() == smap.end());

    smap[1] = 3;
    CHECK(smap.size() == 1);

    auto ifit = smap.begin();
    CHECK(ifit->first == 1);
    CHECK(ifit->second == 3);
    CHECK(smap[1] == 3);
    CHECK(smap.at(1) == 3);
    CHECK(smap.count(1) == 1);
    CHECK(smap.count(5) == 0);

    ++ifit;
    CHECK(ifit == smap.end());

    auto res = smap.insert(std::make_pair(6, 3));
    CHECK(res.second);
    CHECK(res.first == smap.begin() + 1);

    res = smap.emplace(3, 5);
    CHECK(res.second);
    CHECK(res.first == smap.begin() + 1);

    res = smap.emplace(6, 8);
    CHECK(!res.second);
    CHECK(res.first == smap.begin() + 2);

    smap[2] = 5;
    smap[52] = 15;
    smap[12] = 1;
    CHECK(smap.size() == 6);

    auto cmp = [](const flat_map<int, float>::value_type& a, const flat_map<int, float>::value_type& b) -> bool
    {
        return a.first < b.first;
    };

    CHECK(std::is_sorted(smap.begin(), smap.end(), cmp));

    smap.erase(12);
    CHECK(smap.size() == 5);

    CHECK(std::is_sorted(smap.begin(), smap.end(), cmp));

    ifit = smap.find(12);
    CHECK(ifit == smap.end());

    ifit = smap.find(6);
    CHECK(ifit != smap.end());
    smap.erase(ifit);

    CHECK(smap.size() == 4);
    CHECK(std::is_sorted(smap.begin(), smap.end(), cmp));
    ifit = smap.find(6);
    CHECK(ifit == smap.end());
}

#endif

#if defined(CHOBO_FLAT_MAP_TEST_VECTOR_PTR_WITH_DOCTEST)

TEST_CASE("[flat_map] vector_ptr test")
{
    using namespace chobo;
    flat_map<int, char, std::less<int>, vector_ptr<std::pair<int, char>>> smap;

    std::vector<std::pair<int, char>> vec;
    smap.modify_container().reset(&vec);

    smap[1] = '1';
    smap[3] = '3';

    CHECK(smap.at(3) == '3');

    auto smap2 = smap;
    CHECK(smap2.size() == 2);
    CHECK(smap2[1] == '1');
    CHECK(smap2.at(3) == '3');

    smap2[0] = '0';

    CHECK(smap.size() == 3);
    CHECK(smap[0] == '0');

    smap.clear();

    CHECK(smap2.empty());
}

#endif


#endif

