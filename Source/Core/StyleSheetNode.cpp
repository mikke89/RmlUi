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
#include "StyleSheetFactory.h"
#include "StyleSheetNodeSelector.h"
#include <algorithm>

namespace Rml {
namespace Core {

StyleSheetNode::StyleSheetNode()
{
	CalculateAndSetSpecificity();
}

StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, const String& tag, const String& id, const StringList& classes, const StringList& pseudo_classes, const StructuralSelectorList& structural_selectors, bool child_combinator)
	: parent(parent), tag(tag), id(id), class_names(classes), pseudo_class_names(pseudo_classes), structural_selectors(structural_selectors), child_combinator(child_combinator)
{
	CalculateAndSetSpecificity();
}

StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, String&& tag, String&& id, StringList&& classes, StringList&& pseudo_classes, StructuralSelectorList&& structural_selectors, bool child_combinator)
	: parent(parent), tag(std::move(tag)), id(std::move(id)), class_names(std::move(classes)), pseudo_class_names(std::move(pseudo_classes)), structural_selectors(std::move(structural_selectors)), child_combinator(child_combinator)
{
	CalculateAndSetSpecificity();
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(const StyleSheetNode& other)
{
	// See if we match the target child
	for (const auto& child : children)
	{
		if (child->EqualRequirements(other.tag, other.id, other.class_names, other.pseudo_class_names, other.structural_selectors, other.child_combinator))
			return child.get();
	}

	// We don't, so create a new child
	auto child = std::make_unique<StyleSheetNode>(this, other.tag, other.id, other.class_names, other.pseudo_class_names, other.structural_selectors, other.child_combinator);
	StyleSheetNode* result = child.get();

	children.push_back(std::move(child));

	return result;
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(String&& tag, String&& id, StringList&& classes, StringList&& pseudo_classes, StructuralSelectorList&& structural_pseudo_classes, bool child_combinator)
{
	// See if we match an existing child
	for (const auto& child : children)
	{
		if (child->EqualRequirements(tag, id, classes, pseudo_classes, structural_pseudo_classes, child_combinator))
			return child.get();
	}

	// We don't, so create a new child
	auto child = std::make_unique<StyleSheetNode>(this, std::move(tag), std::move(id), std::move(classes), std::move(pseudo_classes), std::move(structural_pseudo_classes), child_combinator);
	StyleSheetNode* result = child.get();

	children.push_back(std::move(child));

	return result;
}

// Merges an entire tree hierarchy into our hierarchy.
bool StyleSheetNode::MergeHierarchy(StyleSheetNode* node, int specificity_offset)
{
	// Merge the other node's properties into ours.
	properties.Merge(node->properties, specificity_offset);

	for (const auto& other_child : node->children)
	{
		StyleSheetNode* local_node = GetOrCreateChildNode(*other_child);
		local_node->MergeHierarchy(other_child.get(), specificity_offset);
	}

	return true;
}

// Builds up a style sheet's index recursively.
void StyleSheetNode::BuildIndexAndOptimizeProperties(StyleSheet::NodeIndex& styled_node_index, const StyleSheet& style_sheet)
{
	RMLUI_ZoneScoped;

	// If this has properties defined, then we insert it into the styled node index.
	if(properties.GetNumProperties() > 0)
	{
		// The keys of the node index is a hashed combination of tag and id. These are used for fast lookup of applicable nodes.
		size_t node_hash = StyleSheet::NodeHash(tag, id);
		StyleSheet::NodeList& nodes = styled_node_index[node_hash];
		auto it = std::find(nodes.begin(), nodes.end(), this);
		if(it == nodes.end())
			nodes.push_back(this);
	}

	// Turn any decorator and font-effect properties from String to DecoratorList / FontEffectList.
	// This is essentially an optimization, it will work fine to skip this step and let ElementStyle::ComputeValues() do all the work.
	// However, when we do it here, we only need to do it once.
	// Note, since the user may set a new decorator through its style, we still do the conversion as necessary again in ComputeValues.
	if (properties.GetNumProperties() > 0)
	{
		// Decorators
		if (const Property* property = properties.GetProperty(PropertyId::Decorator))
		{
			if (property->unit == Property::STRING)
			{
				const String string_value = property->Get<String>();
				
				if(DecoratorsPtr decorators = style_sheet.InstanceDecoratorsFromString(string_value, property->source))
				{
					Property new_property = *property;
					new_property.value = std::move(decorators);
					new_property.unit = Property::DECORATOR;
					properties.SetProperty(PropertyId::Decorator, new_property);
				}
			}
		}

		// Font-effects
		if (const Property * property = properties.GetProperty(PropertyId::FontEffect))
		{
			if (property->unit == Property::STRING)
			{
				const String string_value = property->Get<String>();
				FontEffectsPtr font_effects = style_sheet.InstanceFontEffectsFromString(string_value, property->source);

				Property new_property = *property;
				new_property.value = std::move(font_effects);
				new_property.unit = Property::FONTEFFECT;
				properties.SetProperty(PropertyId::FontEffect, new_property);
			}
		}
	}

	for (auto& child : children)
	{
		child->BuildIndexAndOptimizeProperties(styled_node_index, style_sheet);
	}
}


bool StyleSheetNode::SetStructurallyVolatileRecursive(bool ancestor_is_structural_pseudo_class)
{
	// If any ancestor or descendant is a structural pseudo class, then we are structurally volatile.
	bool self_is_structural_pseudo_class = (!structural_selectors.empty());

	// Check our children for structural pseudo-classes.
	bool descendant_is_structural_pseudo_class = false;
	for (auto& child : children)
	{
		if (child->SetStructurallyVolatileRecursive(self_is_structural_pseudo_class || ancestor_is_structural_pseudo_class))
			descendant_is_structural_pseudo_class = true;
	}

	is_structurally_volatile = (self_is_structural_pseudo_class || ancestor_is_structural_pseudo_class || descendant_is_structural_pseudo_class);

	return (self_is_structural_pseudo_class || descendant_is_structural_pseudo_class);
}

bool StyleSheetNode::EqualRequirements(const String& _tag, const String& _id, const StringList& _class_names, const StringList& _pseudo_class_names, const StructuralSelectorList& _structural_selectors, bool _child_combinator) const
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
	if (child_combinator != _child_combinator)
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

inline bool StyleSheetNode::Match(const Element* element) const
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
		if (!node_selector.selector->IsApplicable(element, node_selector.a, node_selector.b))
			return false;
	}
	
	return true;
}

