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

#include "StyleSheetNode.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "StyleSheetFactory.h"
#include "StyleSheetSelector.h"
#include <algorithm>
#include <tuple>

namespace Rml {

static inline bool IsTextElement(const Element* element)
{
	return element->GetTagName() == "#text";
}

StyleSheetNode::StyleSheetNode()
{
	CalculateAndSetSpecificity();
}

StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, const CompoundSelector& selector) : parent(parent), selector(selector)
{
	CalculateAndSetSpecificity();
}

StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, CompoundSelector&& selector) : parent(parent), selector(std::move(selector))
{
	CalculateAndSetSpecificity();
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(const CompoundSelector& other)
{
	// See if we match an existing child
	for (const auto& child : children)
	{
		if (child->selector == other)
			return child.get();
	}

	// We don't, so create a new child
	auto child = MakeUnique<StyleSheetNode>(this, other);
	StyleSheetNode* result = child.get();

	children.push_back(std::move(child));

	return result;
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(CompoundSelector&& other)
{
	// See if we match an existing child
	for (const auto& child : children)
	{
		if (child->selector == other)
			return child.get();
	}

	// We don't, so create a new child
	auto child = MakeUnique<StyleSheetNode>(this, std::move(other));
	StyleSheetNode* result = child.get();

	children.push_back(std::move(child));

	return result;
}

void StyleSheetNode::MergeHierarchy(StyleSheetNode* node, int specificity_offset)
{
	RMLUI_ZoneScoped;

	// Merge the other node's properties into ours.
	properties.Merge(node->properties, specificity_offset);

	for (const auto& other_child : node->children)
	{
		StyleSheetNode* local_node = GetOrCreateChildNode(other_child->selector);
		local_node->MergeHierarchy(other_child.get(), specificity_offset);
	}
}

UniquePtr<StyleSheetNode> StyleSheetNode::DeepCopy(StyleSheetNode* in_parent) const
{
	RMLUI_ZoneScoped;

	auto node = MakeUnique<StyleSheetNode>(in_parent, selector);

	node->properties = properties;
	node->children.resize(children.size());

	for (size_t i = 0; i < children.size(); i++)
	{
		node->children[i] = children[i]->DeepCopy(node.get());
	}

	return node;
}

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
		if (!selector.id.empty())
		{
			IndexInsertNode(styled_node_index.ids, selector.id, this);
		}
		else if (!selector.class_names.empty())
		{
			// @performance Right now we just use the first class for simplicity. Later we may want to devise a better strategy to try to add the
			// class with the most unique name. For example by adding the class from this node's list that has the fewest existing matches.
			IndexInsertNode(styled_node_index.classes, selector.class_names.front(), this);
		}
		else if (!selector.tag.empty())
		{
			IndexInsertNode(styled_node_index.tags, selector.tag, this);
		}
		else
		{
			styled_node_index.other.push_back(this);
		}
	}

	for (auto& child : children)
		child->BuildIndex(styled_node_index);
}

int StyleSheetNode::GetSpecificity() const
{
	return specificity;
}

void StyleSheetNode::ImportProperties(const PropertyDictionary& _properties, int rule_specificity)
{
	properties.Import(_properties, specificity + rule_specificity);
}

const PropertyDictionary& StyleSheetNode::GetProperties() const
{
	return properties;
}

bool StyleSheetNode::Match(const Element* element) const
{
	if (!selector.tag.empty() && selector.tag != element->GetTagName())
		return false;

	if (!selector.id.empty() && selector.id != element->GetId())
		return false;

	for (auto& name : selector.class_names)
	{
		if (!element->IsClassSet(name))
			return false;
	}

	for (auto& name : selector.pseudo_class_names)
	{
		if (!element->IsPseudoClassSet(name))
			return false;
	}

	if (!selector.attributes.empty() && !MatchAttributes(element))
		return false;

	if (!selector.structural_selectors.empty() && !MatchStructuralSelector(element))
		return false;

	return true;
}

bool StyleSheetNode::MatchStructuralSelector(const Element* element) const
{
	for (auto& node_selector : selector.structural_selectors)
	{
		if (!IsSelectorApplicable(element, node_selector))
			return false;
	}

	return true;
}

