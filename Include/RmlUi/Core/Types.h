/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICORETYPES_H
#define RMLUICORETYPES_H

#include <string>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

#include "Traits.h"

#ifdef RMLUI_NO_THIRDPARTY_CONTAINERS
#include <set>
#include <unordered_set>
#include <unordered_map>
#else
#include "Containers/chobo/flat_map.hpp"
#include "Containers/chobo/flat_set.hpp"
#include "Containers/robin_hood.h"
#endif

namespace Rml {
namespace Core {

// Commonly used basic types
using byte = unsigned char;
using ScriptObject = void*;
using std::size_t;

// Unicode code point
enum class Character : char32_t { Null, Replacement = 0xfffd };

}
}

#include "Colour.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4.h"
#include "ObserverPtr.h"

namespace Rml {
namespace Core {


// Color and linear algebra
using Colourf = Colour< float, 1 >;
using Colourb = Colour< byte, 255 >;
using Vector2i = Vector2< int >;
using Vector2f = Vector2< float >;
using Vector3i = Vector3< int >;
using Vector3f = Vector3< float >;
using Vector4i = Vector4< int >;
using Vector4f = Vector4< float >;
using ColumnMajorMatrix4f = Matrix4< float, ColumnMajorStorage< float > >;
using RowMajorMatrix4f = Matrix4< float, RowMajorStorage< float > >;
using Matrix4f = ColumnMajorMatrix4f;


// Common classes
class Element;
class ElementInstancer;
class ElementAnimation;
class Context;
class Event;
class Property;
class Variant;
class Transform;
class PropertyIdSet;
class Decorator;
class FontEffect;
struct Animation;
struct Transition;
struct TransitionList;
struct Rectangle;
enum class EventId : uint16_t;
enum class PropertyId : uint8_t;

// Types for external interfaces.
using FileHandle = uintptr_t;
using TextureHandle = uintptr_t;
using CompiledGeometryHandle = uintptr_t;
using DecoratorDataHandle = uintptr_t;
using FontFaceHandle = uintptr_t;
using FontEffectsHandle = uintptr_t;

// Strings
using String = std::string;
using StringList = std::vector< String >;
using U16String = std::u16string;

// Smart pointer types
template<typename T>
using UniquePtr = std::unique_ptr<T>;
template<typename T>
using UniqueReleaserPtr = std::unique_ptr<T, Releaser<T>>;
template<typename T>
using SharedPtr = std::shared_ptr<T>;
template<typename T>
using WeakPtr = std::weak_ptr<T>;

using ElementPtr = UniqueReleaserPtr<Element>;
using ContextPtr = UniqueReleaserPtr<Context>;
using EventPtr = UniqueReleaserPtr<Event>;

// Containers
#ifdef RMLUI_NO_THIRDPARTY_CONTAINERS
template <typename Key, typename Value>
using UnorderedMap = std::unordered_map< Key, Value >;
template <typename Key, typename Value>
using SmallUnorderedMap = UnorderedMap< Key, Value >;
template <typename T>
using UnorderedSet = std::unordered_set< T >;
template <typename T>
using SmallUnorderedSet = std::unordered_set< T >;
template <typename T>
using SmallOrderedSet = std::set< T >;
#else
template < typename Key, typename Value>
using UnorderedMap = robin_hood::unordered_flat_map< Key, Value >;
template <typename Key, typename Value>
using SmallUnorderedMap = chobo::flat_map< Key, Value >;
template <typename T>
using UnorderedSet = robin_hood::unordered_flat_set< T >;
template <typename T>
using SmallUnorderedSet = chobo::flat_set< T >;
template <typename T>
using SmallOrderedSet = chobo::flat_set< T >;
#endif


// Container types for common classes
using ElementList = std::vector< Element* >;
using OwnedElementList = std::vector< ElementPtr >;
using ElementAnimationList = std::vector< ElementAnimation >;

using PseudoClassList = SmallUnorderedSet< String >;
using AttributeNameList = SmallUnorderedSet< String >;
using PropertyMap = UnorderedMap< PropertyId, Property >;

using Dictionary = SmallUnorderedMap< String, Variant >;
using ElementAttributes = Dictionary;
using XMLAttributes = Dictionary;

using AnimationList = std::vector<Animation>;
using DecoratorList = std::vector<SharedPtr<const Decorator>>;
using FontEffectList = std::vector<SharedPtr<const FontEffect>>;

struct Decorators {
	DecoratorList list;
	String value;
};
struct FontEffects {
	FontEffectList list;
	String value;
};

// Additional smart pointers
using TransformPtr = SharedPtr< Transform >;
using DecoratorsPtr = SharedPtr<const Decorators>;
using FontEffectsPtr = SharedPtr<const FontEffects>;

}
}

namespace std {
// Hash specialization for enum class types (required on some older compilers)
template <> struct hash<::Rml::Core::PropertyId> {
	using utype = typename ::std::underlying_type<::Rml::Core::PropertyId>::type;
	size_t operator() (const ::Rml::Core::PropertyId& t) const { ::std::hash<utype> h; return h(static_cast<utype>(t)); }
};
template <> struct hash<::Rml::Core::Character> {
    using utype = typename ::std::underlying_type<::Rml::Core::Character>::type;
    size_t operator() (const ::Rml::Core::Character& t) const { ::std::hash<utype> h; return h(static_cast<utype>(t)); }
};
}

#endif
