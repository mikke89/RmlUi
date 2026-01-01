#pragma once

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
