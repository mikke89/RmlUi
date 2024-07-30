/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2019 Michael R. P. Ragazzon
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

#include "../../Include/RmlUi/Core/Spritesheet.h"
#include "../../Include/RmlUi/Core/Log.h"

namespace Rml {

Spritesheet::Spritesheet(const String& name, const String& source, const String& document_path, int definition_line_number, float display_scale) :
	name(name), definition_line_number(definition_line_number), display_scale(display_scale), texture_source(source, document_path)
{}

bool SpritesheetList::AddSpriteSheet(const String& name, const String& image_source, const String& definition_source, int definition_line_number,
	float display_scale, const SpriteDefinitionList& sprite_definitions)
{
	auto sprite_sheet_ptr = MakeShared<Spritesheet>(name, image_source, definition_source, definition_line_number, display_scale);
	Spritesheet* sprite_sheet = sprite_sheet_ptr.get();
	spritesheets.push_back(std::move(sprite_sheet_ptr));

	sprite_map.reserve(sprite_map.size() + sprite_definitions.size());

	// Insert all the sprites, overwriting any existing sprites already defined in the current media block scope.
	for (auto& sprite_definition : sprite_definitions)
	{
		const String& sprite_name = sprite_definition.first;
		const Rectanglef& sprite_rectangle = sprite_definition.second;

		Sprite& new_sprite = sprite_map[sprite_name];
		if (new_sprite.sprite_sheet)
		{
			Log::Message(Log::LT_WARNING, "Sprite '%s' was overwritten due to duplicate names at the same block scope. Declared at %s:%d and %s:%d",
				sprite_name.c_str(), new_sprite.sprite_sheet->texture_source.GetDefinitionSource().c_str(),
				new_sprite.sprite_sheet->definition_line_number, definition_source.c_str(), definition_line_number);
		}

		new_sprite = Sprite{sprite_rectangle, sprite_sheet};
	}

	return true;
}

const Sprite* SpritesheetList::GetSprite(const String& name) const
{
	auto it = sprite_map.find(name);
	if (it != sprite_map.end())
		return &it->second;
	return nullptr;
}

void SpritesheetList::Merge(const SpritesheetList& other)
{
	spritesheets.insert(spritesheets.end(), other.spritesheets.begin(), other.spritesheets.end());
	sprite_map.reserve(sprite_map.size() + other.sprite_map.size());

	for (auto& pair : other.sprite_map)
	{
		const String& sprite_name = pair.first;
		const Sprite& sprite = pair.second;

		// Add the sprite into our map. If a sprite with the same name exists, it will be overwritten by the other sprite.
		sprite_map[sprite_name] = sprite;
	}
}

void SpritesheetList::Reserve(size_t size_sprite_sheets, size_t size_sprites)
{
	spritesheets.reserve(size_sprite_sheets);
	sprite_map.reserve(size_sprites);
}

size_t SpritesheetList::NumSpriteSheets() const
{
	return spritesheets.size();
}

size_t SpritesheetList::NumSprites() const
{
	return sprite_map.size();
}

} // namespace Rml
