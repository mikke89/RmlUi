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

#ifndef ROCKETCORESTYLESHEET_H
#define ROCKETCORESTYLESHEET_H

#include "Dictionary.h"
#include "ReferenceCountable.h"
#include <set>
#include "PropertyDictionary.h"
#include "Texture.h"

namespace Rocket {
namespace Core {

class Element;
class ElementDefinition;
class StyleSheetNode;
class Decorator;
struct Spritesheet;

struct KeyframeBlock {
	float normalized_time;  // [0, 1]
	PropertyDictionary properties;
};
struct Keyframes {
	std::vector<PropertyId> property_ids;
	std::vector<KeyframeBlock> blocks;
};
typedef UnorderedMap<String, Keyframes> KeyframesMap;

struct DecoratorSpecification {
	String decorator_type;
	PropertyDictionary properties;
	std::shared_ptr<Decorator> decorator;
};

struct Sprite {
	Rectangle rectangle;
	Spritesheet* sprite_sheet;
};

struct Spritesheet {
	String name;
	String image_source;
	String definition_source;
	int definition_line_number;
	Texture texture;
	StringList sprite_names; 

	Spritesheet(const String& name, const String& image_source, const String& definition_source, int definition_line_number, const Texture& texture)
		: name(name), image_source(image_source), definition_source(definition_source), definition_line_number(definition_line_number), texture(texture) {}
};

using SpriteMap = UnorderedMap<String, Sprite>;
using SpritesheetMap = UnorderedMap<String, std::shared_ptr<Spritesheet>>;
using SpriteDefinitionList = std::vector<std::pair<String, Rectangle>>;

class SpritesheetList {
public:

	bool AddSpriteSheet(const String& name, const String& image_source, const String& definition_source, int definition_line_number, const SpriteDefinitionList& sprite_definitions)
	{
		// Load the texture
		Texture texture;
		if (!texture.Load(image_source, definition_source))
		{
			Log::Message(Log::LT_WARNING, "Could not load image '%s' specified in spritesheet '%s' at %s:%d", image_source.c_str(), name.c_str(), definition_source.c_str(), definition_line_number);
			return false;
		}

		auto result = spritesheet_map.emplace(name, std::make_shared<Spritesheet>(name, image_source, definition_source, definition_line_number, texture));
		if (!result.second)
		{
			Log::Message(Log::LT_WARNING, "Spritesheet '%s' has the same name as another spritesheet, ignored. See %s:%d", name.c_str(), definition_source.c_str(), definition_line_number);
			return false;
		}

		Spritesheet& spritesheet = *result.first->second;
		StringList& sprite_names = spritesheet.sprite_names;
		
		// Insert all the sprites with names not already defined in the global sprite list.
		int num_removed_sprite_names = 0;
		for (auto& sprite_definition : sprite_definitions)
		{
			const String& sprite_name = sprite_definition.first;
			const Rectangle& sprite_rectangle = sprite_definition.second;
			auto result = sprite_map.emplace(sprite_name, Sprite{ sprite_rectangle, &spritesheet });
			if (result.second)
			{
				sprite_names.push_back(sprite_name);
			}
			else
			{
				Log::Message(Log::LT_WARNING, "Sprite '%s' has the same name as an existing sprite, skipped. See %s:%d", sprite_name.c_str(), definition_source.c_str(), definition_line_number);
			}
		}

		return true;
	}

	// Note: The pointer is invalidated whenever another sprite is added. Do not store it around.
	const Sprite* GetSprite(const String& name) const
	{
		auto it = sprite_map.find(name);
		if (it != sprite_map.end())
			return &it->second;
		return nullptr;
	}

