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

#include "precompiled.h"
#include "StyleSheetNode.h"
#include <algorithm>
#include "../../Include/RmlUi/Core/Element.h"
#include "StyleSheetFactory.h"
#include "StyleSheetNodeSelector.h"

namespace Rml {
namespace Core {

StyleSheetNode::StyleSheetNode() : parent(nullptr)
{
	specificity = CalculateSpecificity();
	is_structurally_volatile = true;
}


StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, const String& tag, const String& id, const StringList& classes, const StringList& pseudo_classes, const NodeSelectorList& structural_pseudo_classes)
	: parent(parent), tag(tag), id(id), class_names(classes), pseudo_class_names(pseudo_classes)
{
	specificity = CalculateSpecificity();
	is_structurally_volatile = true;
}

StyleSheetNode::StyleSheetNode(StyleSheetNode* parent, String&& tag, String&& id, StringList&& classes, StringList&& pseudo_classes, NodeSelectorList&& structural_pseudo_classes) 
	: parent(parent), tag(std::move(tag)), id(std::move(id)), class_names(std::move(classes)), pseudo_class_names(std::move(pseudo_classes))
{
	specificity = CalculateSpecificity();
	is_structurally_volatile = true;
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(const StyleSheetNode& other)
{
	// See if we match the target child
	for (const auto& child : children)
	{
		if (child->IsEquivalent(other.tag, other.id, other.class_names, other.pseudo_class_names, other.structural_selectors))
			return child.get();
	}

	// We don't, so create a new child
	auto child = std::make_unique<StyleSheetNode>(this, other.tag, other.id, other.class_names, other.pseudo_class_names, other.structural_selectors);
	StyleSheetNode* result = child.get();

	children.push_back(std::move(child));

	return result;
}

StyleSheetNode* StyleSheetNode::GetOrCreateChildNode(String&& tag, String&& id, StringList&& classes, StringList&& pseudo_classes, NodeSelectorList&& structural_pseudo_classes)
{
	// @performance: Maybe sort children by tag,id or something else?

	// See if we match an existing child
	for (const auto& child : children)
	{
		if (child->IsEquivalent(tag, id, classes, pseudo_classes, structural_pseudo_classes))
			return child.get();
	}

	// We don't, so create a new child
	auto child = std::make_unique<StyleSheetNode>(this, std::move(tag), std::move(id), std::move(classes), std::move(pseudo_classes), std::move(structural_pseudo_classes));
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
	// If this has properties defined, then we insert it into the styled node index.
	if(properties.GetNumProperties() > 0)
	{
		StyleSheet::NodeList& nodes = styled_node_index[tag];
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
				
				if(DecoratorListPtr decorator_list = style_sheet.InstanceDecoratorsFromString(string_value, property->source))
				{
					Property new_property = *property;
					new_property.value = std::move(decorator_list);
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
				FontEffectListPtr font_effects = style_sheet.InstanceFontEffectsFromString(string_value, property->source);

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

bool StyleSheetNode::IsEquivalent(const String& _tag, const String& _id, const StringList& _class_names, const StringList& _pseudo_class_names, const NodeSelectorList& _structural_selectors) const
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

	return true;
}


// Returns the name of this node.
const String& StyleSheetNode::GetTag() const
{
	return tag;
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

bool StyleSheetNode::Match(const Element* element, const StyleSheetNode* node)
{
	if (!node->tag.empty() && node->tag != element->GetTagName())
		return false;

	if (!node->id.empty() && node->id != element->GetId())
		return false;

	for (auto& name : node->class_names)
	{
		if (!element->IsClassSet(name))
			return false;
	}

	for (auto& name : node->pseudo_class_names)
	{
		if (!element->IsPseudoClassSet(name))
			return false;
	}

	for (auto& node_selector : node->structural_selectors)
	{
		if (!node_selector.selector->IsApplicable(element, node_selector.a, node_selector.b))
			return false;
	}
	
	return true;
}

// Returns true if this node is applicable to the given element, given its IDs, classes and heritage.
bool StyleSheetNode::IsApplicable(const Element* element) const
{
	// This function is called with an element that matches a style node only with the tag name. We have to determine
	// here whether or not it also matches the required hierarchy.

	// We must have a parent; if not, something's amok with the style tree.
	if (parent == nullptr)
	{
		RMLUI_ERRORMSG("Invalid RCSS hierarchy.");
		return false;
	}

	const StyleSheetNode* node = this;
	
	// Check for matching local requirements
	if (!Match(element, node))
		return false;

	// Then match each parent node
	for(node = node->parent; node && node->parent; node = node->parent)
	{
		for(element = element->GetParentNode(); element; element = element->GetParentNode())
		{
			if (Match(element, node))
				break;
		}

		if (!element)
			return false;
	}

	return true;
}

bool StyleSheetNode::IsStructurallyVolatile() const
{
	return is_structurally_volatile;
}


int StyleSheetNode::CalculateSpecificity()
{
	// Calculate the specificity of just this node; tags are worth 10,000, IDs 1,000,000 and other specifiers (classes
	// and pseudo-classes) 100,000.

	int specificity = 0;

	if (!tag.empty())
		specificity += 10000;

	if (!id.empty())
		specificity += 1000000;

	specificity += 100000*(int)class_names.size();
	specificity += 100000*(int)pseudo_class_names.size();
	specificity += 100000*(int)structural_selectors.size();

	// Add our parent's specificity onto ours.
	// @performance: Replace with parent->specificity
	if (parent)
		specificity += parent->CalculateSpecificity();

	return specificity;
}

}
}
