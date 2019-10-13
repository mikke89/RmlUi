// chobo-flat-set v1.00
//
// std::set-like class with an underlying vector
//
// Unofficial, chobo-like container, largely based on chobo::flat_map
//
// MIT License:
// Copyright(c) 2019 Michael R. P. Ragazzon 
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
//  1.00 (2019-05-04) First public release
//
//
//                  DOCUMENTATION
//
// Simply include this file wherever you need.
// It defines the class chobo::flat_set, which is an almsot drop-in replacement
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
// * std::set has <value, compare, allocator>
// * chobo::flat_set has <value, compare, container>
// The container must be an std::vector compatible type (chobo::static_vector
// and chobo::vector_ptr are, for example, viable). The container value type
// must be 'value'.
//
//                  Changing the allocator.
//
// If you want to change the allocator of flat set, you'll have to provide a
// container with the appriate one. Example:
//
// chobo::flat_set<
//      string,
//      less<string>,
//      std::vector<<string>, MyAllocator<string>>
//  > myset
//
//
//                  Configuration
//
// chobo::flat_set has one configurable setting:
//
// 1. const char* overloads
// By default chobo::flat_set provides overloads for the access methods
// (at, operator[], find, lower_bound, count) for const char* for cases when
// std::string is the key, so that no allocations happen when accessing with
// a C-string of a string literal.
// However if const char* or any other class with implicit conversion from
// const char* is the key, they won't compile.
// If you plan on using flat_set with such keys, you'll need to define
// CHOBO_FLAT_SET_NO_CONST_CHAR_OVERLOADS before including the header
//
//
//                  TESTS
//
// The tests are included in the header file and use doctest (https://github.com/onqtam/doctest).
// To run them, define CHOBO_FLAT_SET_TEST_WITH_DOCTEST before including
// the header in a file which has doctest.h already included.
//
// Additionally if chobo::static_vector is also available you may define
// CHOBO_FLAT_SET_TEST_STATIC_VECTOR_WITH_DOCTEST to test flat_set with an
// unrelying static_vector
//
// Additionally if chobo::vector_ptr is also available you may define
// CHOBO_FLAT_SET_TEST_VECTOR_PTR_WITH_DOCTEST to test flat_set with an
// unrelying vector_ptr
//
#pragma once

#include <vector>
#include <algorithm>
#include <type_traits>

#if !defined(CHOBO_FLAT_SET_NO_CONST_CHAR_OVERLOADS)
#include <cstring>
#endif

namespace chobo
{

template <typename T, typename Compare = std::less<T>, typename Container = std::vector<T>>
class flat_set
{
public:
    typedef T key_type;
    typedef T value_type;
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

    flat_set()
    {}

    explicit flat_set(const key_compare& comp, const allocator_type& alloc = allocator_type())
        : m_cmp(comp)
        , m_container(alloc)
    {}

    flat_set(const flat_set& x) = default;
    flat_set(flat_set&& x) = default;


    flat_set(std::initializer_list<value_type> ilist)
    {
        m_container.reserve(ilist.size());
        for (auto&& il : ilist)
            emplace(il);
    }

    flat_set& operator=(const flat_set& x)
    {
        m_cmp = x.m_cmp;
        m_container = x.m_container;
        return *this;
    }
    flat_set& operator=(flat_set&& x) noexcept
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
        auto i = lower_bound(val);
        if (i != end() && !m_cmp(val, *i))
        {
            return { i, false };
        }

        return{ m_container.emplace(i, std::forward<P>(val)), true };
    }

