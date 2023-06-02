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

#ifndef RMLUI_CONFIG_CONFIG_H
#define RMLUI_CONFIG_CONFIG_H

/*
 * This file provides the means to configure various types used across RmlUi. It is possible to override container
 * types with your own, provided they are compatible with STL, or customize STL containers, for example by setting
 * custom allocators. This file may be edited directly, or can be copied to an alternate location, modified, and
 * included by setting the CMake option CUSTOM_CONFIGURATION_FILE (RMLUI_CUSTOM_CONFIGURATION_FILE preprocessor
 * define) to the path of that file.
 */

#ifdef RMLUI_CUSTOM_CONFIGURATION_FILE
	#include RMLUI_CUSTOM_CONFIGURATION_FILE
#else
	#include <array>
	#include <functional>
	#include <list>
	#include <memory>
	#include <queue>
	#include <stack>
	#include <string>
	#include <map>
	#include <unordered_map>
	#include <utility>
	#include <vector>

	#ifdef RMLUI_NO_THIRDPARTY_CONTAINERS
		#include <set>
		#include <unordered_set>
	#else
		#include "../Core/Containers/itlib/flat_map.hpp"
		#include "../Core/Containers/itlib/flat_set.hpp"
		#include "../Core/Containers/robin_hood.h"
	#endif // RMLUI_NO_THIRDPARTY_CONTAINERS

namespace Rml {

	// Default matrix type to be used. This alias may be set to ColumnMajorMatrix4f or RowMajorMatrix4f. This alias can not
	// be set here because matrix types are defined after this include in Core/Types.h.
	#ifdef RMLUI_MATRIX_ROW_MAJOR
		#define RMLUI_MATRIX4_TYPE RowMajorMatrix4f
	#else
		#define RMLUI_MATRIX4_TYPE ColumnMajorMatrix4f
	#endif

	// A way to disable 'final' specified for Rml::Releaser class. It breaks EASTL.
	#define RMLUI_RELEASER_FINAL final

// Containers types.
template <typename T>
using Vector = std::vector<T>;
template <typename T, size_t N = 1>
using Array = std::array<T, N>;
template <typename T>
using Stack = std::stack<T>;
template <typename T>
using List = std::list<T>;
template <typename T>
using Queue = std::queue<T>;
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;
template <typename Key, typename Value>
using StableMap = std::map<Key, Value>;
template <typename Key, typename Value>
using UnorderedMultimap = std::unordered_multimap<Key, Value>;

	#ifdef RMLUI_NO_THIRDPARTY_CONTAINERS
template <typename Key, typename Value>
using UnorderedMap = std::unordered_map<Key, Value>;
template <typename Key, typename Value>
using SmallUnorderedMap = UnorderedMap<Key, Value>;
template <typename T>
using UnorderedSet = std::unordered_set<T>;
template <typename T>
using SmallUnorderedSet = std::unordered_set<T>;
template <typename T>
using SmallOrderedSet = std::set<T>;
	#else
template <typename Key, typename Value>
using UnorderedMap = robin_hood::unordered_flat_map<Key, Value>;
template <typename Key, typename Value>
using SmallUnorderedMap = itlib::flat_map<Key, Value>;
template <typename T>
using UnorderedSet = robin_hood::unordered_flat_set<T>;
template <typename T>
using SmallUnorderedSet = itlib::flat_set<T>;
template <typename T>
using SmallOrderedSet = itlib::flat_set<T>;
	#endif // RMLUI_NO_THIRDPARTY_CONTAINERS

// Utilities.
template <typename T>
using Hash = std::hash<T>;
template <typename T>
using Function = std::function<T>;
template <typename Iterator>
inline std::move_iterator<Iterator> MakeMoveIterator(Iterator it)
{
	return std::make_move_iterator(it);
}

// Strings.
using String = std::string;
using StringList = Vector<String>;

// Smart pointer types.
template <typename T>
using UniquePtr = std::unique_ptr<T>;
template <typename T>
class Releaser;
template <typename T>
using UniqueReleaserPtr = std::unique_ptr<T, Releaser<T>>;
template <typename T>
using SharedPtr = std::shared_ptr<T>;
template <typename T>
using WeakPtr = std::weak_ptr<T>;
template <typename T, typename... Args>
inline SharedPtr<T> MakeShared(Args&&... args)
{
	return std::make_shared<T, Args...>(std::forward<Args>(args)...);
}
template <typename T, typename... Args>
inline UniquePtr<T> MakeUnique(Args&&... args)
{
	return std::make_unique<T, Args...>(std::forward<Args>(args)...);
}

} // namespace Rml

	/***
	// The following defines should be used for inserting custom type cast operators for conversion of RmlUi types
	// to user types. RmlUi uses template math types, therefore conversion operators to non-templated types
	// should be done using SFINAE as in example below.

	// Extra code to be inserted into RmlUi::Color<> class body. Note: be mindful of colorspaces used by different
	// color types. RmlUi assumes that float colors are interpreted in linear colorspace while byte colors are
	// interpreted as sRGB.
	#define RMLUI_COLOUR_USER_EXTRA                                                                         \
	    template<typename U = ColourType, typename std::enable_if_t<std::is_same_v<U, byte>>* = nullptr>    \
	    operator MyColor() const { return MyColor(                                                          \
	        (float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, (float)alpha / 255.0f); }     \
	    template<typename U = ColourType, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>   \
	    operator MyColor() const { return MyColor(red, green, blue, alpha); }                               \

	// Extra code to be inserted into RmlUi::Vector2<> class body.
	#define RMLUI_VECTOR2_USER_EXTRA                                                                        \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>           \
	    operator typename MyIntVector2() const { return MyIntVector2(x, y); }                               \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>         \
	    operator MyVector2() const { return MyVector2(x, y); }                                              \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>           \
	    Vector2(MyIntVector2 value) : Vector2(value.x_, value.y_) { }                                       \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>         \
	    Vector2(MyVector2 value) : Vector2(value.x_, value.y_) { }                                          \

	// Extra code to be inserted into RmlUi::Vector3<> class body.
	#define RMLUI_VECTOR3_USER_EXTRA                                                                        \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>           \
	    operator typename MyIntVector3() const { return MyIntVector3(x, y, z); }                            \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>         \
	    operator MyVector3() const { return MyVector3(x, y, z); }                                           \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>           \
	    Vector3(MyIntVector3 value) : Vector3(value.x_, value.y_, value.z_) { }                             \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>         \
	    Vector3(MyVector3 value) : Vector3(value.x_, value.y_, value.z_) { }                                \

	// Extra code to be inserted into RmlUi::Vector4<> class body.
	#define RMLUI_VECTOR4_USER_EXTRA                                                                        \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>           \
	    operator typename MyIntVector4() const { return MyIntVector4(x, y, z, w); }                         \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>         \
	    operator MyVector4() const { return MyVector4(x, y, z, w); }                                        \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>           \
	    Vector4(MyIntVector4 value) : Vector4(value.x_, value.y_, value.z_, value.w_) { }                   \
	    template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>         \
	    Vector4(MyVector4 value) : Vector4(value.x_, value.y_, value.z_, value.w_) { }                      \

	// Extra code to be inserted into RmlUi::Matrix4<> class body.
	    #define RMLUI_MATRIX4_USER_EXTRA operator MyMatrix4() const { return MyMatrix4(data()); }
	***/

#endif     // RMLUI_USER_CONFIG_FILE

#endif     // RMLUI_CONFIG_CONFIG_H
