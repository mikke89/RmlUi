/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2019 Michael R. P. Ragazzon
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

#include "../../Include/RmlUi/Core/Spritesheet.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/Log.h"

namespace Rml {

bool SpritesheetList::AddSpriteSheet(const String& name, const String& image_source, const String& definition_source, int definition_line_number, const SpriteDefinitionList& sprite_definitions)
{
	// Load the texture
	Texture texture;
	texture.Set(image_source, definition_source);

	auto sprite_sheet = MakeShared<Spritesheet>(name, image_source, definition_source, definition_line_number, texture);
	auto result = spritesheet_map.emplace(name, sprite_sheet);
	if (!result.second)
	{
		Log::Message(Log::LT_WARNING, "Spritesheet '%s' has the same name as another spritesheet, ignored. See %s:%d", name.c_str(), definition_source.c_str(), definition_line_number);
		return false;
	}

	StringList& sprite_names = sprite_sheet->sprite_names;

	// Insert all the sprites with names not already defined in the global sprite list.
	for (auto& sprite_definition : sprite_definitions)
	{
		const String& sprite_name = sprite_definition.first;
		const Rectangle& sprite_rectangle = sprite_definition.second;
		auto sprite_result = sprite_map.emplace(sprite_name, Sprite{ sprite_rectangle, sprite_sheet.get() });
		if (sprite_result.second)
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



const Sprite* SpritesheetList::GetSprite(const String& name) const
{
	auto it = sprite_map.find(name);
	if (it != sprite_map.end())
		return &it->second;
	return nullptr;
}


void SpritesheetList::Merge(const SpritesheetList& other)
{
	for (auto& pair : other.spritesheet_map)
	{
		auto sheet_result = spritesheet_map.emplace(pair);
		const String& sheet_name = sheet_result.first->first;
		const Spritesheet& sheet = *sheet_result.first->second;
		bool sheet_inserted = sheet_result.second;
		
		if (sheet_inserted)
		{
			// The sprite sheet was succesfully added, now try to add each sprite to the global list.
			for (const String& sprite_name  : sheet.sprite_names)
			{
				// Lookup the sprite in the other map.
				auto it_sprite = other.sprite_map.find(sprite_name);
				if (it_sprite != other.sprite_map.end())
				{
					// Add the sprite into our map. Each sprite name must be unique.
					auto sprite_result = sprite_map.emplace(sprite_name, it_sprite->second);
					bool inserted = sprite_result.second;

					if (!inserted)
					{
						Log::Message(Log::LT_WARNING, "Duplicate sprite name '%s' found while merging style sheets, defined in %s:%d.", sprite_name.c_str(), sheet.definition_source.c_str(), sheet.definition_line_number);
					}
				}
				else
				{
					RMLUI_ERRORMSG("Something went wrong while merging style sheets.");
				}
			}
		}
		else
		{
			Log::Message(Log::LT_WARNING, "Duplicate sprite sheet name '%s' found while merging style sheets, defined in %s:%d.", sheet_name.c_str(), sheet.definition_source.c_str(), sheet.definition_line_number);
		}
	}
}

void SpritesheetList::Reserve(size_t size_sprite_sheets, size_t size_sprites) 
{ 
	spritesheet_map.reserve(size_sprite_sheets);
	sprite_map.reserve(size_sprites);
}

size_t SpritesheetList::NumSpriteSheets() const 
{
	return spritesheet_map.size();
}

size_t SpritesheetList::NumSprites() const 
{ 
	return sprite_map.size();
}

String SpritesheetList::ToString() const
{
	String result = CreateString(100, "#SpriteSheets: %d\n", spritesheet_map.size());

	for (auto& sheet : spritesheet_map)
	{
		result += CreateString(100, "  Sheet '%s'.   #Sprites %d.\n", sheet.first.c_str(), sheet.second->sprite_names.size());
	}

	result += CreateString(100, "\n#Sprites: %d\n", sprite_map.size());
	for (auto& sprite : sprite_map)
	{
		result += CreateString(100, "  In '%s': %s\n", sprite.second.sprite_sheet->name.c_str(), sprite.first.c_str());
	}

	return result;
}

} // namespace Rml
