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

#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "ElementDefinition.h"
#include "ElementStyle.h"
#include "StyleSheetNode.h"
#include <algorithm>

namespace Rml {

StyleSheet::StyleSheet()
{
	root = MakeUnique<StyleSheetNode>();
	specificity_offset = 0;
}

StyleSheet::~StyleSheet() {}

UniquePtr<StyleSheet> StyleSheet::CombineStyleSheet(const StyleSheet& other_sheet) const
{
	RMLUI_ZoneScoped;

	UniquePtr<StyleSheet> new_sheet = UniquePtr<StyleSheet>(new StyleSheet());

	new_sheet->root = root->DeepCopy();
	new_sheet->specificity_offset = specificity_offset;
	new_sheet->keyframes = keyframes;
	new_sheet->named_decorator_map = named_decorator_map;
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
	named_decorator_map.reserve(named_decorator_map.size() + other_sheet.named_decorator_map.size());
	for (auto& other_decorator : other_sheet.named_decorator_map)
	{
		named_decorator_map[other_decorator.first] = other_decorator.second;
	}

	spritesheet_list.Reserve(spritesheet_list.NumSpriteSheets() + other_sheet.spritesheet_list.NumSpriteSheets(),
		spritesheet_list.NumSprites() + other_sheet.spritesheet_list.NumSprites());
	spritesheet_list.Merge(other_sheet.spritesheet_list);
}

void StyleSheet::BuildNodeIndex()
{
	RMLUI_ZoneScoped;
	styled_node_index = {};
	root->BuildIndex(styled_node_index);
}

const NamedDecorator* StyleSheet::GetNamedDecorator(const String& name) const
{
	auto it = named_decorator_map.find(name);
	if (it != named_decorator_map.end())
		return &(it->second);
	return nullptr;
}

const Keyframes* StyleSheet::GetKeyframes(const String& name) const
{
	auto it = keyframes.find(name);
	if (it != keyframes.end())
		return &(it->second);
	return nullptr;
}

const DecoratorPtrList& StyleSheet::InstanceDecorators(RenderManager& render_manager, const DecoratorDeclarationList& declaration_list,
	const PropertySource* source) const
{
	RMLUI_ASSERT_NONRECURSIVE; // Since we may return a reference to the below static variable.
	static DecoratorPtrList non_cached_decorator_list;

	// Empty declaration values are used for interpolated values which we don't want to cache.
	const bool enable_cache = !declaration_list.value.empty();

	// Generate the cache key. Relative paths of textures may be affected by the source path, and ultimately
	// which texture should be displayed. Thus, we need to include this path in the cache key.
	String key;

	if (enable_cache)
	{
		key.reserve(declaration_list.value.size() + 1 + (source ? source->path.size() : 0));
		key = declaration_list.value;
		key += ';';
		if (source)
			key += source->path;

		auto it_cache = decorator_cache.find(key);
		if (it_cache != decorator_cache.end())
			return it_cache->second;
	}
	else
	{
		non_cached_decorator_list.clear();
	}

	DecoratorPtrList& decorators = enable_cache ? decorator_cache[key] : non_cached_decorator_list;
	decorators.reserve(declaration_list.list.size());

	for (const DecoratorDeclaration& declaration : declaration_list.list)
	{
		SharedPtr<Decorator> decorator;

		if (declaration.instancer)
		{
			RMLUI_ZoneScopedN("InstanceDecorator");
			decorator = declaration.instancer->InstanceDecorator(declaration.type, declaration.properties,
				DecoratorInstancerInterface(render_manager, *this, source));

			if (!decorator)
				Log::Message(Log::LT_WARNING, "Decorator '%s' in '%s' could not be instanced, declared at %s:%d", declaration.type.c_str(),
					declaration_list.value.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
		}
		else
		{
			// If we have no instancer, this means the type is the name of an @decorator rule.
			auto it_map = named_decorator_map.find(declaration.type);
			if (it_map != named_decorator_map.end())
				decorator = it_map->second.instancer->InstanceDecorator(it_map->second.type, it_map->second.properties,
					DecoratorInstancerInterface(render_manager, *this, source));

			if (!decorator)
				Log::Message(Log::LT_WARNING, "Decorator name '%s' could not be found in any @decorator rule, declared at %s:%d",
					declaration.type.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
		}

		if (!decorator)
		{
			decorators.clear();
			break;
		}

		decorators.push_back(std::move(decorator));
	}

	return decorators;
}

const Sprite* StyleSheet::GetSprite(const String& name) const
{
	return spritesheet_list.GetSprite(name);
}

SharedPtr<const ElementDefinition> StyleSheet::GetElementDefinition(const Element* element) const
{
	RMLUI_ASSERT_NONRECURSIVE;

	// Using static to avoid allocations. Make sure we don't call this function recursively.
	static Vector<const StyleSheetNode*> applicable_nodes;
	applicable_nodes.clear();

	auto AddApplicableNodes = [element](const StyleSheetIndex::NodeIndex& node_index, const String& key) {
		auto it_nodes = node_index.find(Hash<String>()(key));
		if (it_nodes != node_index.end())
		{
			const StyleSheetIndex::NodeList& nodes = it_nodes->second;

			for (const StyleSheetNode* node : nodes)
			{
				// We found a node that has at least one requirement matching the element. Now see if we satisfy the remaining requirements of the
				// node, including all ancestor nodes. What this involves is traversing the style nodes backwards, trying to match nodes in the
				// element's hierarchy to nodes in the style hierarchy.
				if (node->IsApplicable(element))
					applicable_nodes.push_back(node);
			}
		}
	};

	// See if there are any styles defined for this element.
	const String& tag = element->GetTagName();
	const String& id = element->GetId();
	const StringList& class_names = element->GetStyle()->GetClassNameList();

	// Text elements are never matched.
	if (tag == "#text")
		return nullptr;

	// First, look up the indexed requirements.
	if (!id.empty())
		AddApplicableNodes(styled_node_index.ids, id);

	for (const String& name : class_names)
		AddApplicableNodes(styled_node_index.classes, name);

	AddApplicableNodes(styled_node_index.tags, tag);

	// Also check all remaining nodes that don't contain any indexed requirements.
	for (const StyleSheetNode* node : styled_node_index.other)
	{
		if (node->IsApplicable(element))
			applicable_nodes.push_back(node);
	}

	// If this element definition won't actually store any information, don't bother with it.
	if (applicable_nodes.empty())
		return nullptr;

	// Sort the applicable nodes by specificity first, then by pointer value in case we have duplicate specificities.
	std::sort(applicable_nodes.begin(), applicable_nodes.end(), [](const StyleSheetNode* a, const StyleSheetNode* b) {
		const int a_specificity = a->GetSpecificity();
		const int b_specificity = b->GetSpecificity();
		if (a_specificity == b_specificity)
			return a < b;
		return a_specificity < b_specificity;
	});

	// Check if this puppy has already been cached in the node index.
	SharedPtr<const ElementDefinition>& definition = node_cache[applicable_nodes];
	if (!definition)
	{
		// Otherwise, create a new definition and add it to our cache.
		definition = MakeShared<const ElementDefinition>(applicable_nodes);
	}

	return definition;
}

} // namespace Rml
