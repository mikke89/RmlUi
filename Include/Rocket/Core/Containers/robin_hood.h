//                 ______  _____                 ______                _________
//  ______________ ___  /_ ___(_)_______         ___  /_ ______ ______ ______  /
//  __  ___/_  __ \__  __ \__  / __  __ \        __  __ \_  __ \_  __ \_  __  /
//  _  /    / /_/ /_  /_/ /_  /  _  / / /        _  / / // /_/ // /_/ // /_/ /
//  /_/     \____/ /_.___/ /_/   /_/ /_/ ________/_/ /_/ \____/ \____/ \__,_/
//                                      _/_____/
//
// robin_hood::unordered_map for C++14
// version 3.2.7
// https://github.com/martinus/robin-hood-hashing
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Martin Ankerl <http://martin.ankerl.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ROBIN_HOOD_H_INCLUDED
#define ROBIN_HOOD_H_INCLUDED

// see https://semver.org/
#define ROBIN_HOOD_VERSION_MAJOR 3 // for incompatible API changes
#define ROBIN_HOOD_VERSION_MINOR 2 // for adding functionality in a backwards-compatible manner
#define ROBIN_HOOD_VERSION_PATCH 7 // for backwards-compatible bug fixes

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// #define ROBIN_HOOD_LOG_ENABLED
#ifdef ROBIN_HOOD_LOG_ENABLED
#    include <iostream>
#    define ROBIN_HOOD_LOG(x) std::cout << __FUNCTION__ << "@" << __LINE__ << ": " << x << std::endl
#else
#    define ROBIN_HOOD_LOG(x)
#endif

// mark unused members with this macro
#define ROBIN_HOOD_UNUSED(identifier)

// bitness
#if SIZE_MAX == UINT32_MAX
#    define ROBIN_HOOD_BITNESS 32
#elif SIZE_MAX == UINT64_MAX
#    define ROBIN_HOOD_BITNESS 64
#else
#    error Unsupported bitness
#endif

// endianess
#ifdef _WIN32
#    define ROBIN_HOOD_LITTLE_ENDIAN 1
#    define ROBIN_HOOD_BIG_ENDIAN 0
#else
#    if __GNUC__ >= 4
#        define ROBIN_HOOD_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#        define ROBIN_HOOD_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    else
#        error cannot determine endianness
#    endif
#endif

// inline
#ifdef _WIN32
#    define ROBIN_HOOD_NOINLINE __declspec(noinline)
#else
#    if __GNUC__ >= 4
#        define ROBIN_HOOD_NOINLINE __attribute__((noinline))
#    else
#        define ROBIN_HOOD_NOINLINE
#    endif
#endif

// count leading/trailing bits
#ifdef _WIN32
#    if ROBIN_HOOD_BITNESS == 32
#        define ROBIN_HOOD_BITSCANFORWARD _BitScanForward
#    else
#        define ROBIN_HOOD_BITSCANFORWARD _BitScanForward64
#    endif
#    include <intrin.h>
#    pragma intrinsic(ROBIN_HOOD_BITSCANFORWARD)
#    define ROBIN_HOOD_COUNT_TRAILING_ZEROES(x)                                          \
        [](size_t mask) -> int {                                                         \
            unsigned long index;                                                         \
            return ROBIN_HOOD_BITSCANFORWARD(&index, mask) ? index : ROBIN_HOOD_BITNESS; \
        }(x)
#else
#    if __GNUC__ >= 4
#        if ROBIN_HOOD_BITNESS == 32
#            define ROBIN_HOOD_CTZ(x) __builtin_ctzl(x)
#            define ROBIN_HOOD_CLZ(x) __builtin_clzl(x)
#        else
#            define ROBIN_HOOD_CTZ(x) __builtin_ctzll(x)
#            define ROBIN_HOOD_CLZ(x) __builtin_clzll(x)
#        endif
#        define ROBIN_HOOD_COUNT_LEADING_ZEROES(x) (x ? ROBIN_HOOD_CLZ(x) : ROBIN_HOOD_BITNESS)
#        define ROBIN_HOOD_COUNT_TRAILING_ZEROES(x) (x ? ROBIN_HOOD_CTZ(x) : ROBIN_HOOD_BITNESS)
#    else
#        error clz not supported
#    endif
#endif

// umul
namespace robin_hood {
namespace detail {
#if defined(__SIZEOF_INT128__)
#    define ROBIN_HOOD_UMULH(a, b) \
        static_cast<uint64_t>(     \
            (static_cast<unsigned __int128>(a) * static_cast<unsigned __int128>(b)) >> 64u)

#    define ROBIN_HOOD_HAS_UMUL128 1
inline uint64_t umul128(uint64_t a, uint64_t b, uint64_t* high) {
    auto result = static_cast<unsigned __int128>(a) * static_cast<unsigned __int128>(b);
    *high = static_cast<uint64_t>(result >> 64);
    return static_cast<uint64_t>(result);
}
#elif (defined(_WIN32) && ROBIN_HOOD_BITNESS == 64)
#    define ROBIN_HOOD_HAS_UMUL128 1
#    include <intrin.h> // for __umulh
#    pragma intrinsic(__umulh)
#    pragma intrinsic(_umul128)
#    define ROBIN_HOOD_UMULH(a, b) __umulh(a, b)
inline uint64_t umul128(uint64_t a, uint64_t b, uint64_t* high) {
    return _umul128(a, b, high);
}
#endif
} // namespace detail
} // namespace robin_hood

// likely/unlikely
#if __GNUC__ >= 4
#    define ROBIN_HOOD_LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#    define ROBIN_HOOD_UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
#else
#    define ROBIN_HOOD_LIKELY(condition) condition
#    define ROBIN_HOOD_UNLIKELY(condition) condition
#endif
namespace robin_hood {

namespace detail {

// make sure this is not inlined as it is slow and dramatically enlarges code, thus making other
// inlinings more difficult. Throws are also generally the slow path.
template <typename E, typename... Args>
static ROBIN_HOOD_NOINLINE void doThrow(Args&&... args) {
    throw E(std::forward<Args>(args)...);
}

template <typename E, typename T, typename... Args>
static T* assertNotNull(T* t, Args&&... args) {
    if (ROBIN_HOOD_UNLIKELY(nullptr == t)) {
        doThrow<E>(std::forward<Args>(args)...);
    }
    return t;
}

template <typename T>
inline T unaligned_load(void const* ptr) {
    // using memcpy so we don't get into unaligned load problems.
    // compiler should optimize this very well anyways.
    T t;
    std::memcpy(&t, ptr, sizeof(T));
    return t;
}

// Allocates bulks of memory for objects of type T. This deallocates the memory in the destructor,
// and keeps a linked list of the allocated memory around. Overhead per allocation is the size of a
// pointer.
template <typename T, size_t MinNumAllocs = 4, size_t MaxNumAllocs = 256>
class BulkPoolAllocator {
public:
    BulkPoolAllocator()
        : mHead(nullptr)
        , mListForFree(nullptr) {}

    // does not copy anything, just creates a new allocator.
    BulkPoolAllocator(const BulkPoolAllocator& ROBIN_HOOD_UNUSED(o) /*unused*/)
        : mHead(nullptr)
        , mListForFree(nullptr) {}

    BulkPoolAllocator(BulkPoolAllocator&& o)
        : mHead(o.mHead)
        , mListForFree(o.mListForFree) {
        o.mListForFree = nullptr;
        o.mHead = nullptr;
    }

