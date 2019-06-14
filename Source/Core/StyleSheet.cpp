/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include "../../Include/Rocket/Core/StyleSheet.h"
#include <algorithm>
#include "ElementDefinition.h"
#include "StyleSheetFactory.h"
#include "StyleSheetNode.h"
#include "StyleSheetParser.h"
#include "../../Include/Rocket/Core/Element.h"
#include "../../Include/Rocket/Core/PropertyDefinition.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"

namespace Rocket {
namespace Core {

template <class T>
static inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Sorts style nodes based on specificity.
static bool StyleSheetNodeSort(const StyleSheetNode* lhs, const StyleSheetNode* rhs)
{
	return lhs->GetSpecificity() < rhs->GetSpecificity();
}

StyleSheet::StyleSheet()
{
	root = new StyleSheetNode("", StyleSheetNode::ROOT);
	specificity_offset = 0;
}

StyleSheet::~StyleSheet()
{
	delete root;

	// Release decorators
	for (auto& pair : decorator_map)
		pair.second.decorator->RemoveReference();

	// Release our reference count on the cached element definitions.
	for (ElementDefinitionCache::iterator cache_iterator = address_cache.begin(); cache_iterator != address_cache.end(); ++cache_iterator)
		(*cache_iterator).second->RemoveReference();

	for (ElementDefinitionCache::iterator cache_iterator = node_cache.begin(); cache_iterator != node_cache.end(); ++cache_iterator)
		(*cache_iterator).second->RemoveReference();
}

bool StyleSheet::LoadStyleSheet(Stream* stream)
{
	StyleSheetParser parser;
	specificity_offset = parser.Parse(root, keyframes, decorator_map, stream);
	return specificity_offset >= 0;
}

/// Combines this style sheet with another one, producing a new sheet
StyleSheet* StyleSheet::CombineStyleSheet(const StyleSheet* other_sheet) const
{
	StyleSheet* new_sheet = new StyleSheet();
	if (!new_sheet->root->MergeHierarchy(root) ||
		!new_sheet->root->MergeHierarchy(other_sheet->root, specificity_offset))
	{
		delete new_sheet;
		return NULL;
	}

	// Any matching @keyframe names are overridden as per CSS rules
	new_sheet->keyframes = keyframes;
	for (auto& other_keyframes : other_sheet->keyframes)
	{
		new_sheet->keyframes[other_keyframes.first] = other_keyframes.second;
	}

	// Copy over the decorators, and replace any matching decorator names from other_sheet
	// @todo / @leak: Add and remove references as appropriate, not sufficient as is!
	new_sheet->decorator_map = decorator_map;
	for (auto& other_decorator: other_sheet->decorator_map)
	{
		new_sheet->decorator_map[other_decorator.first] = other_decorator.second;
	}
	for (auto& pair : new_sheet->decorator_map)
		pair.second.decorator->AddReference();

	new_sheet->specificity_offset = specificity_offset + other_sheet->specificity_offset;
	return new_sheet;
}

// Builds the node index for a combined style sheet.
void StyleSheet::BuildNodeIndex()
{
	if (complete_node_index.empty())
	{
		styled_node_index.clear();
		complete_node_index.clear();

		root->BuildIndex(styled_node_index, complete_node_index);
	}
}

// Returns the Keyframes of the given name, or null if it does not exist.
Keyframes * StyleSheet::GetKeyframes(const String & name)
{
	auto it = keyframes.find(name);
	if (it != keyframes.end())
		return &(it->second);
	return nullptr;
}

Decorator* StyleSheet::GetDecorator(const String& name) const
{
	auto it = decorator_map.find(name);
	if (it == decorator_map.end())
		return nullptr;
	return it->second.decorator;
}

Decorator* StyleSheet::GetOrInstanceDecorator(const String& decorator_value, const String& source_file, int source_line_number)
{
	// Try to find a decorator declared with @decorator or otherwise previously instanced shorthand decorator.
	auto it_find = decorator_map.find(decorator_value);
	if (it_find != decorator_map.end())
	{
		return it_find->second.decorator;
	}

	// The decorator does not exist, try to instance a new one from a shorthand decorator value declared as:
	//   decorator: <type>( <shorthand> );
	// where <type> is the decorator type and the value <shorthand> is applied to its "decorator"-shorthand.

	// Check syntax
	size_t shorthand_open = decorator_value.find('(');
	size_t shorthand_close = decorator_value.rfind(')');
	if (shorthand_open == String::npos || shorthand_close == String::npos || shorthand_open >= shorthand_close)
		return nullptr;

	String type = StringUtilities::StripWhitespace(decorator_value.substr(0, shorthand_open));

	// Check for valid decorator type
	const PropertySpecification* specification = Factory::GetDecoratorPropertySpecification(type);
	if (!specification)
		return nullptr;

	String shorthand = decorator_value.substr(shorthand_open + 1, shorthand_close - shorthand_open - 1);

	// Parse the shorthand properties
	PropertyDictionary properties;
	if (!specification->ParsePropertyDeclaration(properties, "decorator", shorthand, source_file, source_line_number))
	{
		Log::Message(Log::LT_WARNING, "Could not parse decorator value '%s' at %s:%d", decorator_value.c_str(), source_file.c_str(), source_line_number);
		return nullptr;
	}

	specification->SetPropertyDefaults(properties);

	Decorator* decorator = Factory::InstanceDecorator(type, properties);
	if (!decorator)
		return nullptr;

	// Insert decorator into map
	auto result = decorator_map.emplace(decorator_value, DecoratorSpecification{ type, properties, decorator });
	
	if (!result.second)
	{
		decorator->RemoveReference();
		return nullptr;
	}

	return decorator;
}

// Returns the compiled element definition for a given element hierarchy.
ElementDefinition* StyleSheet::GetElementDefinition(const Element* element) const
{
	ROCKET_ASSERT_NONRECURSIVE;

	// Address cache is disabled for the time being; this doesn't work since the introduction of structural
	// pseudo-classes.
	ElementDefinitionCache::iterator cache_iterator;
/*	String element_address = element->GetAddress();

	// Look the address up in the definition, see if we've processed a similar element before.
	cache_iterator = address_cache.find(element_address);
	if (cache_iterator != address_cache.end())
	{
		ElementDefinition* definition = (*cache_iterator).second;
		definition->AddReference();
		return definition;
	}*/

	// See if there are any styles defined for this element.
	// Using static to avoid allocations. Make sure we don't call this function recursively.
	static std::vector< const StyleSheetNode* > applicable_nodes;
	applicable_nodes.clear();

	String tags[] = {element->GetTagName(), ""};
	for (int i = 0; i < 2; i++)
	{
		NodeIndex::const_iterator iterator = styled_node_index.find(tags[i]);
		if (iterator != styled_node_index.end())
		{
			const NodeList& nodes = (*iterator).second;

			// There are! Now see if we satisfy all of their parenting requirements. What this involves is traversing the style
			// nodes backwards, trying to match nodes in the element's hierarchy to nodes in the style hierarchy.
			for (NodeList::const_iterator iterator = nodes.begin(); iterator != nodes.end(); iterator++)
			{
				if ((*iterator)->IsApplicable(element))
				{
					// Get the node to add any of its non-tag children that we match into our list.
					(*iterator)->GetApplicableDescendants(applicable_nodes, element);
				}
			}
		}
	}

	std::sort(applicable_nodes.begin(), applicable_nodes.end(), StyleSheetNodeSort);

	// Compile the list of volatile pseudo-classes for this element definition.
	PseudoClassList volatile_pseudo_classes;
	bool structurally_volatile = false;

	for (int i = 0; i < 2; ++i)
	{
		NodeIndex::const_iterator iterator = complete_node_index.find(tags[i]);
		if (iterator != complete_node_index.end())
		{
			const NodeList& nodes = (*iterator).second;

			// See if we satisfy all of the parenting requirements for each of these nodes (as in the previous loop).
			for (NodeList::const_iterator iterator = nodes.begin(); iterator != nodes.end(); iterator++)
			{
				structurally_volatile |= (*iterator)->IsStructurallyVolatile();

				if ((*iterator)->IsApplicable(element))
				{
					std::vector< const StyleSheetNode* > volatile_nodes;
					(*iterator)->GetApplicableDescendants(volatile_nodes, element);

					for (size_t i = 0; i < volatile_nodes.size(); ++i)
						volatile_nodes[i]->GetVolatilePseudoClasses(volatile_pseudo_classes);
				}
			}
		}
	}

	// If this element definition won't actually store any information, don't bother with it.
	if (applicable_nodes.empty() &&
		volatile_pseudo_classes.empty() &&
		!structurally_volatile)
		return NULL;

	// Check if this puppy has already been cached in the node index; it may be that it has already been created by an
	// element with a different address but an identical output definition.
	size_t seed = 0;
	for (const auto* node : applicable_nodes)
		hash_combine(seed, node);
	for (const String& str : volatile_pseudo_classes)
		hash_combine(seed, str);

	cache_iterator = node_cache.find(seed);
	if (cache_iterator != node_cache.end())
	{
		ElementDefinition* definition = (*cache_iterator).second;
		definition->AddReference();
		applicable_nodes.clear();
		return definition;
	}

	// Create the new definition and add it to our cache. One reference count is added, bringing the total to two; one
	// for the element that requested it, and one for the cache.
	ElementDefinition* new_definition = new ElementDefinition();
	new_definition->Initialise(applicable_nodes, volatile_pseudo_classes, structurally_volatile);

	// Add to the address cache.
//	address_cache[element_address] = new_definition;
//	new_definition->AddReference();

	// Add to the node cache.
	node_cache[seed] = new_definition;
	new_definition->AddReference();

	applicable_nodes.clear();
	return new_definition;
}

// Destroys the style sheet.
void StyleSheet::OnReferenceDeactivate()
{
	delete this;
}

}
}
