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
#include "StringCache.h"
#include "StyleSheetFactory.h"
#include "StyleSheetNode.h"
#include "StyleSheetParser.h"
#include "Utilities.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"
#include <algorithm>

namespace Rml {
namespace Core {

// Sorts style nodes based on specificity.
inline static bool StyleSheetNodeSort(const StyleSheetNode* lhs, const StyleSheetNode* rhs)
{
	return lhs->GetSpecificity() < rhs->GetSpecificity();
}

StyleSheet::StyleSheet()
{
	root = std::make_unique<StyleSheetNode>();
	specificity_offset = 0;
}

StyleSheet::~StyleSheet()
{
}

bool StyleSheet::LoadStyleSheet(Stream* stream, int begin_line_number)
{
	StyleSheetParser parser;
	specificity_offset = parser.Parse(root.get(), stream, *this, keyframes, decorator_map, spritesheet_list, begin_line_number);
	return specificity_offset >= 0;
}

/// Combines this style sheet with another one, producing a new sheet
SharedPtr<StyleSheet> StyleSheet::CombineStyleSheet(const StyleSheet& other_sheet) const
{
	RMLUI_ZoneScoped;

	SharedPtr<StyleSheet> new_sheet = std::make_shared<StyleSheet>();
	if (!new_sheet->root->MergeHierarchy(root.get()) ||
		!new_sheet->root->MergeHierarchy(other_sheet.root.get(), specificity_offset))
	{
		return nullptr;
	}

	// Any matching @keyframe names are overridden as per CSS rules
	new_sheet->keyframes.reserve(keyframes.size() + other_sheet.keyframes.size());
	new_sheet->keyframes = keyframes;
	for (auto& other_keyframes : other_sheet.keyframes)
	{
		new_sheet->keyframes[other_keyframes.first] = other_keyframes.second;
	}

	// Copy over the decorators, and replace any matching decorator names from other_sheet
	new_sheet->decorator_map.reserve(decorator_map.size() + other_sheet.decorator_map.size());
	new_sheet->decorator_map = decorator_map;
	for (auto& other_decorator: other_sheet.decorator_map)
	{
		new_sheet->decorator_map[other_decorator.first] = other_decorator.second;
	}

	new_sheet->spritesheet_list.Reserve(
		spritesheet_list.NumSpriteSheets() + other_sheet.spritesheet_list.NumSpriteSheets(),
		spritesheet_list.NumSprites() + other_sheet.spritesheet_list.NumSprites()
	);
	new_sheet->spritesheet_list = other_sheet.spritesheet_list;
	new_sheet->spritesheet_list.Merge(spritesheet_list);

	new_sheet->specificity_offset = specificity_offset + other_sheet.specificity_offset;
	return new_sheet;
}

// Builds the node index for a combined style sheet.
void StyleSheet::BuildNodeIndexAndOptimizeProperties()
{
	RMLUI_ZoneScoped;
	styled_node_index.clear();
	root->BuildIndexAndOptimizeProperties(styled_node_index, *this);
	root->SetStructurallyVolatileRecursive(false);
}

// Returns the Keyframes of the given name, or null if it does not exist.
Keyframes * StyleSheet::GetKeyframes(const String & name)
{
	auto it = keyframes.find(name);
	if (it != keyframes.end())
		return &(it->second);
	return nullptr;
}

SharedPtr<Decorator> StyleSheet::GetDecorator(const String& name) const
{
	auto it = decorator_map.find(name);
	if (it == decorator_map.end())
		return nullptr;
	return it->second.decorator;
}

const Sprite* StyleSheet::GetSprite(const String& name) const
{
	return spritesheet_list.GetSprite(name);
}

DecoratorsPtr StyleSheet::InstanceDecoratorsFromString(const String& decorator_string_value, const SharedPtr<const PropertySource>& source) const
{
	// Decorators are declared as
	//   decorator: <decorator-value>[, <decorator-value> ...];
	// Where <decorator-value> is either a @decorator name:
	//   decorator: invader-theme-background, ...;
	// or is an anonymous decorator with inline properties
	//   decorator: tiled-box( <shorthand properties> ), ...;
	
	Decorators decorators;
	if (decorator_string_value.empty() || decorator_string_value == NONE)
		return nullptr;

	const char* source_path = (source ? source->path.c_str() : "");
	const int source_line_number = (source ? source->line_number : 0);

	// Make sure we don't split inside the parenthesis since they may appear in decorator shorthands.
	StringList decorator_string_list;
	StringUtilities::ExpandString(decorator_string_list, decorator_string_value, ',', '(', ')');

	decorators.value = decorator_string_value;
	decorators.list.reserve(decorator_string_list.size());

	// Get or instance each decorator in the comma-separated string list
	for (const String& decorator_string : decorator_string_list)
	{
		const size_t shorthand_open = decorator_string.find('(');
		const size_t shorthand_close = decorator_string.rfind(')');
		const bool invalid_parenthesis = (shorthand_open == String::npos || shorthand_close == String::npos || shorthand_open >= shorthand_close);

		if (invalid_parenthesis)
		{
			// We found no parenthesis, that means the value must be a name of a @decorator rule, look it up
			SharedPtr<Decorator> decorator = GetDecorator(decorator_string);
			if (decorator)
				decorators.list.emplace_back(std::move(decorator));
			else
				Log::Message(Log::LT_WARNING, "Decorator name '%s' could not be found in any @decorator rule, declared at %s:%d", decorator_string.c_str(), source_path, source_line_number);
		}
		else
		{
			// Since we have parentheses it must be an anonymous decorator with inline properties
			const String type = StringUtilities::StripWhitespace(decorator_string.substr(0, shorthand_open));

			// Check for valid decorator type
			DecoratorInstancer* instancer = Factory::GetDecoratorInstancer(type);
			if (!instancer)
			{
				Log::Message(Log::LT_WARNING, "Decorator type '%s' not found, declared at %s:%d", type.c_str(), source_path, source_line_number);
				continue;
			}

			const String shorthand = decorator_string.substr(shorthand_open + 1, shorthand_close - shorthand_open - 1);
			const PropertySpecification& specification = instancer->GetPropertySpecification();

			// Parse the shorthand properties given by the 'decorator' shorthand property
			PropertyDictionary properties;
			if (!specification.ParsePropertyDeclaration(properties, "decorator", shorthand))
			{
				Log::Message(Log::LT_WARNING, "Could not parse decorator value '%s' at %s:%d", decorator_string.c_str(), source_path, source_line_number);
				continue;
			}

			// Set unspecified values to their defaults
			specification.SetPropertyDefaults(properties);
			
			properties.SetSourceOfAllProperties(source);

			SharedPtr<Decorator> decorator = instancer->InstanceDecorator(type, properties, DecoratorInstancerInterface(*this));

			if (decorator)
				decorators.list.emplace_back(std::move(decorator));
			else
			{
				Log::Message(Log::LT_WARNING, "Decorator '%s' could not be instanced, declared at %s:%d", decorator_string.c_str(), source_path, source_line_number);
				continue;
			}
		}
	}

	return std::make_shared<Decorators>(std::move(decorators));
}

FontEffectsPtr StyleSheet::InstanceFontEffectsFromString(const String& font_effect_string_value, const SharedPtr<const PropertySource>& source) const
{	
	// Font-effects are declared as
	//   font-effect: <font-effect-value>[, <font-effect-value> ...];
	// Where <font-effect-value> is declared with inline properties, e.g.
	//   font-effect: outline( 1px black ), ...;

	if (font_effect_string_value.empty() || font_effect_string_value == NONE)
		return nullptr;

	const char* source_path = (source ? source->path.c_str() : "");
	const int source_line_number = (source ? source->line_number : 0);

	FontEffects font_effects;

	// Make sure we don't split inside the parenthesis since they may appear in decorator shorthands.
	StringList font_effect_string_list;
	StringUtilities::ExpandString(font_effect_string_list, font_effect_string_value, ',', '(', ')');

	font_effects.value = font_effect_string_value;
	font_effects.list.reserve(font_effect_string_list.size());

	// Get or instance each decorator in the comma-separated string list
	for (const String& font_effect_string : font_effect_string_list)
	{
		const size_t shorthand_open = font_effect_string.find('(');
		const size_t shorthand_close = font_effect_string.rfind(')');
		const bool invalid_parenthesis = (shorthand_open == String::npos || shorthand_close == String::npos || shorthand_open >= shorthand_close);

		if (invalid_parenthesis)
		{
			// We found no parenthesis, font-effects can only be declared anonymously for now.
			Log::Message(Log::LT_WARNING, "Invalid syntax for font-effect '%s', declared at %s:%d", font_effect_string.c_str(), source_path, source_line_number);
		}
		else
		{
			// Since we have parentheses it must be an anonymous decorator with inline properties
			const String type = StringUtilities::StripWhitespace(font_effect_string.substr(0, shorthand_open));

			// Check for valid font-effect type
			FontEffectInstancer* instancer = Factory::GetFontEffectInstancer(type);
			if (!instancer)
			{
				Log::Message(Log::LT_WARNING, "Font-effect type '%s' not found, declared at %s:%d", type.c_str(), source_path, source_line_number);
				continue;
			}

			const String shorthand = font_effect_string.substr(shorthand_open + 1, shorthand_close - shorthand_open - 1);
			const PropertySpecification& specification = instancer->GetPropertySpecification();

			// Parse the shorthand properties given by the 'font-effect' shorthand property
			PropertyDictionary properties;
			if (!specification.ParsePropertyDeclaration(properties, "font-effect", shorthand))
			{
				Log::Message(Log::LT_WARNING, "Could not parse font-effect value '%s' at %s:%d", font_effect_string.c_str(), source_path, source_line_number);
				continue;
			}

			// Set unspecified values to their defaults
			specification.SetPropertyDefaults(properties);

			properties.SetSourceOfAllProperties(source);

			SharedPtr<FontEffect> font_effect = instancer->InstanceFontEffect(type, properties);
			if (font_effect)
			{
				// Create a unique hash value for the given type and values
				size_t fingerprint = std::hash<String>{}(type);
				for (const auto& id_value : properties.GetProperties())
					Utilities::HashCombine(fingerprint, id_value.second.Get<String>());

				font_effect->SetFingerprint(fingerprint);

				font_effects.list.emplace_back(std::move(font_effect));
			}
			else
			{
				Log::Message(Log::LT_WARNING, "Font-effect '%s' could not be instanced, declared at %s:%d", font_effect_string.c_str(), source_path, source_line_number);
				continue;
			}
		}
	}

	// Partition the list such that the back layer effects appear before the front layer effects
	std::stable_partition(font_effects.list.begin(), font_effects.list.end(), 
		[](const SharedPtr<const FontEffect>& effect) { return effect->GetLayer() == FontEffect::Layer::Back; }
	);

	return std::make_shared<FontEffects>(std::move(font_effects));
}

size_t StyleSheet::NodeHash(const String& tag, const String& id)
{
	size_t seed = 0;
	if (!tag.empty())
		seed = std::hash<String>()(tag);
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
	static std::vector< const StyleSheetNode* > applicable_nodes;
	applicable_nodes.clear();

	const String& tag = element->GetTagName();
	const String& id = element->GetId();

	// The styled_node_index is hashed with the tag and id of the RCSS rule. However, we must also check
	// the rules which don't have them defined, because they apply regardless of tag and id.
	std::array<size_t, 4> node_hash;
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
			for (StyleSheetNode* node : nodes)
			{
				if (node->IsApplicable(element))
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
	auto new_definition = std::make_shared<ElementDefinition>(applicable_nodes);
	node_cache[seed] = new_definition;

	return new_definition;
}

}
}