bool StyleSheetNode::MatchAttributes(const Element* element) const
{
	for (const AttributeSelector& attribute : selector.attributes)
	{
		const Variant* variant = element->GetAttribute(attribute.name);
		if (!variant)
			return false;
		if (attribute.type == AttributeSelectorType::Always)
			continue;

		String buffer;
		const String* element_value_ptr = &buffer;
		if (variant->GetType() == Variant::STRING)
			element_value_ptr = &variant->GetReference<String>();
		else
			variant->GetInto(buffer);

		const String& element_value = *element_value_ptr;
		const String& css_value = attribute.value;

		auto BeginsWith = [](const String& target, const String& prefix) {
			return prefix.size() <= target.size() && std::equal(prefix.begin(), prefix.end(), target.begin());
		};
		auto EndsWith = [](const String& target, const String& suffix) {
			return suffix.size() <= target.size() && std::equal(suffix.rbegin(), suffix.rend(), target.rbegin());
		};

		switch (attribute.type)
		{
		case AttributeSelectorType::Always: break;
		case AttributeSelectorType::Equal:
			if (element_value != css_value)
				return false;
			break;
		case AttributeSelectorType::InList:
		{
			bool found = false;
			for (size_t index = element_value.find(css_value); index != String::npos; index = element_value.find(css_value, index + 1))
			{
				const size_t index_right = index + css_value.size();
				const bool whitespace_left = (index == 0 || element_value[index - 1] == ' ');
				const bool whitespace_right = (index_right == element_value.size() || element_value[index_right] == ' ');

				if (whitespace_left && whitespace_right)
				{
					found = true;
					break;
				}
			}
			if (!found)
				return false;
		}
		break;
		case AttributeSelectorType::BeginsWithThenHyphen:
			// Begins with 'css_value' followed by a hyphen, or matches exactly.
			if (!BeginsWith(element_value, css_value) || (element_value.size() != css_value.size() && element_value[css_value.size()] != '-'))
				return false;
			break;
		case AttributeSelectorType::BeginsWith:
			if (!BeginsWith(element_value, css_value))
				return false;
			break;
		case AttributeSelectorType::EndsWith:
			if (!EndsWith(element_value, css_value))
				return false;
			break;
		case AttributeSelectorType::Contains:
			if (element_value.find(css_value) == String::npos)
				return false;
			break;
		}
	}
	return true;
}

bool StyleSheetNode::TraverseMatch(const Element* element) const
{
	RMLUI_ASSERT(parent);
	if (!parent->parent)
		return true;

	switch (selector.combinator)
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
			else if (selector.combinator == SelectorCombinator::Child)
				return false;
		}
	}
	break;
	case SelectorCombinator::NextSibling:
	case SelectorCombinator::SubsequentSibling:
	{
		Element* parent_element = element->GetParentNode();
		if (!parent_element)
			return false;

		const int preceding_sibling_index = [element, parent_element] {
			const int num_children = parent_element->GetNumChildren(true);
			for (int i = 0; i < num_children; i++)
			{
				if (parent_element->GetChild(i) == element)
					return i - 1;
			}
			return -1;
		}();

		// Try to match the previous sibling. If it succeeds we continue on to the next node, otherwise we try to again with its previous sibling.
		for (int i = preceding_sibling_index; i >= 0; i--)
		{
			element = parent_element->GetChild(i);

			// First check if our sibling is a text element and if so skip it. For the descendant/child combinator above we can omit this step since
			// text elements don't have children and thus any ancestor is not a text element.
			if (IsTextElement(element))
				continue;
			else if (parent->Match(element) && parent->TraverseMatch(element))
				return true;
			// If the node has a next-sibling combinator we must match this first sibling.
			else if (selector.combinator == SelectorCombinator::NextSibling)
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
	for (const String& name : selector.pseudo_class_names)
	{
		if (!element->IsPseudoClassSet(name))
			return false;
	}

	if (!selector.tag.empty() && selector.tag != element->GetTagName())
		return false;

	for (const String& name : selector.class_names)
	{
		if (!element->IsClassSet(name))
			return false;
	}

	if (!selector.id.empty() && selector.id != element->GetId())
		return false;

	if (!selector.attributes.empty() && !MatchAttributes(element))
		return false;

	// Check the structural selector requirements last as they can be quite slow.
	if (!selector.structural_selectors.empty() && !MatchStructuralSelector(element))
		return false;

	// Walk up through all our parent nodes, each one of them must be matched by some ancestor or sibling element.
	if (parent && !TraverseMatch(element))
		return false;

	return true;
}

void StyleSheetNode::CalculateAndSetSpecificity()
{
	// First calculate the specificity of this node alone.
	specificity = 0;

	if (!selector.tag.empty())
		specificity += SelectorSpecificity::Tag;

	if (!selector.id.empty())
		specificity += SelectorSpecificity::ID;

	specificity += SelectorSpecificity::Class * (int)selector.class_names.size();
	specificity += SelectorSpecificity::Attribute * (int)selector.attributes.size();
	specificity += SelectorSpecificity::PseudoClass * (int)selector.pseudo_class_names.size();

	for (const StructuralSelector& selector : selector.structural_selectors)
		specificity += selector.specificity;

	// Then add our parent's specificity onto ours.
	if (parent)
		specificity += parent->specificity;
}

} // namespace Rml
