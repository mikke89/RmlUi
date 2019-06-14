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
	Decorator* decorator = nullptr;
};

struct Sprite {
	Rectangle rectangle;
	Spritesheet* sprite_sheet;
	String name;
};

struct Spritesheet {
	String name;
	String image_source;
	String definition_source;
	int definition_line_number;
	Texture texture;
	std::vector<std::unique_ptr<Sprite>> sprites;

	Spritesheet(const String& name, const String& definition_source, int definition_line_number) : name(name), definition_source(definition_source), definition_line_number(definition_line_number) {}

	void AddSprite(const String& name, Rectangle rectangle)
	{
		sprites.emplace_back(new Sprite{ rectangle, this, name });
	}
};

using SpriteMap = UnorderedMap<String, const Sprite*>;
using SpritesheetMap = UnorderedMap<String, std::unique_ptr<Spritesheet>>;

class SpriteSheetList {
public:
	bool AddSpriteSheet(std::unique_ptr<Spritesheet> in_sprite_sheet) {
		ROCKET_ASSERT(in_sprite_sheet);
		Spritesheet& spritesheet = *in_sprite_sheet;
		auto result = spritesheet_map.emplace(spritesheet.name, std::move(in_sprite_sheet));
		if (!result.second)
		{
			Log::Message(Log::LT_WARNING, "Spritesheet '%s' has the same name as another spritesheet, skipped. See %s:%d", spritesheet.name.c_str(), spritesheet.definition_source.c_str(), spritesheet.definition_line_number);
			return false;
		}

		auto& sprites = spritesheet.sprites;
		for (auto it = sprites.begin(); it != sprites.end();)
		{
			ROCKET_ASSERT(*it);
			const String& name = (*it)->name;
			auto result = sprite_map.emplace(name, it->get());
			if (!result.second)
			{
				Log::Message(Log::LT_WARNING, "Sprite '%s' has the same name as another sprite, skipped. See %s:%d", name.c_str(), spritesheet.definition_source.c_str(), spritesheet.definition_line_number);
				it = sprites.erase(it);
			}
			else
				++it;
		}

		// Load the texture
		spritesheet.texture.Load(spritesheet.image_source, spritesheet.definition_source);
		
		return true;
	}

	const Sprite* GetSprite(const String& name)
	{
		auto it = sprite_map.find(name);
		if (it != sprite_map.end())
			return it->second;
		return nullptr;
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
	Decorator* GetDecorator(const String& name) const;

	Decorator* GetOrInstanceDecorator(const String& decorator_value, const String& source_file, int source_line_number);

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

	SpriteSheetList sprite_sheets;

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
