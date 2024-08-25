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

#ifndef RMLUI_CORE_STYLESHEETTYPES_H
#define RMLUI_CORE_STYLESHEETTYPES_H

#include "Factory.h"
#include "PropertyDictionary.h"
#include "Types.h"
#include "Utilities.h"

namespace Rml {

class Decorator;
class DecoratorInstancer;
class StyleSheet;
class StyleSheetNode;

struct KeyframeBlock {
	KeyframeBlock(float normalized_time) : normalized_time(normalized_time) {}
	float normalized_time; // [0, 1]
	PropertyDictionary properties;
};
struct Keyframes {
	Vector<PropertyId> property_ids;
	Vector<KeyframeBlock> blocks;
};
using KeyframesMap = UnorderedMap<String, Keyframes>;

struct NamedDecorator {
	String type;
	DecoratorInstancer* instancer;
	PropertyDictionary properties;
};
using NamedDecoratorMap = UnorderedMap<String, NamedDecorator>;

struct DecoratorDeclaration {
	String type;
	DecoratorInstancer* instancer;
	PropertyDictionary properties;
	BoxArea paint_area;
};
struct DecoratorDeclarationList {
	Vector<DecoratorDeclaration> list;
	String value;
};
struct FilterDeclaration {
	String type;
	FilterInstancer* instancer;
	PropertyDictionary properties;
};
struct FilterDeclarationList {
	Vector<FilterDeclaration> list;
	String value;
};

enum class MediaQueryModifier {
	None,
	Not // passes only if the query is false instead of true
};

struct MediaBlock {
	MediaBlock() {}
	MediaBlock(PropertyDictionary _properties, SharedPtr<StyleSheet> _stylesheet, MediaQueryModifier _modifier) :
		properties(std::move(_properties)), stylesheet(std::move(_stylesheet)), modifier(_modifier)
	{}

	PropertyDictionary properties; // Media query properties
	SharedPtr<StyleSheet> stylesheet;
	MediaQueryModifier modifier = MediaQueryModifier::None;
};
using MediaBlockList = Vector<MediaBlock>;

/**
   StyleSheetIndex contains a cached index of all styled nodes for quick lookup when finding applicable style nodes for the current state of a given
   element.
 */
struct StyleSheetIndex {
	using NodeList = Vector<const StyleSheetNode*>;
	using NodeIndex = UnorderedMap<size_t, NodeList>;

	// The following objects are given in prioritized order. Any nodes in the first object will not be contained in the next one and so on.
	NodeIndex ids, classes, tags;
	NodeList other;
};
} // namespace Rml

namespace std {
// Hash specialization for the node list, so it can be used as key in UnorderedMap.
template <>
struct hash<::Rml::StyleSheetIndex::NodeList> {
	size_t operator()(const ::Rml::StyleSheetIndex::NodeList& nodes) const noexcept
	{
		size_t seed = 0;
		for (const ::Rml::StyleSheetNode* node : nodes)
			::Rml::Utilities::HashCombine(seed, node);
		return seed;
	}
};
} // namespace std
#endif
