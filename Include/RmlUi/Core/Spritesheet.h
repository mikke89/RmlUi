/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
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

#ifndef RMLUISPRITESHEET_H
#define RMLUISPRITESHEET_H

#include "Texture.h"
#include "Types.h"

namespace Rml {

struct Spritesheet;

struct Sprite {
	Rectanglef rectangle; // in 'px' units
	const Spritesheet* sprite_sheet;
};
using SpriteMap = UnorderedMap<String, Sprite>; // key: sprite name (as given in @spritesheet)

/**
    Spritesheet holds a list of sprite names given in the @spritesheet at-rule in RCSS.
 */
struct Spritesheet {
	String name;
	int definition_line_number;
	float display_scale; // The inverse of the 'resolution' spritesheet property.
	TextureSource texture_source;

	Spritesheet(const String& name, const String& source, const String& document_path, int definition_line_number, float display_scale);
};

using SpriteDefinitionList = Vector<Pair<String, Rectanglef>>; // Sprite name and rectangle

/**
    SpritesheetList holds all the spritesheets and sprites given in a style sheet.
 */
class SpritesheetList {
public:
	/// Adds a new sprite sheet to the list and inserts all sprites with unique names into the global list.
	bool AddSpriteSheet(const String& name, const String& image_source, const String& definition_source, int definition_line_number,
		float display_scale, const SpriteDefinitionList& sprite_definitions);

	/// Get a sprite from its name if it exists.
	/// Note: The pointer is invalidated whenever another sprite is added. Do not store it around.
	const Sprite* GetSprite(const String& name) const;

	/// Merge 'other' into this. Sprites in 'other' will overwrite local sprites if they share the same name.
	void Merge(const SpritesheetList& other);

	void Reserve(size_t size_sprite_sheets, size_t size_sprites);
	size_t NumSpriteSheets() const;
	size_t NumSprites() const;

private:
	using Spritesheets = Vector<SharedPtr<const Spritesheet>>;

	Spritesheets spritesheets;
	SpriteMap sprite_map;
};

} // namespace Rml
#endif
