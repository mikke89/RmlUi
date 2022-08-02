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

#ifndef RMLUI_CORE_STYLESHEETSELECTOR_H
#define RMLUI_CORE_STYLESHEETSELECTOR_H

#include "../../Include/RmlUi/Core/Types.h"
#include <tuple>

namespace Rml {

class Element;
class StyleSheetNode;
struct SelectorTree;

namespace SelectorSpecificity {
	enum {
		// Constants used to determine the specificity of a selector.
		Tag = 10'000,
		Class = 100'000,
		PseudoClass = Class,
		ID = 1'000'000,
	};
}

enum class StructuralSelectorType {
	Invalid,
	Nth_Child,
	Nth_Last_Child,
	Nth_Of_Type,
	Nth_Last_Of_Type,
	First_Child,
	Last_Child,
	First_Of_Type,
	Last_Of_Type,
	Only_Child,
	Only_Of_Type,
	Empty,
	Not
};

struct StructuralSelector {
	StructuralSelector(StructuralSelectorType type, int a, int b) : type(type), a(a), b(b) {}
	StructuralSelector(StructuralSelectorType type, SharedPtr<const SelectorTree> tree, int specificity) :
		type(type), specificity(specificity), selector_tree(std::move(tree))
	{}

	StructuralSelectorType type = StructuralSelectorType::Invalid;

	// For counting selectors, the following are the 'a' and 'b' variables of an + b.
	int a = 0;
	int b = 0;

	// Specificity is usually determined like a pseudo class, but some types override this value.
	int specificity = SelectorSpecificity::PseudoClass;

	// For selectors that contain internal selectors such as :not().
	SharedPtr<const SelectorTree> selector_tree;
};

inline bool operator==(const StructuralSelector& a, const StructuralSelector& b)
{
	// Currently sub-selectors (selector_tree) are only superficially compared. This mainly has the consequence that selectors with a sub-selector
	// which are instantiated separately will never compare equal, even if they have the exact same sub-selector expression. This further results in
	// such selectors not being de-duplicated. This should not lead to any functional differences but leads to potentially missed memory/performance
	// optimizations. E.g. 'div a, div b' will combine the two div nodes, while ':not(div) a, :not(div) b' will not combine the two not-div nodes.
	return a.type == b.type && a.a == b.a && a.b == b.b && a.selector_tree == b.selector_tree;
}
inline bool operator<(const StructuralSelector& a, const StructuralSelector& b)
{
	return std::tie(a.type, a.a, a.b, a.selector_tree) < std::tie(b.type, b.a, b.b, b.selector_tree);
}

// A tree of unstyled style sheet nodes.
struct SelectorTree {
	UniquePtr<StyleSheetNode> root;
	Vector<StyleSheetNode*> leafs; // Owned by root.
};

enum class SelectorCombinator : byte {
	Descendant,        // The 'E F' (whitespace) combinator: Matches if F is a descendant of E.
	Child,             // The 'E > F' combinator: Matches if F is a child of E.
	NextSibling,       // The 'E + F' combinator: Matches if F is immediately preceded by E.
	SubsequentSibling, // The 'E ~ F' combinator: Matches if F is preceded by E.
};

/// Returns true if the the node the given selector is discriminating for is applicable to a given element.
/// @param element[in] The element to determine node applicability for.
/// @param selector[in] The selector to test against the element.
bool IsSelectorApplicable(const Element* element, const StructuralSelector& selector);

} // namespace Rml
#endif