	// Merge other into this
	void Merge(const SpritesheetList& other)
	{
		for (auto& pair : other.spritesheet_map)
		{
			auto sheet_result = spritesheet_map.emplace(pair);
			bool sheet_inserted = sheet_result.second;
			if (sheet_inserted)
			{
				const String& sheet_name = sheet_result.first->first;
				Spritesheet& sheet = *sheet_result.first->second;

				// The spritesheet was succesfully added, now try to add each sprite to the global list.
				// Each sprite name must be unique, if we fail, we must also remove the sprite from the spritesheet.
				for (auto it = sheet.sprite_names.begin(); it != sheet.sprite_names.end(); )
				{
					const String& sprite_name = *it;
					auto it_sprite = other.sprite_map.find(sprite_name);
					bool success = false;
					if(it_sprite != other.sprite_map.end())
					{
						auto sprite_result = sprite_map.emplace(*it, it_sprite->second);
						success = sprite_result.second;
					}

					if (success)
					{
						++it;
					}
					else
					{
						Log::Message(Log::LT_WARNING, "Duplicate sprite name '%s' found while merging style sheets, defined in %s:%d.", sprite_name.c_str(), sheet.definition_source.c_str(), sheet.definition_line_number);
						it = sheet.sprite_names.erase(it);
					}
				}
			}
		}
	}


private:
	SpritesheetMap spritesheet_map;
	SpriteMap sprite_map;
};

using DecoratorSpecificationMap = UnorderedMap<String, DecoratorSpecification>;


/**
	StyleSheet maintains a single stylesheet definition. A stylesheet can be combined with another stylesheet to create
	a new, merged stylesheet.

	@author Lloyd Weehuizen
 */

class ROCKETCORE_API StyleSheet : public ReferenceCountable
{
public:
	typedef std::unordered_set< StyleSheetNode* > NodeList;
	typedef UnorderedMap< String, NodeList > NodeIndex;

	StyleSheet();
	virtual ~StyleSheet();

	/// Loads a style from a CSS definition.
	bool LoadStyleSheet(Stream* stream);

	/// Combines this style sheet with another one, producing a new sheet.
	StyleSheet* CombineStyleSheet(const StyleSheet* sheet) const;
	/// Builds the node index for a combined style sheet.
	void BuildNodeIndex();

	/// Returns the Keyframes of the given name, or null if it does not exist.
	Keyframes* GetKeyframes(const String& name);

	/// Returns the Decorator of the given name, or null if it does not exist.
	std::shared_ptr<Decorator> GetDecorator(const String& name) const;

	const Sprite* GetSprite(const String& name) const;

	std::shared_ptr<Decorator> GetOrInstanceDecorator(const String& decorator_value, const String& source_file, int source_line_number);

	/// Returns the compiled element definition for a given element hierarchy. A reference count will be added for the
	/// caller, so another should not be added. The definition should be released by removing the reference count.
	ElementDefinition* GetElementDefinition(const Element* element) const;

protected:
	/// Destroys the style sheet.
	virtual void OnReferenceDeactivate();

private:
	// Root level node, attributes from special nodes like "body" get added to this node
	StyleSheetNode* root;

	// The maximum specificity offset used in this style sheet to distinguish between properties in
	// similarly-specific rules, but declared on different lines. When style sheets are merged, the
	// more-specific style sheet (ie, coming further 'down' the include path) adds the offset of
	// the less-specific style sheet onto its offset, thereby ensuring its properties take
	// precedence in the event of a conflict.
	int specificity_offset;

	// Name of every @keyframes mapped to their keys
	KeyframesMap keyframes;

	// Name of every @decorator mapped to their specification
	DecoratorSpecificationMap decorator_map;

	SpritesheetList spritesheet_list;

	// Map of only nodes with actual style information.
	NodeIndex styled_node_index;
	// Map of every node, even empty, un-styled, nodes.
	NodeIndex complete_node_index;

	typedef UnorderedMap< size_t, ElementDefinition* > ElementDefinitionCache;
	// Index of element addresses to element definitions.
	mutable ElementDefinitionCache address_cache;
	// Index of node sets to element definitions.
	mutable ElementDefinitionCache node_cache;
};

}
}

#endif
