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

#include "precompiled.h"
#include "../../Include/RmlUi/Core/Spritesheet.h"

namespace Rml {
namespace Core {



bool SpritesheetList::AddSpriteSheet(const String& name, const String& image_source, const String& definition_source, int definition_line_number, const SpriteDefinitionList& sprite_definitions)
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
				if (it_sprite != other.sprite_map.end())
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

void SpritesheetList::Reserve(size_t size_sprite_sheets, size_t size_sprites) 
{ 
	spritesheet_map.reserve(size_sprite_sheets);
	// There seems to be a bug in robin hood unordered map (or we are doing something wrong). For certain sizes, we don't find any sprites during the merge after.
	//sprite_map.reserve(size_sprites);
}

size_t SpritesheetList::NumSpriteSheets() const 
{
	return spritesheet_map.size();
}

size_t SpritesheetList::NumSprites() const 
{ 
	return sprite_map.size();
}

}
}