    BulkPoolAllocator& operator=(BulkPoolAllocator&& o) {
        reset();
        mHead = o.mHead;
        mListForFree = o.mListForFree;
        o.mListForFree = nullptr;
        o.mHead = nullptr;
        return *this;
    }

    BulkPoolAllocator& operator=(const BulkPoolAllocator& ROBIN_HOOD_UNUSED(o) /*unused*/) {
        // does not do anything
        return *this;
    }

    ~BulkPoolAllocator() {
        reset();
    }

    // Deallocates all allocated memory.
    void reset() {
        while (mListForFree) {
            T* tmp = *mListForFree;
            free(mListForFree);
            mListForFree = reinterpret_cast<T**>(tmp);
        }
        mHead = nullptr;
    }

    // allocates, but does NOT initialize. Use in-place new constructor, e.g.
    //   T* obj = pool.allocate();
    //   ::new (static_cast<void*>(obj)) T();
    T* allocate() {
        T* tmp = mHead;
        if (!tmp) {
            tmp = performAllocation();
        }

        mHead = *reinterpret_cast<T**>(tmp);
        return tmp;
    }

    // does not actually deallocate but puts it in store.
    // make sure you have already called the destructor! e.g. with
    //  obj->~T();
    //  pool.deallocate(obj);
    void deallocate(T* obj) {
        *reinterpret_cast<T**>(obj) = mHead;
        mHead = obj;
    }

    // Adds an already allocated block of memory to the allocator. This allocator is from now on
    // responsible for freeing the data (with free()). If the provided data is not large enough to
    // make use of, it is immediately freed. Otherwise it is reused and freed in the destructor.
    void addOrFree(void* ptr, const size_t numBytes) {
        // calculate number of available elements in ptr
        if (numBytes < ALIGNMENT + ALIGNED_SIZE) {
            // not enough data for at least one element. Free and return.
            free(ptr);
        } else {
            add(ptr, numBytes);
        }
    }

    void swap(BulkPoolAllocator<T, MinNumAllocs, MaxNumAllocs>& other) {
        using std::swap;
        swap(mHead, other.mHead);
        swap(mListForFree, other.mListForFree);
    }

private:
    // iterates the list of allocated memory to calculate how many to alloc next.
    // Recalculating this each time saves us a size_t member.
    // This ignores the fact that memory blocks might have been added manually with addOrFree. In
    // practice, this should not matter much.
    size_t calcNumElementsToAlloc() const {
        auto tmp = mListForFree;
        size_t numAllocs = MinNumAllocs;

        while (numAllocs * 2 <= MaxNumAllocs && tmp) {
            auto x = reinterpret_cast<T***>(tmp);
            tmp = *x;
            numAllocs *= 2;
        }

        return numAllocs;
    }

    // WARNING: Underflow if numBytes < ALIGNMENT! This is guarded in addOrFree().
    void add(void* ptr, const size_t numBytes) {
        const size_t numElements = (numBytes - ALIGNMENT) / ALIGNED_SIZE;

        auto data = reinterpret_cast<T**>(ptr);

        // link free list
        auto x = reinterpret_cast<T***>(data);
        *x = mListForFree;
        mListForFree = data;

        // create linked list for newly allocated data
        auto const headT = reinterpret_cast<T*>(reinterpret_cast<char*>(ptr) + ALIGNMENT);

        auto const head = reinterpret_cast<char*>(headT);

        // Visual Studio compiler automatically unrolls this loop, which is pretty cool
        for (size_t i = 0; i < numElements; ++i) {
            *reinterpret_cast<char**>(head + i * ALIGNED_SIZE) = head + (i + 1) * ALIGNED_SIZE;
        }

        // last one points to 0
        *reinterpret_cast<T**>(head + (numElements - 1) * ALIGNED_SIZE) = mHead;
        mHead = headT;
    }

    // Called when no memory is available (mHead == 0).
    // Don't inline this slow path.
    ROBIN_HOOD_NOINLINE T* performAllocation() {
        size_t const numElementsToAlloc = calcNumElementsToAlloc();

        // alloc new memory: [prev |T, T, ... T]
        // std::cout << (sizeof(T*) + ALIGNED_SIZE * numElementsToAlloc) << " bytes" << std::endl;
        size_t const bytes = ALIGNMENT + ALIGNED_SIZE * numElementsToAlloc;
        add(assertNotNull<std::bad_alloc>(malloc(bytes)), bytes);
        return mHead;
    }

    // enforce byte alignment of the T's
    static constexpr size_t ALIGNMENT =
        (std::max)(std::alignment_of<T>::value, std::alignment_of<T*>::value);
    static constexpr size_t ALIGNED_SIZE = ((sizeof(T) - 1) / ALIGNMENT + 1) * ALIGNMENT;

    static_assert(MinNumAllocs >= 1, "MinNumAllocs");
    static_assert(MaxNumAllocs >= MinNumAllocs, "MaxNumAllocs");
    static_assert(ALIGNED_SIZE >= sizeof(T*), "ALIGNED_SIZE");
    static_assert(0 == (ALIGNED_SIZE % sizeof(T*)), "ALIGNED_SIZE mod");
    static_assert(ALIGNMENT >= sizeof(T*), "ALIGNMENT");

    T* mHead;
    T** mListForFree;
};

template <typename T, size_t MinSize, size_t MaxSize, bool IsFlatMap>
struct NodeAllocator;

// dummy allocator that does nothing
template <typename T, size_t MinSize, size_t MaxSize>
struct NodeAllocator<T, MinSize, MaxSize, true> {

    // we are not using the data, so just free it.
    void addOrFree(void* ptr, size_t ROBIN_HOOD_UNUSED(numBytes) /*unused*/) {
        free(ptr);
    }
};

template <typename T, size_t MinSize, size_t MaxSize>
struct NodeAllocator<T, MinSize, MaxSize, false> : public BulkPoolAllocator<T, MinSize, MaxSize> {};

} // namespace detail

struct is_transparent_tag {};

// A custom pair implementation is used in the map because std::pair is not is_trivially_copyable,
// which means it would  not be allowed to be used in std::memcpy. This struct is copyable, which is
// also tested.
template <typename First, typename Second>
struct pair {
    using first_type = First;
    using second_type = Second;

    // pair constructors are explicit so we don't accidentally call this ctor when we don't have to.
    explicit pair(std::pair<First, Second> const& o)
        : first{o.first}
        , second{o.second} {}

    // pair constructors are explicit so we don't accidentally call this ctor when we don't have to.
    explicit pair(std::pair<First, Second>&& o)
        : first{std::move(o.first)}
        , second{std::move(o.second)} {}

    constexpr pair(const First& firstArg, const Second& secondArg)
        : first{firstArg}
        , second{secondArg} {}

    constexpr pair(First&& firstArg, Second&& secondArg)
        : first{std::move(firstArg)}
        , second{std::move(secondArg)} {}

    template <typename FirstArg, typename SecondArg>
    constexpr pair(FirstArg&& firstArg, SecondArg&& secondArg)
        : first{std::forward<FirstArg>(firstArg)}
        , second{std::forward<SecondArg>(secondArg)} {}

    template <typename... Args1, typename... Args2>
    pair(std::piecewise_construct_t /*unused*/, std::tuple<Args1...> firstArgs,
         std::tuple<Args2...> secondArgs)
        : pair{firstArgs, secondArgs, std::index_sequence_for<Args1...>{},
               std::index_sequence_for<Args2...>{}} {}

