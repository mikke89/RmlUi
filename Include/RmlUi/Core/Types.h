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

#ifndef RMLUI_CORE_TYPES_H
#define RMLUI_CORE_TYPES_H

#include "../Config/Config.h"
#include "Traits.h"
#include <cstdlib>
#include <memory>
#include <stddef.h>
#include <stdint.h>

namespace Rml {

// Commonly used basic types
using byte = unsigned char;
using ScriptObject = void*;

enum class Character : char32_t { Null, Replacement = 0xfffd }; // Unicode code point
enum class BoxArea { Margin, Border, Padding, Content, Auto };

} // namespace Rml

#include "Colour.h"
#include "Matrix4.h"
#include "ObserverPtr.h"
#include "Rectangle.h"
#include "Span.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

namespace Rml {

// Color and linear algebra
enum class ColorFormat { RGBA8, A8 };
using Colourf = Colour<float, 1, false>;
using Colourb = Colour<byte, 255, false>;
using ColourbPremultiplied = Colour<byte, 255, true>;
using Vector2i = Vector2<int>;
using Vector2f = Vector2<float>;
using Vector3i = Vector3<int>;
using Vector3f = Vector3<float>;
using Vector4i = Vector4<int>;
using Vector4f = Vector4<float>;
using Rectanglei = Rectangle<int>;
using Rectanglef = Rectangle<float>;
using ColumnMajorMatrix4f = Matrix4<float, ColumnMajorStorage<float>>;
using RowMajorMatrix4f = Matrix4<float, RowMajorStorage<float>>;
using Matrix4f = RMLUI_MATRIX4_TYPE;

// Common classes
class Element;
class ElementInstancer;
class ElementAnimation;
class RenderManager;
class Texture;
class Context;
class Event;
class Property;
class Variant;
class Transform;
class PropertyIdSet;
class Decorator;
class FontEffect;
class StringView;
struct Animation;
struct Transition;
struct TransitionList;
struct DecoratorDeclarationList;
struct FilterDeclarationList;
struct ColorStop;
struct BoxShadow;
enum class EventId : uint16_t;
enum class PropertyId : uint8_t;
enum class MediaQueryId : uint8_t;
enum class FamilyId : int;

// Types for external interfaces.
using FileHandle = uintptr_t;
using TextureHandle = uintptr_t;
using CompiledGeometryHandle = uintptr_t;
using CompiledFilterHandle = uintptr_t;
using CompiledShaderHandle = uintptr_t;
using DecoratorDataHandle = uintptr_t;
using FontFaceHandle = uintptr_t;
using FontEffectsHandle = uintptr_t;
using LayerHandle = uintptr_t;

using ElementPtr = UniqueReleaserPtr<Element>;
using ContextPtr = UniqueReleaserPtr<Context>;
using EventPtr = UniqueReleaserPtr<Event>;

enum class StableVectorIndex : uint32_t { Invalid = uint32_t(-1) };
enum class TextureFileIndex : uint32_t { Invalid = uint32_t(-1) };

// Container types for common classes
using ElementList = Vector<Element*>;
using OwnedElementList = Vector<ElementPtr>;
using VariantList = Vector<Variant>;
using ElementAnimationList = Vector<ElementAnimation>;

using AttributeNameList = SmallUnorderedSet<String>;
using PropertyMap = UnorderedMap<PropertyId, Property>;

using Dictionary = SmallUnorderedMap<String, Variant>;
using ElementAttributes = Dictionary;
using XMLAttributes = Dictionary;

using AnimationList = Vector<Animation>;
using FontEffectList = Vector<SharedPtr<const FontEffect>>;
struct FontEffects {
	FontEffectList list;
	String value;
};
using ColorStopList = Vector<ColorStop>;
using BoxShadowList = Vector<BoxShadow>;
using FilterHandleList = Vector<CompiledFilterHandle>;

// Additional smart pointers
using TransformPtr = SharedPtr<Transform>;
using DecoratorsPtr = SharedPtr<const DecoratorDeclarationList>;
using FiltersPtr = SharedPtr<const FilterDeclarationList>;
using FontEffectsPtr = SharedPtr<const FontEffects>;

// Data binding types
class DataView;
using DataViewPtr = UniqueReleaserPtr<DataView>;
class DataController;
using DataControllerPtr = UniqueReleaserPtr<DataController>;

} // namespace Rml

namespace std {
// Hash specialization for enum class types (required on some older compilers)
template <>
struct hash<::Rml::PropertyId> {
	using utype = ::std::underlying_type_t<::Rml::PropertyId>;
	size_t operator()(const ::Rml::PropertyId& t) const noexcept
	{
		::std::hash<utype> h;
		return h(static_cast<utype>(t));
	}
};
template <>
struct hash<::Rml::Character> {
	using utype = ::std::underlying_type_t<::Rml::Character>;
	size_t operator()(const ::Rml::Character& t) const noexcept
	{
		::std::hash<utype> h;
		return h(static_cast<utype>(t));
	}
};
template <>
struct hash<::Rml::FamilyId> {
	using utype = ::std::underlying_type_t<::Rml::FamilyId>;
	size_t operator()(const ::Rml::FamilyId& t) const noexcept
	{
		::std::hash<utype> h;
		return h(static_cast<utype>(t));
	}
};
} // namespace std

#endif
