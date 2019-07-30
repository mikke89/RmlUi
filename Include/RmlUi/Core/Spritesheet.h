/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
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

#ifndef RMLUISPRITESHEET_H
#define RMLUISPRITESHEET_H

#include "Types.h"
#include "Texture.h"

namespace Rml {
namespace Core {

struct Spritesheet;


struct Rectangle {
	Rectangle(float x = 0, float y = 0, float width = 0, float height = 0) : x(x), y(y), width(width), height(height) {}
	float x, y, width, height;
};

struct Sprite {
	Rectangle rectangle; // in 'px' units
	const Spritesheet* sprite_sheet;
};
using SpriteMap = UnorderedMap<String, Sprite>; // key: sprite name (as given in @spritesheet)


/**
	Spritesheet holds a list of sprite names given in the @spritesheet at-rule in RCSS.
 */
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

using SpritesheetMap = SmallUnorderedMap<String, SharedPtr<const Spritesheet>>; // key: spritesheet name (as given in @spritesheet)
using SpriteDefinitionList = std::vector<std::pair<String, Rectangle>>; // Sprite name and rectangle


/**
	SpritesheetList holds all the spritesheets and sprites given in a style sheet.
 */
class SpritesheetList {
public:
	/// Adds a new sprite sheet to the list and inserts all sprites with unique names into the global list.
	bool AddSpriteSheet(const String& name, const String& image_source, const String& definition_source, int definition_line_number, const SpriteDefinitionList& sprite_definitions);

	/// Get a sprite from its name if it exists.
	/// Note: The pointer is invalidated whenever another sprite is added. Do not store it around.
	const Sprite* GetSprite(const String& name) const;

	/// Merge 'other' into this.
	void Merge(const SpritesheetList& other);

	void Reserve(size_t size_sprite_sheets, size_t size_sprites);
	size_t NumSpriteSheets() const;
	size_t NumSprites() const;

	String ToString() const;

private:
	SpritesheetMap spritesheet_map;
	SpriteMap sprite_map;
};


}
}

#endif
