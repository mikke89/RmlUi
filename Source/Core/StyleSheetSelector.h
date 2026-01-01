#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;
class StyleSheetNode;

/**
    Constants used to determine the specificity of a selector.
 */
namespace SelectorSpecificity {
	enum {
		Tag = 10'000,
		Class = 100'000,
		Attribute = Class,
		PseudoClass = Class,
		ID = 1'000'000,
	};
}

/**
    Combinator determines how two connected compound selectors are matched against the element hierarchy.
 */
enum class SelectorCombinator {
	Descendant,        // The 'E F' (whitespace) combinator: Matches if F is a descendant of E.
	Child,             // The 'E > F' combinator: Matches if F is a child of E.
	NextSibling,       // The 'E + F' combinator: Matches if F is immediately preceded by E.
	SubsequentSibling, // The 'E ~ F' combinator: Matches if F is preceded by E.
};

/**
    Attribute selector.

    Such as [unit], [unit=meter], [href^=http]
 */
enum class AttributeSelectorType {
	Always,
	Equal = '=',
	InList = '~',
	BeginsWithThenHyphen = '|',
	BeginsWith = '^',
	EndsWith = '$',
	Contains = '*',
};
struct AttributeSelector {
	AttributeSelectorType type = AttributeSelectorType::Always;
	String name;
	String value;
};
bool operator==(const AttributeSelector& a, const AttributeSelector& b);
bool operator<(const AttributeSelector& a, const AttributeSelector& b);
using AttributeSelectorList = Vector<AttributeSelector>;

/**
    A tree of unstyled style sheet nodes.
 */
struct SelectorTree {
	UniquePtr<StyleSheetNode> root;
	Vector<StyleSheetNode*> leafs; // Owned by root.
};

/**
    Tree-structural selector.

    Such as :nth-child(2n+1), :empty(), :not(div)
 */
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
	Not,
	Scope,
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
bool operator==(const StructuralSelector& a, const StructuralSelector& b);
bool operator<(const StructuralSelector& a, const StructuralSelector& b);
using StructuralSelectorList = Vector<StructuralSelector>;

/**
    Compound selector contains all the basic selectors for a single node.

    Such as div#foo.bar:nth-child(2)
 */
struct CompoundSelector {
	String tag;
	String id;
	StringList class_names;
	StringList pseudo_class_names;
	AttributeSelectorList attributes;
	StructuralSelectorList structural_selectors;
	SelectorCombinator combinator = SelectorCombinator::Descendant; // Determines how to match with our parent node.
};
bool operator==(const CompoundSelector& a, const CompoundSelector& b);

/// Returns true if the node the given selector is discriminating for is applicable to a given element.
/// @param element[in] The element to determine node applicability for.
/// @param selector[in] The selector to test against the element.
/// @param scope[in] The element considered as the reference point/scope (for :scope).
bool IsSelectorApplicable(const Element* element, const StructuralSelector& selector, const Element* scope);

} // namespace Rml
