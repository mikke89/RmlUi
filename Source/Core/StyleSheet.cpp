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

#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "ElementDefinition.h"
#include "StyleSheetNode.h"
#include "Utilities.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include <algorithm>

namespace Rml {

// Sorts style nodes based on specificity.
inline static bool StyleSheetNodeSort(const StyleSheetNode* lhs, const StyleSheetNode* rhs)
{
	return lhs->GetSpecificity() < rhs->GetSpecificity();
}

StyleSheet::StyleSheet()
{
	root = MakeUnique<StyleSheetNode>();
	specificity_offset = 0;
}

StyleSheet::~StyleSheet()
{
}

/// Combines this style sheet with another one, producing a new sheet
UniquePtr<StyleSheet> StyleSheet::CombineStyleSheet(const StyleSheet& other_sheet) const
{
	RMLUI_ZoneScoped;

	UniquePtr<StyleSheet> new_sheet = UniquePtr<StyleSheet>(new StyleSheet());
	
	new_sheet->root = root->DeepCopy();
	new_sheet->specificity_offset = specificity_offset;
	new_sheet->keyframes = keyframes;
	new_sheet->decorator_map = decorator_map;
	new_sheet->spritesheet_list = spritesheet_list;

	new_sheet->MergeStyleSheet(other_sheet);

	return new_sheet;
}

void StyleSheet::MergeStyleSheet(const StyleSheet& other_sheet)
{
	RMLUI_ZoneScoped;

	root->MergeHierarchy(other_sheet.root.get(), specificity_offset);
	specificity_offset += other_sheet.specificity_offset;

	// Any matching @keyframe names are overridden as per CSS rules
	keyframes.reserve(keyframes.size() + other_sheet.keyframes.size());
	for (auto& other_keyframes : other_sheet.keyframes)
	{
		keyframes[other_keyframes.first] = other_keyframes.second;
	}

	// Copy over the decorators, and replace any matching decorator names from other_sheet
	decorator_map.reserve(decorator_map.size() + other_sheet.decorator_map.size());
	for (auto& other_decorator : other_sheet.decorator_map)
	{
		decorator_map[other_decorator.first] = other_decorator.second;
	}

	spritesheet_list.Reserve(
		spritesheet_list.NumSpriteSheets() + other_sheet.spritesheet_list.NumSpriteSheets(),
		spritesheet_list.NumSprites() + other_sheet.spritesheet_list.NumSprites()
	);
	spritesheet_list.Merge(other_sheet.spritesheet_list);
}

// Builds the node index for a combined style sheet.
void StyleSheet::BuildNodeIndex()
{
	RMLUI_ZoneScoped;
	styled_node_index.clear();
	root->BuildIndex(styled_node_index);
	root->SetStructurallyVolatileRecursive(false);
}

// Returns the Keyframes of the given name, or null if it does not exist.
const Keyframes * StyleSheet::GetKeyframes(const String & name) const
{
	auto it = keyframes.find(name);
	if (it != keyframes.end())
		return &(it->second);
	return nullptr;
}

const Vector<SharedPtr<const Decorator>>& StyleSheet::InstanceDecorators(const DecoratorDeclarationList& declaration_list, const PropertySource* source) const
{
	// Generate the cache key. Relative paths of textures may be affected by the source path, and ultimately
	// which texture should be displayed. Thus, we need to include this path in the cache key.
	String key;
	key.reserve(declaration_list.value.size() + 1 + (source ? source->path.size() : 0));
	key = declaration_list.value;
	key += ';';
	if (source)
		key += source->path;

	auto it_cache = decorator_cache.find(key);
	if (it_cache != decorator_cache.end())
		return it_cache->second;

	Vector<SharedPtr<const Decorator>>& decorators = decorator_cache[key];

	for (const DecoratorDeclaration& declaration : declaration_list.list)
	{
		if (declaration.instancer)
		{
			RMLUI_ZoneScopedN("InstanceDecorator");
			
			if (SharedPtr<Decorator> decorator = declaration.instancer->InstanceDecorator(declaration.type, declaration.properties, DecoratorInstancerInterface(*this, source)))
				decorators.push_back(std::move(decorator));
			else
				Log::Message(Log::LT_WARNING, "Decorator '%s' in '%s' could not be instanced, declared at %s:%d", declaration.type.c_str(), declaration_list.value.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
		}
		else
		{
			// If we have no instancer, this means the type is the name of an @decorator rule.
			SharedPtr<Decorator> decorator;
			auto it_map = decorator_map.find(declaration.type);
			if (it_map != decorator_map.end())
				decorator = it_map->second.decorator;

			if (decorator)
				decorators.push_back(std::move(decorator));
			else
				Log::Message(Log::LT_WARNING, "Decorator name '%s' could not be found in any @decorator rule, declared at %s:%d", declaration.type.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
		}
	}

	return decorators;
}

const Sprite* StyleSheet::GetSprite(const String& name) const
{
	return spritesheet_list.GetSprite(name);
}

size_t StyleSheet::NodeHash(const String& tag, const String& id)
{
	size_t seed = 0;
	if (!tag.empty())
		seed = Hash<String>()(tag);
	if(!id.empty())
		Utilities::HashCombine(seed, id);
	return seed;
}

// Returns the compiled element definition for a given element hierarchy.
SharedPtr<ElementDefinition> StyleSheet::GetElementDefinition(const Element* element) const
{
	RMLUI_ASSERT_NONRECURSIVE;

	// See if there are any styles defined for this element.
	// Using static to avoid allocations. Make sure we don't call this function recursively.
	static Vector< const StyleSheetNode* > applicable_nodes;
	applicable_nodes.clear();

	const String& tag = element->GetTagName();
	const String& id = element->GetId();

	// The styled_node_index is hashed with the tag and id of the RCSS rule. However, we must also check
	// the rules which don't have them defined, because they apply regardless of tag and id.
	Array<size_t, 4> node_hash;
	int num_hashes = 2;

	node_hash[0] = 0;
	node_hash[1] = NodeHash(tag, String());

	// If we don't have an id, we can safely skip nodes that define an id. Otherwise, we also check the id nodes.
	if (!id.empty())
	{
		num_hashes = 4;
		node_hash[2] = NodeHash(String(), id);
		node_hash[3] = NodeHash(tag, id);
	}

	// The hashes are keys into a set of applicable nodes (given tag and id).
	for (int i = 0; i < num_hashes; i++)
	{
		auto it_nodes = styled_node_index.find(node_hash[i]);
		if (it_nodes != styled_node_index.end())
		{
			const NodeList& nodes = it_nodes->second;

			// Now see if we satisfy all of the requirements not yet tested: classes, pseudo classes, structural selectors, 
			// and the full requirements of parent nodes. What this involves is traversing the style nodes backwards, 
			// trying to match nodes in the element's hierarchy to nodes in the style hierarchy.
			for (const StyleSheetNode* node : nodes)
			{
				if (node->IsApplicable(element, true))
				{
					applicable_nodes.push_back(node);
				}
			}
		}
	}

	std::sort(applicable_nodes.begin(), applicable_nodes.end(), StyleSheetNodeSort);

	// If this element definition won't actually store any information, don't bother with it.
	if (applicable_nodes.empty())
		return nullptr;

	// Check if this puppy has already been cached in the node index.
	size_t seed = 0;
	for (const StyleSheetNode* node : applicable_nodes)
		Utilities::HashCombine(seed, node);

	auto cache_iterator = node_cache.find(seed);
	if (cache_iterator != node_cache.end())
	{
		SharedPtr<ElementDefinition>& definition = (*cache_iterator).second;
		return definition;
	}

	// Create the new definition and add it to our cache.
	auto new_definition = MakeShared<ElementDefinition>(applicable_nodes);
	node_cache[seed] = new_definition;

	return new_definition;
}

} // namespace Rml