// Returns true if this node is applicable to the given element, given its IDs, classes and heritage.
bool StyleSheetNode::IsApplicable(const Element* const in_element) const
{
	// This function is called with an element that matches a style node only with the tag name and id. We have to determine
	// here whether or not it also matches the required hierarchy.
	
	// First, check locally for matching class and pseudo class. Id and tag have already been checked in StyleSheet.
	if (!MatchClassPseudoClass(in_element))
		return false;

	const Element* element = in_element;

	// Walk up through all our parent nodes, each one of them must be matched by some ancestor element.
	for(const StyleSheetNode* node = parent; node && node->parent; node = node->parent)
	{
		// Try a match on every element ancestor. If it succeeds, we continue on to the next node.
		for(element = element->GetParentNode(); element; element = element->GetParentNode())
		{
			if (node->Match(element))
				break;
			// If we have a child combinator on the node, we must match this first ancestor.
			else if (node->child_combinator)
				return false;
		}

		// We have run out of element ancestors before we matched every node. Bail out.
		if (!element)
			return false;
	}

	// Finally, check the structural selector requirements last as they can be quite slow.
	if (!MatchStructuralSelector(in_element))
		return false;

	return true;
}

bool StyleSheetNode::IsStructurallyVolatile() const
{
	return is_structurally_volatile;
}


void StyleSheetNode::CalculateAndSetSpecificity()
{
	// Calculate the specificity of just this node; tags are worth 10,000, IDs 1,000,000 and other specifiers (classes
	// and pseudo-classes) 100,000.
	specificity = 0;

	if (!tag.empty())
		specificity += 10'000;

	if (!id.empty())
		specificity += 1'000'000;

	specificity += 100'000*(int)class_names.size();
	specificity += 100'000*(int)pseudo_class_names.size();
	specificity += 100'000*(int)structural_selectors.size();

	// Add our parent's specificity onto ours.
	if (parent)
		specificity += parent->specificity;
}

}
}