	template <typename InputIt >
    void insert(InputIt first, InputIt last)
    {
		difference_type diff = std::distance(first, last);
		if(diff > 0) reserve(size() + (size_t)diff);
		for (auto it = first; it != last; ++it)
			emplace(*it);
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

    void swap(flat_set& x)
    {
        std::swap(m_cmp, x.m_cmp);
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

#if !defined(CHOBO_FLAT_SET_NO_CONST_CHAR_OVERLOADS)
    ///////////////////////////////////////////////////////////////////////////////////
    // const char* overloads for sets with an std::string key to avoid allocs
    iterator lower_bound(const char* k)
    {
        static_assert(std::is_same<std::string, key_type>::value, "flat_set::lower_bound(const char*) works only for std::strings");
        static_assert(std::is_same<std::less<std::string>, key_compare>::value, "flat_set::lower_bound(const char*) works only for std::string-s, compared with std::less<std::string>");
        return std::lower_bound(m_container.begin(), m_container.end(), k, [](const value_type& a, const char* b) -> bool
        {
            return strcmp(a.c_str(), b) < 0;
        });
    }

    const_iterator lower_bound(const char* k) const
    {
        static_assert(std::is_same<std::string, key_type>::value, "flat_set::lower_bound(const char*) works only for std::strings");
        static_assert(std::is_same<std::less<std::string>, key_compare>::value, "flat_set::lower_bound(const char*) works only for std::string-s, compared with std::less<std::string>");
        return std::lower_bound(m_container.begin(), m_container.end(), k, [](const value_type& a, const char* b) -> bool
        {
            return strcmp(a.c_str(), b) < 0;
        });
    }

    iterator find(const char* k)
    {
        auto i = lower_bound(k);
        if (i != end() && *i == k)
            return i;

        return end();
    }

    const_iterator find(const char* k) const
    {
        auto i = lower_bound(k);
        if (i != end() && *i == k)
            return i;

        return end();
    }

    size_t count(const char* k) const
    {
        return find(k) == end() ? 0 : 1;
    }

#endif // !defined(CHOBO_FLAT_SET_NO_CONST_CHAR_OVERLOADS)

private:
	key_compare m_cmp;
    container_type m_container;
};

template <typename T, typename Compare, typename Container>
bool operator==(const flat_set<T, Compare, Container>& a, const flat_set<T, Compare, Container>& b)
{
    return a.container() == b.container();
}
template <typename T, typename Compare, typename Container>
bool operator!=(const flat_set<T, Compare, Container>& a, const flat_set<T, Compare, Container>& b)
{
    return a.container() != b.container();
}
template <typename T, typename Compare, typename Container>
bool operator<(const flat_set<T, Compare, Container>& a, const flat_set<T, Compare, Container>& b)
{
	return a.container() < b.container();
}

}

#if defined(CHOBO_FLAT_SET_TEST_WITH_DOCTEST)

#include <string>

namespace chobo_flat_set_test
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

TEST_CASE("[flat_set] test")
{
    using namespace chobo;
    using namespace chobo_flat_set_test;

    flat_set<int> iset;
    CHECK(iset.empty());
    CHECK(iset.size() == 0);
    CHECK(iset.capacity() == 0);
    CHECK(iset.begin() == iset.end());

    iset.insert(3);
    CHECK(iset.size() == 1);

    auto iit = iset.begin();
    CHECK(*iit == 3);
    CHECK(iset.count(3) == 1);
    CHECK(iset.count(5) == 0);

    ++iit;
    CHECK(iit == iset.end());

    auto res = iset.insert(6);
    CHECK(res.second);
    CHECK(res.first == iset.begin() + 1);

    res = iset.emplace(4);
    CHECK(res.second);
    CHECK(res.first == iset.begin() + 1);

    res = iset.emplace(6);
    CHECK(!res.second);
    CHECK(res.first == iset.begin() + 2);

    iset.emplace(3);
	CHECK(iset.size() == 3);
    iset.emplace(9);
    iset.insert(9);
    iset.emplace(12);
    CHECK(iset.size() == 5);

    auto cmp = [](const flat_set<int>::value_type& a, const flat_set<int>::value_type& b) -> bool
    {
        return a < b;
    };

    CHECK(std::is_sorted(iset.begin(), iset.end(), cmp));

    iset.erase(12);
    CHECK(iset.size() == 4);

    CHECK(std::is_sorted(iset.begin(), iset.end(), cmp));

    iit = iset.find(11);
    CHECK(iit == iset.end());

    iit = iset.find(6);
    CHECK(iit != iset.end());
    iset.erase(iit);

    CHECK(iset.size() == 3);
    CHECK(std::is_sorted(iset.begin(), iset.end(), cmp));
    iit = iset.find(6);
    CHECK(iit == iset.end());

    //

    flat_set<std::string> sset;

    CHECK(sset.find("123") == sset.end());
	sset.emplace("123");
    CHECK(*sset.begin() == "123");

    sset.emplace("asd");

    auto sit = sset.find("asd");
    CHECK(sit != sset.end());
    CHECK(sit == sset.begin() + 1);

    CHECK(sset.count("bababa") == 0);
    CHECK(sset.count("asd") == 1);

    std::string asd = "asd";
    CHECK(sset.find(asd) == sset.find("asd"));

    sset.emplace("0The quick brown fox jumps over the lazy dog");
    CHECK(sset.begin()->at(1) == 'T');
    const void* cstr = sset.begin()->c_str();

    auto sset2 = std::move(sset);
    CHECK(sset.empty());
    CHECK(sset2.begin()->c_str() == cstr);

    sset = std::move(sset2);
    CHECK(sset2.empty());
    CHECK(sset.begin()->c_str() == cstr);

    CHECK(sset2 != sset);
    sset2 = sset;
    CHECK(sset2 == sset);

    // no == comparable tests
    flat_set<int_wrap, int_wrap::compare> iwset;
    iwset.emplace(5);
	iwset.emplace(20);
	iwset.emplace(10);

    auto iwi = iwset.emplace(3);
    CHECK(iwi.second == true);
    CHECK(iwi.first == iwset.begin());

    CHECK(iwset.begin()->val == 3);
    CHECK(iwset.rbegin()->val == 20);

    iwi = iwset.insert(int_wrap(11));
    CHECK(iwi.second == true);
    CHECK(iwi.first + 2 == iwset.end());

    iwi = iwset.emplace(int_wrap(10));
    CHECK(iwi.second == false);

    CHECK(iwset.find(18) == iwset.end());
    CHECK(iwset.find(11) != iwset.end());

    const auto ciwset = iwset;

    CHECK(ciwset.begin()->val == 3);
    CHECK(ciwset.rbegin()->val == 20);

    CHECK(ciwset.find(18) == ciwset.end());
    CHECK(ciwset.find(11) != ciwset.end());

    // swap
    flat_set<int> m1, m2;
    m1.reserve(10);
    m1.emplace(1);
	m1.emplace(2);
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


	// initializer list
	flat_set<std::string> ilset = { "hello", "great", "magnificent", "world", "hello", "again" };
	CHECK(ilset.size() == 5);
	CHECK(std::is_sorted(ilset.begin(), ilset.end()));

	ilset = { "b", "a" };
	CHECK(ilset.size() == 2);
	CHECK(std::is_sorted(ilset.begin(), ilset.end()));
}

#if defined(CHOBO_FLAT_SET_TEST_STATIC_VECTOR_WITH_DOCTEST)

TEST_CASE("[flat_set] static_vector test")
{
    using namespace chobo;
	
	// Not implemented
}

#endif

#if defined(CHOBO_FLAT_SET_TEST_VECTOR_PTR_WITH_DOCTEST)

TEST_CASE("[flat_set] vector_ptr test")
{
    using namespace chobo;

	// Not implemented
}

#endif


#endif

