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

#include "StyleSheetNode.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "StyleSheetFactory.h"
#include "StyleSheetSelector.h"
#include <algorithm>

namespace Rml {

static inline bool IsTextElement(const Element* element)
{
	return element->GetTagName() == "#text";
}

StyleSheetNode::StyleSheetNode()
{
	CalculateAndSetSpecificity();
}

StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, const String& tag, const String& id, const StringList& classes,
	const StringList& pseudo_classes, const StructuralSelectorList& structural_selectors, SelectorCombinator combinator) :
	parent(parent),
	tag(tag), id(id), class_names(classes), pseudo_class_names(pseudo_classes), structural_selectors(structural_selectors), combinator(combinator)
{
	CalculateAndSetSpecificity();
}

StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, String&& tag, String&& id, StringList&& classes, StringList&& pseudo_classes,
	StructuralSelectorList&& structural_selectors, SelectorCombinator combinator) :
	parent(parent),
	tag(std::move(tag)), id(std::move(id)), class_names(std::move(classes)), pseudo_class_names(std::move(pseudo_classes)),
	structural_selectors(std::move(structural_selectors)), combinator(combinator)
{
	CalculateAndSetSpecificity();
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(const StyleSheetNode& other)
{
	// See if we match the target child
	for (const auto& child : children)
	{
		if (child->EqualRequirements(other.tag, other.id, other.class_names, other.pseudo_class_names, other.structural_selectors, other.combinator))
			return child.get();
	}

	// We don't, so create a new child
	auto child = MakeUnique<StyleSheetNode>(this, other.tag, other.id, other.class_names, other.pseudo_class_names, other.structural_selectors,
		other.combinator);
	StyleSheetNode* result = child.get();

	children.push_back(std::move(child));

	return result;
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(String&& tag, String&& id, StringList&& classes, StringList&& pseudo_classes,
	StructuralSelectorList&& structural_pseudo_classes, SelectorCombinator combinator)
{
	// See if we match an existing child
	for (const auto& child : children)
	{
		if (child->EqualRequirements(tag, id, classes, pseudo_classes, structural_pseudo_classes, combinator))
			return child.get();
	}

	// We don't, so create a new child
	auto child = MakeUnique<StyleSheetNode>(this, std::move(tag), std::move(id), std::move(classes), std::move(pseudo_classes),
		std::move(structural_pseudo_classes), combinator);
	StyleSheetNode* result = child.get();

	children.push_back(std::move(child));

	return result;
}

// Merges an entire tree hierarchy into our hierarchy.
void StyleSheetNode::MergeHierarchy(StyleSheetNode* node, int specificity_offset)
{
	RMLUI_ZoneScoped;

	// Merge the other node's properties into ours.
	properties.Merge(node->properties, specificity_offset);

	for (const auto& other_child : node->children)
	{
		StyleSheetNode* local_node = GetOrCreateChildNode(*other_child);
		local_node->MergeHierarchy(other_child.get(), specificity_offset);
	}
}

UniquePtr<StyleSheetNode> StyleSheetNode::DeepCopy(StyleSheetNode* in_parent) const
{
	RMLUI_ZoneScoped;

	auto node = MakeUnique<StyleSheetNode>(in_parent, tag, id, class_names, pseudo_class_names, structural_selectors, combinator);

	node->properties = properties;
	node->children.resize(children.size());

	for (size_t i = 0; i < children.size(); i++)
	{
		node->children[i] = children[i]->DeepCopy(node.get());
	}

	return node;
}

// Builds up a style sheet's index recursively.
void StyleSheetNode::BuildIndex(StyleSheetIndex& styled_node_index) const
{
	// If this has properties defined, then we insert it into the styled node index.
	if (properties.GetNumProperties() > 0)
	{
		auto IndexInsertNode = [](StyleSheetIndex::NodeIndex& node_index, const String& key, const StyleSheetNode* node) {
			StyleSheetIndex::NodeList& nodes = node_index[Hash<String>()(key)];
			auto it = std::find(nodes.begin(), nodes.end(), node);
			if (it == nodes.end())
				nodes.push_back(node);
		};

		// Add this node to the appropriate index for looking up applicable nodes later. Prioritize the most unique requirement first and the most
		// general requirement last. This way we are able to rule out as many nodes as possible as quickly as possible.
		if (!id.empty())
		{
			IndexInsertNode(styled_node_index.ids, id, this);
		}
		else if (!class_names.empty())
		{
			// @performance Right now we just use the first class for simplicity. Later we may want to devise a better strategy to try to add the
			// class with the most unique name. For example by adding the class from this node's list that has the fewest existing matches.
			IndexInsertNode(styled_node_index.classes, class_names.front(), this);
		}
		else if (!tag.empty())
		{
			IndexInsertNode(styled_node_index.tags, tag, this);
		}
		else
		{
			styled_node_index.other.push_back(this);
		}
	}

	for (auto& child : children)
		child->BuildIndex(styled_node_index);
}

bool StyleSheetNode::EqualRequirements(const String& _tag, const String& _id, const StringList& _class_names, const StringList& _pseudo_class_names,
	const StructuralSelectorList& _structural_selectors, SelectorCombinator _combinator) const
{
	if (tag != _tag)
		return false;
	if (id != _id)
		return false;
	if (class_names != _class_names)
		return false;
	if (pseudo_class_names != _pseudo_class_names)
		return false;
	if (structural_selectors != _structural_selectors)
		return false;
	if (combinator != _combinator)
		return false;

	return true;
}

// Returns the specificity of this node.
int StyleSheetNode::GetSpecificity() const
{
	return specificity;
}

// Imports properties from a single rule definition (ie, with a shared specificity) into the node's
// properties.
void StyleSheetNode::ImportProperties(const PropertyDictionary& _properties, int rule_specificity)
{
	properties.Import(_properties, specificity + rule_specificity);
}

// Returns the node's default properties.
const PropertyDictionary& StyleSheetNode::GetProperties() const
{
	return properties;
}

bool StyleSheetNode::Match(const Element* element) const
{
	if (!tag.empty() && tag != element->GetTagName())
		return false;

	if (!id.empty() && id != element->GetId())
		return false;

	if (!MatchClassPseudoClass(element))
		return false;

	if (!MatchStructuralSelector(element))
		return false;

	return true;
}

inline bool StyleSheetNode::MatchClassPseudoClass(const Element* element) const
{
	for (auto& name : class_names)
	{
		if (!element->IsClassSet(name))
			return false;
	}

	for (auto& name : pseudo_class_names)
	{
		if (!element->IsPseudoClassSet(name))
			return false;
	}

	return true;
}

inline bool StyleSheetNode::MatchStructuralSelector(const Element* element) const
{
	for (auto& node_selector : structural_selectors)
	{
		if (!IsSelectorApplicable(element, node_selector))
			return false;
	}

	return true;
}

bool StyleSheetNode::TraverseMatch(const Element* element) const
{
	RMLUI_ASSERT(parent);
	if (!parent->parent)
		return true;

	switch (combinator)
	{
	case SelectorCombinator::Descendant:
	case SelectorCombinator::Child:
	{
		// Try to match the next element parent. If it succeeds we continue on to the next node, otherwise we try an alternate path through the
		// hierarchy using the next element parent. Repeat until we run out of elements.
		for (element = element->GetParentNode(); element; element = element->GetParentNode())
		{
			if (parent->Match(element) && parent->TraverseMatch(element))
				return true;
			// If the node has a child combinator we must match this first ancestor.
			else if (combinator == SelectorCombinator::Child)
				return false;
		}
	}
	break;
	case SelectorCombinator::NextSibling:
	case SelectorCombinator::SubsequentSibling:
	{
		// Try to match the previous sibling. If it succeeds we continue on to the next node, otherwise we try to again with its previous sibling.
		for (element = element->GetPreviousSibling(); element; element = element->GetPreviousSibling())
		{
			// First check if our sibling is a text element and if so skip it. For the descendant/child combinator above we can omit this step since
			// text elements don't have children and thus any ancestor is not a text element.
			if (IsTextElement(element))
				continue;
			else if (parent->Match(element) && parent->TraverseMatch(element))
				return true;
			// If the node has a next-sibling combinator we must match this first sibling.
			else if (combinator == SelectorCombinator::NextSibling)
				return false;
		}
	}
	break;
	}

	// We have run out of element ancestors before we matched every node. Bail out.
	return false;
}

bool StyleSheetNode::IsApplicable(const Element* element) const
{
	// Determine whether the element matches the current node and its entire lineage. The entire hierarchy of the element's document will be
	// considered during the match as necessary.

	// We could in principle just call Match() here and then go on with the ancestor style nodes. Instead, we test the requirements of this node in a
	// particular order for performance reasons.
	for (const String& name : pseudo_class_names)
	{
		if (!element->IsPseudoClassSet(name))
			return false;
	}

	if (!tag.empty() && tag != element->GetTagName())
		return false;

	for (const String& name : class_names)
	{
		if (!element->IsClassSet(name))
			return false;
	}

	if (!id.empty() && id != element->GetId())
		return false;

	// Walk up through all our parent nodes, each one of them must be matched by some ancestor or sibling element.
	if (parent && !TraverseMatch(element))
		return false;

	// Finally, check the structural selector requirements last as they can be quite slow.
	if (!MatchStructuralSelector(element))
		return false;

	return true;
}

void StyleSheetNode::CalculateAndSetSpecificity()
{
	// First calculate the specificity of this node alone.
	specificity = 0;

	if (!tag.empty())
		specificity += SelectorSpecificity::Tag;

	if (!id.empty())
		specificity += SelectorSpecificity::ID;

	specificity += SelectorSpecificity::Class * (int)class_names.size();
	specificity += SelectorSpecificity::PseudoClass * (int)pseudo_class_names.size();

	for (const StructuralSelector& selector : structural_selectors)
		specificity += selector.specificity;

	// Then add our parent's specificity onto ours.
	if (parent)
		specificity += parent->specificity;
}

} // namespace Rml