    // constructor called from the std::piecewise_construct_t ctor
    template <typename... Args1, size_t... Indexes1, typename... Args2, size_t... Indexes2>
    inline pair(std::tuple<Args1...>& tuple1, std::tuple<Args2...>& tuple2,
                std::index_sequence<Indexes1...> /*unused*/,
                std::index_sequence<Indexes2...> /*unused*/)
        : first{std::forward<Args1>(std::get<Indexes1>(tuple1))...}
        , second{std::forward<Args2>(std::get<Indexes2>(tuple2))...} {
        // make visual studio compiler happy about warning about unused tuple1 & tuple2.
        // Visual studio's pair implementation disables warning 4100.
        (void)tuple1;
        (void)tuple2;
    }

    first_type& getFirst() {
        return first;
    }
    first_type const& getFirst() const {
        return first;
    }
    second_type& getSecond() {
        return second;
    }
    second_type const& getSecond() const {
        return second;
    }

    void swap(pair<First, Second>& o) {
        using std::swap;
        swap(first, o.first);
        swap(second, o.second);
    }

    First first;
    Second second;
};

// A thin wrapper around std::hash, performing a single multiplication to (hopefully) get nicely
// randomized upper bits, which are used by the unordered_map.
template <typename T>
struct hash : public std::hash<T> {
    size_t operator()(T const& obj) const {
        return std::hash<T>::operator()(obj);
    }
};

// Hash an arbitrary amount of bytes. This is basically Murmur2 hash without caring about big
// endianness. TODO add a fallback for very large strings?
inline size_t hash_bytes(void const* ptr, size_t const len) {
    static constexpr uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
    static constexpr uint64_t seed = UINT64_C(0xe17a1465);
    static constexpr unsigned int r = 47;

    auto const data64 = reinterpret_cast<uint64_t const*>(ptr);
    uint64_t h = seed ^ (len * m);

    size_t const n_blocks = len / 8;
    for (size_t i = 0; i < n_blocks; ++i) {
        uint64_t k = detail::unaligned_load<uint64_t>(data64 + i);

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    auto const data8 = reinterpret_cast<uint8_t const*>(data64 + n_blocks);
    switch (len & 7u) {
    case 7:
        h ^= static_cast<uint64_t>(data8[6]) << 48u;
        // fallthrough
    case 6:
        h ^= static_cast<uint64_t>(data8[5]) << 40u;
        // fallthrough
    case 5:
        h ^= static_cast<uint64_t>(data8[4]) << 32u;
        // fallthrough
    case 4:
        h ^= static_cast<uint64_t>(data8[3]) << 24u;
        // fallthrough
    case 3:
        h ^= static_cast<uint64_t>(data8[2]) << 16u;
        // fallthrough
    case 2:
        h ^= static_cast<uint64_t>(data8[1]) << 8u;
        // fallthrough
    case 1:
        h ^= static_cast<uint64_t>(data8[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return static_cast<size_t>(h);
}

template <>
struct hash<std::string> {
    size_t operator()(std::string const& str) const {
        return hash_bytes(str.data(), str.size());
    }
};

// specialization used for uint64_t and int64_t. Uses 128bit multiplication
template <>
struct hash<uint64_t> {
    size_t operator()(uint64_t const& obj) const {
#if defined(ROBIN_HOOD_HAS_UMUL128)
        // 167079903232 masksum, 120428523 ops best: 0xde5fb9d2630458e9
        static constexpr uint64_t k = UINT64_C(0xde5fb9d2630458e9);
        uint64_t h;
        uint64_t l = detail::umul128(obj, k, &h);
        return h + l;
#elif ROBIN_HOOD_BITNESS == 32
        static constexpr uint32_t k = UINT32_C(0x9a0ecda7);
        uint64_t const r = obj * k;
        uint32_t h = static_cast<uint32_t>(r >> 32);
        uint32_t l = static_cast<uint32_t>(r);
        return h + l;
#else
        // murmurhash 3 finalizer
        uint64_t h = obj;
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccd;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53;
        h ^= h >> 33;
        return static_cast<size_t>(h);
#endif
    }
};

template <>
struct hash<int64_t> {
    size_t operator()(int64_t const& obj) const {
        return hash<uint64_t>{}(static_cast<uint64_t>(obj));
    }
};

template <>
struct hash<uint32_t> {
    size_t operator()(uint32_t const& h) const {
#if ROBIN_HOOD_BITNESS == 32
        return static_cast<size_t>((UINT64_C(0xca4bcaa75ec3f625) * (uint64_t)h) >> 32);
#else
        return hash<uint64_t>{}(static_cast<uint64_t>(h));
#endif
    }
};

template <>
struct hash<int32_t> {
    size_t operator()(int32_t const& obj) const {
        return hash<uint32_t>{}(static_cast<uint32_t>(obj));
    }
};

namespace detail {

// A highly optimized hashmap implementation, using the Robin Hood algorithm.
//
// In most cases, this map should be usable as a drop-in replacement for std::unordered_map, but be
// about 2x faster in most cases and require much less allocations.
//
// This implementation uses the following memory layout:
//
// [Node, Node, ... Node | info, info, ... infoSentinel ]
//
// * Node: either a DataNode that directly has the std::pair<key, val> as member,
//   or a DataNode with a pointer to std::pair<key,val>. Which DataNode representation to use
//   depends on how fast the swap() operation is. Heuristically, this is automatically choosen based
//   on sizeof(). there are always 2^n Nodes.
//
// * info: Each Node in the map has a corresponding info byte, so there are 2^n info bytes.
//   Each byte is initialized to 0, meaning the corresponding Node is empty. Set to 1 means the
//   corresponding node contains data. Set to 2 means the corresponding Node is filled, but it
//   actually belongs to the previous position and was pushed out because that place is already
//   taken.
//
// * infoSentinel: Sentinel byte set to 1, so that iterator's ++ can stop at end() without the need
// for a idx
//   variable.
//
// According to STL, order of templates has effect on throughput. That's why I've moved the boolean
// to the front.
// https://www.reddit.com/r/cpp/comments/ahp6iu/compile_time_binary_size_reductions_and_cs_future/eeguck4/
template <bool IsFlatMap, size_t MaxLoadFactor100, typename Key, typename T, typename Hash,
          typename KeyEqual>
class unordered_map
    : public Hash,
      public KeyEqual,
      detail::NodeAllocator<
          robin_hood::pair<typename std::conditional<IsFlatMap, Key, Key const>::type, T>, 4, 16384,
          IsFlatMap> {
public:
    using key_type = Key;
    using mapped_type = T;
    using value_type =
        robin_hood::pair<typename std::conditional<IsFlatMap, Key, Key const>::type, T>;
    using size_type = size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using Self =
        unordered_map<IsFlatMap, MaxLoadFactor100, key_type, mapped_type, hasher, key_equal>;
    static constexpr bool is_flat_map = IsFlatMap;

private:
    static_assert(MaxLoadFactor100 > 10 && MaxLoadFactor100 < 100,
                  "MaxLoadFactor100 needs to be >10 && < 100");

    // configuration defaults

    // make sure we have 8 elements, needed to quickly rehash mInfo
    static constexpr size_t InitialNumElements = sizeof(uint64_t);
    static constexpr int InitialInfoNumBits = 5;
    static constexpr uint8_t InitialInfoInc = 1 << InitialInfoNumBits;
    static constexpr uint8_t InitialInfoHashShift = sizeof(size_t) * 8 - InitialInfoNumBits;
    using DataPool = detail::NodeAllocator<value_type, 4, 16384, IsFlatMap>;

    // type needs to be wider than uint8_t.
    using InfoType = int32_t;

    // DataNode ////////////////////////////////////////////////////////

    // Primary template for the data node. We have special implementations for small and big
    // objects. For large objects it is assumed that swap() is fairly slow, so we allocate these on
    // the heap so swap merely swaps a pointer.
    template <typename M, bool>
    class DataNode {};

    // Small: just allocate on the stack.
    template <typename M>
    class DataNode<M, true> {
    public:
        template <typename... Args>
        explicit DataNode(M& ROBIN_HOOD_UNUSED(map) /*unused*/, Args&&... args)
            : mData(std::forward<Args>(args)...) {}

        DataNode(M& ROBIN_HOOD_UNUSED(map) /*unused*/, DataNode<M, true>&& n)
            : mData(std::move(n.mData)) {}

        // doesn't do anything
        void destroy(M& ROBIN_HOOD_UNUSED(map) /*unused*/) {}
        void destroyDoNotDeallocate() {}

        value_type const* operator->() const {
            return &mData;
        }
        value_type* operator->() {
            return &mData;
        }

        const value_type& operator*() const {
            return mData;
        }

        value_type& operator*() {
            return mData;
        }

        typename value_type::first_type& getFirst() {
            return mData.first;
        }

        typename value_type::first_type const& getFirst() const {
            return mData.first;
        }

        typename value_type::second_type& getSecond() {
            return mData.second;
        }

        typename value_type::second_type const& getSecond() const {
            return mData.second;
        }

        void swap(DataNode<M, true>& o) {
            mData.swap(o.mData);
        }

    private:
        value_type mData;
    };

    // big object: allocate on heap.
    template <typename M>
    class DataNode<M, false> {
    public:
        template <typename... Args>
        explicit DataNode(M& map, Args&&... args)
            : mData(map.allocate()) {
            ::new (static_cast<void*>(mData)) value_type(std::forward<Args>(args)...);
        }

        DataNode(M& ROBIN_HOOD_UNUSED(map) /*unused*/, DataNode<M, false>&& n)
            : mData(std::move(n.mData)) {}

        void destroy(M& map) {
            // don't deallocate, just put it into list of datapool.
            mData->~value_type();
            map.deallocate(mData);
        }

        void destroyDoNotDeallocate() {
            mData->~value_type();
        }

        value_type const* operator->() const {
            return mData;
        }

        value_type* operator->() {
            return mData;
        }

        const value_type& operator*() const {
            return *mData;
        }

        value_type& operator*() {
            return *mData;
        }

        typename value_type::first_type& getFirst() {
            return mData->first;
        }

        typename value_type::first_type const& getFirst() const {
            return mData->first;
        }

        typename value_type::second_type& getSecond() {
            return mData->second;
        }

        typename value_type::second_type const& getSecond() const {
            return mData->second;
        }

        void swap(DataNode<M, false>& o) {
            using std::swap;
            swap(mData, o.mData);
        }

    private:
        value_type* mData;
    };

    using Node = DataNode<Self, IsFlatMap>;

    // Cloner //////////////////////////////////////////////////////////

    template <typename M, bool UseMemcpy>
    struct Cloner;

    // fast path: Just copy data, without allocating anything.
    template <typename M>
    struct Cloner<M, true> {
        void operator()(M const& source, M& target) const {
            // std::memcpy(target.mKeyVals, source.mKeyVals,
            //             target.calcNumBytesTotal(target.mMask + 1));
            auto src = reinterpret_cast<char const*>(source.mKeyVals);
            auto tgt = reinterpret_cast<char*>(target.mKeyVals);
            std::copy(src, src + target.calcNumBytesTotal(target.mMask + 1), tgt);
        }
    };

    template <typename M>
    struct Cloner<M, false> {
        void operator()(M const& source, M& target) const {
            // make sure to copy initialize sentinel as well
            // std::memcpy(target.mInfo, source.mInfo, target.calcNumBytesInfo(target.mMask + 1));
            std::copy(source.mInfo, source.mInfo + target.calcNumBytesInfo(target.mMask + 1),
                      target.mInfo);

            for (size_t i = 0; i < target.mMask + 1; ++i) {
                if (target.mInfo[i]) {
                    ::new (static_cast<void*>(target.mKeyVals + i))
                        Node(target, *source.mKeyVals[i]);
                }
            }
        }
    };

    // Destroyer ///////////////////////////////////////////////////////

    template <typename M, bool IsFlatMapAndTrivial>
    struct Destroyer {};

    template <typename M>
    struct Destroyer<M, true> {
        void nodes(M& m) const {
            m.mNumElements = 0;
        }

        void nodesDoNotDeallocate(M& m) const {
            m.mNumElements = 0;
        }
    };

    template <typename M>
    struct Destroyer<M, false> {
        void nodes(M& m) const {
            m.mNumElements = 0;
            // clear also resets mInfo to 0, that's sometimes not necessary.
            for (size_t idx = 0; idx <= m.mMask; ++idx) {
                if (0 != m.mInfo[idx]) {
                    Node& n = m.mKeyVals[idx];
                    n.destroy(m);
                    n.~Node();
                }
            }
        }

        void nodesDoNotDeallocate(M& m) const {
            m.mNumElements = 0;
            // clear also resets mInfo to 0, that's sometimes not necessary.
            for (size_t idx = 0; idx <= m.mMask; ++idx) {
                if (0 != m.mInfo[idx]) {
                    Node& n = m.mKeyVals[idx];
                    n.destroyDoNotDeallocate();
                    n.~Node();
                }
            }
        }
    };

    // Iter ////////////////////////////////////////////////////////////

    struct fast_forward_tag {};

    // generic iterator for both const_iterator and iterator.
    template <bool IsConst>
    class Iter {
    private:
        using NodePtr = typename std::conditional<IsConst, Node const*, Node*>::type;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename Self::value_type;
        using reference = typename std::conditional<IsConst, value_type const&, value_type&>::type;
        using pointer = typename std::conditional<IsConst, value_type const*, value_type*>::type;
        using iterator_category = std::forward_iterator_tag;

        // default constructed iterator can be compared to itself, but WON'T return true when
        // compared to end().
        Iter()
            : mKeyVals(nullptr)
            , mInfo(nullptr) {}

        // both const_iterator and iterator can be constructed from a non-const iterator
        Iter(Iter<false> const& other)
            : mKeyVals(other.mKeyVals)
            , mInfo(other.mInfo) {}

        Iter(NodePtr valPtr, uint8_t const* infoPtr)
            : mKeyVals(valPtr)
            , mInfo(infoPtr) {}

        Iter(NodePtr valPtr, uint8_t const* infoPtr,
             fast_forward_tag ROBIN_HOOD_UNUSED(tag) /*unused*/)
            : mKeyVals(valPtr)
            , mInfo(infoPtr) {
            fastForward();
        }

        // prefix increment. Undefined behavior if we are at end()!
        Iter& operator++() {
            mInfo++;
            mKeyVals++;
            fastForward();
            return *this;
        }

        reference operator*() const {
            return **mKeyVals;
        }

        pointer operator->() const {
            return &**mKeyVals;
        }

        template <bool O>
        bool operator==(Iter<O> const& o) const {
            return mKeyVals == o.mKeyVals;
        }

        template <bool O>
        bool operator!=(Iter<O> const& o) const {
            return mKeyVals != o.mKeyVals;
        }

    private:
        // fast forward to the next non-free info byte
        void fastForward() {
            int inc;
            do {
                auto const n = detail::unaligned_load<size_t>(mInfo);
#if ROBIN_HOOD_LITTLE_ENDIAN
                inc = ROBIN_HOOD_COUNT_TRAILING_ZEROES(n) / 8;
#else
                inc = ROBIN_HOOD_COUNT_LEADING_ZEROES(n) / 8;
#endif
                mInfo += inc;
                mKeyVals += inc;
            } while (inc == sizeof(size_t));
        }

        friend class unordered_map<IsFlatMap, MaxLoadFactor100, key_type, mapped_type, hasher,
                                   key_equal>;
        NodePtr mKeyVals;
        uint8_t const* mInfo;
    };

    ////////////////////////////////////////////////////////////////////

    size_t calcNumBytesInfo(size_t numElements) const {
        const size_t s = sizeof(uint8_t) * (numElements + 1);
        if (ROBIN_HOOD_UNLIKELY(s / sizeof(uint8_t) != numElements + 1)) {
            throwOverflowError();
        }
        // make sure it's a bit larger, so we can load 64bit numbers
        return s + sizeof(uint64_t);
    }
    size_t calcNumBytesNode(size_t numElements) const {
        const size_t s = sizeof(Node) * numElements;
        if (ROBIN_HOOD_UNLIKELY(s / sizeof(Node) != numElements)) {
            throwOverflowError();
        }
        return s;
    }
    size_t calcNumBytesTotal(size_t numElements) const {
        const size_t si = calcNumBytesInfo(numElements);
        const size_t sn = calcNumBytesNode(numElements);
        const size_t s = si + sn;
        if (ROBIN_HOOD_UNLIKELY(s <= si || s <= sn)) {
            throwOverflowError();
        }
        return s;
    }

    // highly performance relevant code.
    // Lower bits are used for indexing into the array (2^n size)
    // The upper 1-5 bits need to be a reasonable good hash, to save comparisons.
    template <typename HashKey>
    void keyToIdx(HashKey&& key, size_t& idx, InfoType& info) const {
        static constexpr size_t bad_hash_prevention =
            std::is_same<::robin_hood::hash<key_type>, hasher>::value
                ? 1
                : (ROBIN_HOOD_BITNESS == 64 ? UINT64_C(0xb3727c1f779b8d8b) : UINT32_C(0xda4afe47));
        idx = Hash::operator()(key) * bad_hash_prevention;
        info = static_cast<InfoType>(mInfoInc + static_cast<InfoType>(idx >> mInfoHashShift));
        idx &= mMask;
    }

    // forwards the index by one, wrapping around at the end
    void next(InfoType* info, size_t* idx) const {
        *idx = (*idx + 1) & mMask;
        *info = static_cast<InfoType>(*info + mInfoInc);
    }

    void nextWhileLess(InfoType* info, size_t* idx) const {
        // unrolling this by hand did not bring any speedups.
        while (*info < mInfo[*idx]) {
            next(info, idx);
        }
    }

    // Shift everything up by one element. Tries to move stuff around.
    // True if some shifting has occured (entry under idx is a constructed object)
    // Fals if no shift has occured (entry under idx is unconstructed memory)
    void shiftUp(size_t idx, size_t const insertion_idx) {
        while (idx != insertion_idx) {
            size_t prev_idx = (idx - 1) & mMask;
            if (mInfo[idx]) {
                mKeyVals[idx] = std::move(mKeyVals[prev_idx]);
            } else {
                ::new (static_cast<void*>(mKeyVals + idx)) Node(std::move(mKeyVals[prev_idx]));
            }
            mInfo[idx] = static_cast<uint8_t>(mInfo[prev_idx] + mInfoInc);
            if (ROBIN_HOOD_UNLIKELY(mInfo[idx] + mInfoInc > 0xFF)) {
                mMaxNumElementsAllowed = 0;
            }
            idx = prev_idx;
        }
    }

    void shiftDown(size_t idx) {
        // until we find one that is either empty or has zero offset.
        // TODO we don't need to move everything, just the last one for the same bucket.
        mKeyVals[idx].destroy(*this);

        // until we find one that is either empty or has zero offset.
        size_t nextIdx = (idx + 1) & mMask;
        while (mInfo[nextIdx] >= 2 * mInfoInc) {
            mInfo[idx] = static_cast<uint8_t>(mInfo[nextIdx] - mInfoInc);
            mKeyVals[idx] = std::move(mKeyVals[nextIdx]);
            idx = nextIdx;
            nextIdx = (idx + 1) & mMask;
        }

        mInfo[idx] = 0;
        // don't destroy, we've moved it
        // mKeyVals[idx].destroy(*this);
        mKeyVals[idx].~Node();
    }

    // copy of find(), except that it returns iterator instead of const_iterator.
    template <typename Other>
    size_t findIdx(Other const& key) const {
        size_t idx;
        InfoType info;
        keyToIdx(key, idx, info);

        do {
            // unrolling this twice gives a bit of a speedup. More unrolling did not help.
            if (info == mInfo[idx] && KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                return idx;
            }
            next(&info, &idx);
            if (info == mInfo[idx] && KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                return idx;
            }
            next(&info, &idx);
        } while (info <= mInfo[idx]);

        // nothing found!
        return mMask == 0 ? 0 : mMask + 1;
    }

    void cloneData(const unordered_map& o) {
        Cloner<unordered_map, IsFlatMap && std::is_trivially_copyable<Node>::value>()(o, *this);
    }

    // inserts a keyval that is guaranteed to be new, e.g. when the hashmap is resized.
    // @return index where the element was created
    size_t insert_move(Node&& keyval) {
        // we don't retry, fail if overflowing
        // don't need to check max num elements
        if (0 == mMaxNumElementsAllowed && !try_increase_info()) {
            throwOverflowError();
        }

        size_t idx;
        InfoType info;
        keyToIdx(keyval.getFirst(), idx, info);

        // skip forward. Use <= because we are certain that the element is not there.
        while (info <= mInfo[idx]) {
            idx = (idx + 1) & mMask;
            info = static_cast<InfoType>(info + mInfoInc);
        }

        // key not found, so we are now exactly where we want to insert it.
        auto const insertion_idx = idx;
        auto const insertion_info = static_cast<uint8_t>(info);
        if (ROBIN_HOOD_UNLIKELY(insertion_info + mInfoInc > 0xFF)) {
            mMaxNumElementsAllowed = 0;
        }

        // find an empty spot
        while (0 != mInfo[idx]) {
            next(&info, &idx);
        }

        auto& l = mKeyVals[insertion_idx];
        if (idx == insertion_idx) {
            ::new (static_cast<void*>(&l)) Node(std::move(keyval));
        } else {
            shiftUp(idx, insertion_idx);
            l = std::move(keyval);
        }

        // put at empty spot
        mInfo[insertion_idx] = insertion_info;

        ++mNumElements;
        return insertion_idx;
    }

public:
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    // Creates an empty hash map. Nothing is allocated yet, this happens at the first insert. This
    // tremendously speeds up ctor & dtor of a map that never receives an element. The penalty is
    // payed at the first insert, and not before. Lookup of this empty map works because everybody
    // points to DummyInfoByte::b. parameter bucket_count is dictated by the standard, but we can
    // ignore it.
    explicit unordered_map(size_t ROBIN_HOOD_UNUSED(bucket_count) /*unused*/ = 0,
                           const Hash& h = Hash{}, const KeyEqual& equal = KeyEqual{})
        : Hash(h)
        , KeyEqual(equal) {}

    template <typename Iter>
    unordered_map(Iter first, Iter last, size_t ROBIN_HOOD_UNUSED(bucket_count) /*unused*/ = 0,
                  const Hash& h = Hash{}, const KeyEqual& equal = KeyEqual{})
        : Hash(h)
        , KeyEqual(equal) {
        insert(first, last);
    }

    unordered_map(std::initializer_list<value_type> init,
                  size_t ROBIN_HOOD_UNUSED(bucket_count) /*unused*/ = 0, const Hash& h = Hash{},
                  const KeyEqual& equal = KeyEqual{})
        : Hash(h)
        , KeyEqual(equal) {
        insert(init.begin(), init.end());
    }

    unordered_map(unordered_map&& o)
        : Hash(std::move(static_cast<Hash&>(o)))
        , KeyEqual(std::move(static_cast<KeyEqual&>(o)))
        , DataPool(std::move(static_cast<DataPool&>(o))) {
        if (o.mMask) {
            mKeyVals = std::move(o.mKeyVals);
            mInfo = std::move(o.mInfo);
            mNumElements = std::move(o.mNumElements);
            mMask = std::move(o.mMask);
            mMaxNumElementsAllowed = std::move(o.mMaxNumElementsAllowed);
            mInfoInc = std::move(o.mInfoInc);
            mInfoHashShift = std::move(o.mInfoHashShift);
            // set other's mask to 0 so its destructor won't do anything
            o.mMask = 0;
        }
    }

    unordered_map& operator=(unordered_map&& o) {
        if (&o != this) {
            if (o.mMask) {
                // only move stuff if the other map actually has some data
                destroy();
                mKeyVals = std::move(o.mKeyVals);
                mInfo = std::move(o.mInfo);
                mNumElements = std::move(o.mNumElements);
                mMask = std::move(o.mMask);
                mMaxNumElementsAllowed = std::move(o.mMaxNumElementsAllowed);
                mInfoInc = std::move(o.mInfoInc);
                mInfoHashShift = std::move(o.mInfoHashShift);
                Hash::operator=(std::move(static_cast<Hash&>(o)));
                KeyEqual::operator=(std::move(static_cast<KeyEqual&>(o)));
                DataPool::operator=(std::move(static_cast<DataPool&>(o)));
                // set other's mask to 0 so its destructor won't do anything
                o.mMask = 0;
            } else {
                // nothing in the other map => just clear us.
                clear();
            }
        }
        return *this;
    }

    unordered_map(const unordered_map& o)
        : Hash(static_cast<const Hash&>(o))
        , KeyEqual(static_cast<const KeyEqual&>(o))
        , DataPool(static_cast<const DataPool&>(o)) {

        if (!o.empty()) {
            // not empty: create an exact copy. it is also possible to just iterate through all
            // elements and insert them, but copying is probably faster.

            mKeyVals = static_cast<Node*>(
                detail::assertNotNull<std::bad_alloc>(malloc(calcNumBytesTotal(o.mMask + 1))));
            // no need for calloc because clonData does memcpy
            mInfo = reinterpret_cast<uint8_t*>(mKeyVals + o.mMask + 1);
            mNumElements = o.mNumElements;
            mMask = o.mMask;
            mMaxNumElementsAllowed = o.mMaxNumElementsAllowed;
            mInfoInc = o.mInfoInc;
            mInfoHashShift = o.mInfoHashShift;
            cloneData(o);
        }
    }

    // Creates a copy of the given map. Copy constructor of each entry is used.
    unordered_map& operator=(unordered_map const& o) {
        if (&o == this) {
            // prevent assigning of itself
            return *this;
        }

        // we keep using the old allocator and not assign the new one, because we want to keep the
        // memory available. when it is the same size.
        if (o.empty()) {
            if (0 == mMask) {
                // nothing to do, we are empty too
                return *this;
            }

            // not empty: destroy what we have there
            // clear also resets mInfo to 0, that's sometimes not necessary.
            destroy();

            // we assign invalid pointer, but this is ok because we never dereference it.
            // The worst that can happen is that find() is called, which will return an iterator but
            // it will be the end() iterator.
            mKeyVals = reinterpret_cast<Node*>(&mMask);
            // we need to point somewhere thats 0 as long as we're empty
            mInfo = reinterpret_cast<uint8_t*>(mMask);
            Hash::operator=(static_cast<const Hash&>(o));
            KeyEqual::operator=(static_cast<const KeyEqual&>(o));
            DataPool::operator=(static_cast<DataPool const&>(o));
            mNumElements = 0;
            mMask = 0;
            mMaxNumElementsAllowed = 0;
            mInfoInc = InitialInfoInc;
            mInfoHashShift = InitialInfoHashShift;
            return *this;
        }

        // clean up old stuff
        Destroyer<Self, IsFlatMap && std::is_trivially_destructible<Node>::value>{}.nodes(*this);

        if (mMask != o.mMask) {
            // no luck: we don't have the same array size allocated, so we need to realloc.
            if (0 != mMask) {
                // only deallocate if we actually have data!
                free(mKeyVals);
            }

            mKeyVals = static_cast<Node*>(
                detail::assertNotNull<std::bad_alloc>(malloc(calcNumBytesTotal(o.mMask + 1))));

            // no need for calloc here because cloneData performs a memcpy.
            mInfo = reinterpret_cast<uint8_t*>(mKeyVals + o.mMask + 1);
            mInfoInc = o.mInfoInc;
            mInfoHashShift = o.mInfoHashShift;
            // sentinel is set in cloneData
        }
        Hash::operator=(static_cast<const Hash&>(o));
        KeyEqual::operator=(static_cast<const KeyEqual&>(o));
        mNumElements = o.mNumElements;
        mMask = o.mMask;
        mMaxNumElementsAllowed = o.mMaxNumElementsAllowed;
        cloneData(o);

        return *this;
    }

    // Swaps everything between the two maps.
    void swap(unordered_map& o) {
        using std::swap;
        swap(mKeyVals, o.mKeyVals);
        swap(mInfo, o.mInfo);
        swap(mNumElements, o.mNumElements);
        swap(mMask, o.mMask);
        swap(mMaxNumElementsAllowed, o.mMaxNumElementsAllowed);
        swap(mInfoInc, o.mInfoInc);
        swap(mInfoHashShift, o.mInfoHashShift);
        swap(static_cast<Hash&>(*this), static_cast<Hash&>(o));
        swap(static_cast<KeyEqual&>(*this), static_cast<KeyEqual&>(o));
        // no harm done in swapping datapool
        swap(static_cast<DataPool&>(*this), static_cast<DataPool&>(o));
    }

    // Clears all data, without resizing.
    void clear() {
        if (empty()) {
            // don't do anything! also important because we don't want to write to DummyInfoByte::b,
            // even though we would just write 0 to it.
            return;
        }

        Destroyer<Self, IsFlatMap && std::is_trivially_destructible<Node>::value>{}.nodes(*this);

        // clear everything except the sentinel
        // std::memset(mInfo, 0, sizeof(uint8_t) * (mMask + 1));
        uint8_t const z = 0;
        std::fill(mInfo, mInfo + (sizeof(uint8_t) * (mMask + 1)), z);

        mInfoInc = InitialInfoInc;
        mInfoHashShift = InitialInfoHashShift;
    }

    // Destroys the map and all it's contents.
    ~unordered_map() {
        destroy();
    }

    // Checks if both maps contain the same entries. Order is irrelevant.
    bool operator==(const unordered_map& other) const {
        if (other.size() != size()) {
            return false;
        }
        for (auto const& otherEntry : other) {
            auto const myIt = find(otherEntry.first);
            if (myIt == end() || !(myIt->second == otherEntry.second)) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const unordered_map& other) const {
        return !operator==(other);
    }

    mapped_type& operator[](const key_type& key) {
        return doCreateByKey(key);
    }

    mapped_type& operator[](key_type&& key) {
        return doCreateByKey(std::move(key));
    }

    template <typename Iter>
    void insert(Iter first, Iter last) {
        for (; first != last; ++first) {
            // value_type ctor needed because this might be called with std::pair's
            insert(value_type(*first));
        }
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        Node n{*this, std::forward<Args>(args)...};
        auto r = doInsert(std::move(n));
        if (!r.second) {
            // insertion not possible: destroy node
            n.destroy(*this);
        }
        return r;
    }

    std::pair<iterator, bool> insert(const value_type& keyval) {
        return doInsert(keyval);
    }

    std::pair<iterator, bool> insert(value_type&& keyval) {
        return doInsert(std::move(keyval));
    }

    // Returns 1 if key is found, 0 otherwise.
    size_t count(const key_type& key) const {
        auto kv = mKeyVals + findIdx(key);
        if (kv != reinterpret_cast<Node*>(mInfo)) {
            return 1;
        }
        return 0;
    }

    // Returns a reference to the value found for key.
    // Throws std::out_of_range if element cannot be found
    mapped_type& at(key_type const& key) {
        auto kv = mKeyVals + findIdx(key);
        if (kv == reinterpret_cast<Node*>(mInfo)) {
            doThrow<std::out_of_range>("key not found");
        }
        return kv->getSecond();
    }

    // Returns a reference to the value found for key.
    // Throws std::out_of_range if element cannot be found
    mapped_type const& at(key_type const& key) const {
        auto kv = mKeyVals + findIdx(key);
        if (kv == reinterpret_cast<Node*>(mInfo)) {
            doThrow<std::out_of_range>("key not found");
        }
        return kv->getSecond();
    }

    const_iterator find(const key_type& key) const {
        const size_t idx = findIdx(key);
        return const_iterator{mKeyVals + idx, mInfo + idx};
    }

    template <typename OtherKey>
    const_iterator find(const OtherKey& key, is_transparent_tag /*unused*/) const {
        const size_t idx = findIdx(key);
        return const_iterator{mKeyVals + idx, mInfo + idx};
    }

    iterator find(const key_type& key) {
        const size_t idx = findIdx(key);
        return iterator{mKeyVals + idx, mInfo + idx};
    }

    template <typename OtherKey>
    iterator find(const OtherKey& key, is_transparent_tag /*unused*/) {
        const size_t idx = findIdx(key);
        return iterator{mKeyVals + idx, mInfo + idx};
    }

    iterator begin() {
        if (empty()) {
            return end();
        }
        return iterator(mKeyVals, mInfo, fast_forward_tag{});
    }
    const_iterator begin() const {
        return cbegin();
    }
    const_iterator cbegin() const {
        if (empty()) {
            return cend();
        }
        return const_iterator(mKeyVals, mInfo, fast_forward_tag{});
    }

    iterator end() {
        // no need to supply valid info pointer: end() must not be dereferenced, and only node
        // pointer is compared.
        return iterator{reinterpret_cast<Node*>(mInfo), nullptr};
    }
    const_iterator end() const {
        return cend();
    }
    const_iterator cend() const {
        return const_iterator{reinterpret_cast<Node*>(mInfo), nullptr};
    }

    iterator erase(const_iterator pos) {
        // its safe to perform const cast here
        return erase(iterator{const_cast<Node*>(pos.mKeyVals), const_cast<uint8_t*>(pos.mInfo)});
    }

    // Erases element at pos, returns iterator to the next element.
    iterator erase(iterator pos) {
        // we assume that pos always points to a valid entry, and not end().
        auto const idx = static_cast<size_t>(pos.mKeyVals - mKeyVals);

        shiftDown(idx);
        --mNumElements;

        if (*pos.mInfo) {
            // we've backward shifted, return this again
            return pos;
        }

        // no backward shift, return next element
        return ++pos;
    }

    size_t erase(const key_type& key) {
        size_t idx;
        InfoType info;
        keyToIdx(key, idx, info);

        // check while info matches with the source idx
        do {
            if (info == mInfo[idx] && KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                shiftDown(idx);
                --mNumElements;
                return 1;
            }
            next(&info, &idx);
        } while (info <= mInfo[idx]);

        // nothing found to delete
        return 0;
    }

    void reserve(size_t count) {
        auto newSize = InitialNumElements > mMask + 1 ? InitialNumElements : mMask + 1;
        while (calcMaxNumElementsAllowed(newSize) < count && newSize != 0) {
            newSize *= 2;
        }
        if (ROBIN_HOOD_UNLIKELY(newSize == 0)) {
            throwOverflowError();
        }

        rehash(newSize);
    }

    void rehash(size_t numBuckets) {
        if (ROBIN_HOOD_UNLIKELY((numBuckets & (numBuckets - 1)) != 0)) {
            doThrow<std::runtime_error>("rehash only allowed for power of two");
        }

        Node* const oldKeyVals = mKeyVals;
        uint8_t const* const oldInfo = mInfo;

        const size_t oldMaxElements = mMask + 1;

        // resize operation: move stuff
        init_data(numBuckets);
        if (oldMaxElements > 1) {
            for (size_t i = 0; i < oldMaxElements; ++i) {
                if (oldInfo[i] != 0) {
                    insert_move(std::move(oldKeyVals[i]));
                    // destroy the node but DON'T destroy the data.
                    oldKeyVals[i].~Node();
                }
            }

            // don't destroy old data: put it into the pool instead
            DataPool::addOrFree(oldKeyVals, calcNumBytesTotal(oldMaxElements));
        }
    }

    size_type size() const {
        return mNumElements;
    }

    size_type max_size() const {
        return static_cast<size_type>(-1);
    }

    bool empty() const {
        return 0 == mNumElements;
    }

    float max_load_factor() const {
        return MaxLoadFactor100 / 100.0f;
    }

    // Average number of elements per bucket. Since we allow only 1 per bucket
    float load_factor() const {
        return static_cast<float>(size()) / (mMask + 1);
    }

    size_t mask() const {
        return mMask;
    }

private:
    ROBIN_HOOD_NOINLINE void throwOverflowError() const {
        throw std::overflow_error("robin_hood::map overflow");
    }

    void init_data(size_t max_elements) {
        mNumElements = 0;
        mMask = max_elements - 1;
        mMaxNumElementsAllowed = calcMaxNumElementsAllowed(max_elements);

        // calloc also zeroes everything
        mKeyVals = reinterpret_cast<Node*>(
            detail::assertNotNull<std::bad_alloc>(calloc(1, calcNumBytesTotal(max_elements))));
        mInfo = reinterpret_cast<uint8_t*>(mKeyVals + max_elements);

        // set sentinel
        mInfo[max_elements] = 1;

        mInfoInc = InitialInfoInc;
        mInfoHashShift = InitialInfoHashShift;
    }

    template <typename Arg>
    mapped_type& doCreateByKey(Arg&& key) {
        while (true) {
            size_t idx;
            InfoType info;
            keyToIdx(key, idx, info);
            nextWhileLess(&info, &idx);

            // while we potentially have a match. Can't do a do-while here because when mInfo is 0
            // we don't want to skip forward
            while (info == mInfo[idx]) {
                if (KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                    // key already exists, do not insert.
                    return mKeyVals[idx].getSecond();
                }
                next(&info, &idx);
            }

            // unlikely that this evaluates to true
            if (ROBIN_HOOD_UNLIKELY(mNumElements >= mMaxNumElementsAllowed)) {
                increase_size();
                continue;
            }

            // key not found, so we are now exactly where we want to insert it.
            auto const insertion_idx = idx;
            auto const insertion_info = info;
            if (ROBIN_HOOD_UNLIKELY(insertion_info + mInfoInc > 0xFF)) {
                mMaxNumElementsAllowed = 0;
            }

            // find an empty spot
            while (0 != mInfo[idx]) {
                next(&info, &idx);
            }

            auto& l = mKeyVals[insertion_idx];
            if (idx == insertion_idx) {
                // put at empty spot. This forwards all arguments into the node where the object is
                // constructed exactly where it is needed.
                ::new (static_cast<void*>(&l))
                    Node(*this, std::piecewise_construct,
                         std::forward_as_tuple(std::forward<Arg>(key)), std::forward_as_tuple());
            } else {
                shiftUp(idx, insertion_idx);
                l = Node(*this, std::piecewise_construct,
                         std::forward_as_tuple(std::forward<Arg>(key)), std::forward_as_tuple());
            }

            // mKeyVals[idx].getFirst() = std::move(key);
            mInfo[insertion_idx] = static_cast<uint8_t>(insertion_info);

            ++mNumElements;
            return mKeyVals[insertion_idx].getSecond();
        }
    }

    // This is exactly the same code as operator[], except for the return values
    template <typename Arg>
    std::pair<iterator, bool> doInsert(Arg&& keyval) {
        while (true) {
            size_t idx;
            InfoType info;
            keyToIdx(keyval.getFirst(), idx, info);
            nextWhileLess(&info, &idx);

            // while we potentially have a match
            while (info == mInfo[idx]) {
                if (KeyEqual::operator()(keyval.getFirst(), mKeyVals[idx].getFirst())) {
                    // key already exists, do NOT insert.
                    // see http://en.cppreference.com/w/cpp/container/unordered_map/insert
                    return std::make_pair<iterator, bool>(iterator(mKeyVals + idx, mInfo + idx),
                                                          false);
                }
                next(&info, &idx);
            }

            // unlikely that this evaluates to true
            if (ROBIN_HOOD_UNLIKELY(mNumElements >= mMaxNumElementsAllowed)) {
                increase_size();
                continue;
            }

            // key not found, so we are now exactly where we want to insert it.
            auto const insertion_idx = idx;
            auto const insertion_info = info;
            if (ROBIN_HOOD_UNLIKELY(insertion_info + mInfoInc > 0xFF)) {
                mMaxNumElementsAllowed = 0;
            }

            // find an empty spot
            while (0 != mInfo[idx]) {
                next(&info, &idx);
            }

            auto& l = mKeyVals[insertion_idx];
            if (idx == insertion_idx) {
                ::new (static_cast<void*>(&l)) Node(*this, std::forward<Arg>(keyval));
            } else {
                shiftUp(idx, insertion_idx);
                l = Node(*this, std::forward<Arg>(keyval));
            }

            // put at empty spot
            mInfo[insertion_idx] = static_cast<uint8_t>(insertion_info);

            ++mNumElements;
            return std::make_pair(iterator(mKeyVals + insertion_idx, mInfo + insertion_idx), true);
        }
    }

    size_t calcMaxNumElementsAllowed(size_t maxElements) {
        static constexpr size_t overflowLimit = (std::numeric_limits<size_t>::max)() / 100;
        static constexpr double factor = MaxLoadFactor100 / 100.0;

        // make sure we can't get an overflow; use floatingpoint arithmetic if necessary.
        if (maxElements > overflowLimit) {
            return static_cast<size_t>(static_cast<double>(maxElements) * factor);
        } else {
            return (maxElements * MaxLoadFactor100) / 100;
        }
    }

    bool try_increase_info() {
        ROBIN_HOOD_LOG("mInfoInc=" << mInfoInc << ", numElements=" << mNumElements
                                   << ", maxNumElementsAllowed="
                                   << calcMaxNumElementsAllowed(mMask + 1));
        if (mInfoInc <= 2) {
            // need to be > 2 so that shift works (otherwise undefined behavior!)
            return false;
        }
        // we got space left, try to make info smaller
        mInfoInc = static_cast<uint8_t>(mInfoInc >> 1);

        // remove one bit of the hash, leaving more space for the distance info.
        // This is extremely fast because we can operate on 8 bytes at once.
        ++mInfoHashShift;
        auto const data = reinterpret_cast<uint64_t*>(mInfo);
        auto const numEntries = (mMask + 1) / 8;

        for (size_t i = 0; i < numEntries; ++i) {
            data[i] = (data[i] >> 1) & UINT64_C(0x7f7f7f7f7f7f7f7f);
        }
        mMaxNumElementsAllowed = calcMaxNumElementsAllowed(mMask + 1);
        return true;
    }

    void increase_size() {
        // nothing allocated yet? just allocate InitialNumElements
        if (0 == mMask) {
            init_data(InitialNumElements);
            return;
        }

        auto const maxNumElementsAllowed = calcMaxNumElementsAllowed(mMask + 1);
        if (mNumElements < maxNumElementsAllowed && try_increase_info()) {
            return;
        }

        ROBIN_HOOD_LOG("mNumElements=" << mNumElements << ", maxNumElementsAllowed="
                                       << maxNumElementsAllowed << ", load="
                                       << (static_cast<double>(mNumElements) * 100.0 /
                                           (static_cast<double>(mMask) + 1)));
        // it seems we have a really bad hash function! don't try to resize again
        if (mNumElements * 2 < calcMaxNumElementsAllowed(mMask + 1)) {
            throwOverflowError();
        }

        rehash((mMask + 1) * 2);
    }

    void destroy() {
        if (0 == mMask) {
            // don't deallocate!
            return;
        }

        Destroyer<Self, IsFlatMap && std::is_trivially_destructible<Node>::value>{}
            .nodesDoNotDeallocate(*this);
        free(mKeyVals);
    }

    // members are sorted so no padding occurs
    Node* mKeyVals = reinterpret_cast<Node*>(&mMask);    // 8 byte  8
    uint8_t* mInfo = reinterpret_cast<uint8_t*>(&mMask); // 8 byte 16
    size_t mNumElements = 0;                             // 8 byte 24
    size_t mMask = 0;                                    // 8 byte 32
    size_t mMaxNumElementsAllowed = 0;                   // 8 byte 40
    InfoType mInfoInc = InitialInfoInc;                  // 4 byte 44
    InfoType mInfoHashShift = InitialInfoHashShift;      // 4 byte 48
                                                         // 16 byte 56 if NodeAllocator
};

} // namespace detail

template <typename Key, typename T, typename Hash = hash<Key>,
          typename KeyEqual = std::equal_to<Key>, size_t MaxLoadFactor100 = 80>
using unordered_flat_map = detail::unordered_map<true, MaxLoadFactor100, Key, T, Hash, KeyEqual>;

template <typename Key, typename T, typename Hash = hash<Key>,
          typename KeyEqual = std::equal_to<Key>, size_t MaxLoadFactor100 = 80>
using unordered_node_map = detail::unordered_map<false, MaxLoadFactor100, Key, T, Hash, KeyEqual>;

template <typename Key, typename T, typename Hash = hash<Key>,
          typename KeyEqual = std::equal_to<Key>, size_t MaxLoadFactor100 = 80>
using unordered_map =
    detail::unordered_map<sizeof(robin_hood::pair<Key, T>) <= sizeof(size_t) * 6 &&
                              std::is_nothrow_move_constructible<robin_hood::pair<Key, T>>::value &&
                              std::is_nothrow_move_assignable<robin_hood::pair<Key, T>>::value,
                          MaxLoadFactor100, Key, T, Hash, KeyEqual>;

} // namespace robin_hood

#endif
