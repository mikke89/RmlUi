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

#include "precompiled.h"
#include "DecoratorTiledInstancer.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {
namespace Core {

DecoratorTiledInstancer::DecoratorTiledInstancer(size_t num_tiles)
{
	tile_property_ids.reserve(num_tiles);
}

// Adds the property declarations for a tile.
void DecoratorTiledInstancer::RegisterTileProperty(const String& name, bool register_repeat_modes)
{
	TilePropertyIds ids = {};

	ids.src = RegisterProperty(CreateString(32, "%s-src", name.c_str()), "").AddParser("string").GetId();
	ids.x = RegisterProperty(CreateString(32, "%s-x", name.c_str()), "0px").AddParser("number_length_percent").GetId();
	ids.y = RegisterProperty(CreateString(32, "%s-y", name.c_str()), "0px").AddParser("number_length_percent").GetId();
	ids.width = RegisterProperty(CreateString(32, "%s-width", name.c_str()), "1.0").AddParser("number_length_percent").GetId();
	ids.height = RegisterProperty(CreateString(32, "%s-height", name.c_str()), "1.0").AddParser("number_length_percent").GetId();
	RegisterShorthand(CreateString(32, "%s-pos", name.c_str()), CreateString(64, "%s-x, %s-y", name.c_str(), name.c_str()), ShorthandType::FallThrough);
	RegisterShorthand(CreateString(32, "%s-size", name.c_str()), CreateString(64, "%s-width, %s-height", name.c_str(), name.c_str()), ShorthandType::FallThrough);

	ids.orientation = RegisterProperty(CreateString(32, "%s-orientation", name.c_str()), "none")
		.AddParser("keyword", "none, rotate-90, rotate-180, rotate-270, flip-horizontal, flip-vertical")
		.GetId();

	if (register_repeat_modes)
	{
		ids.repeat = RegisterProperty(CreateString(32, "%s-repeat", name.c_str()), "stretch")
			.AddParser("keyword", "stretch, clamp-stretch, clamp-truncate, repeat-stretch, repeat-truncate")
			.GetId();
		RegisterShorthand(name, CreateString(256, "%s-src, %s-repeat, %s-x, %s-y, %s-width, %s-height, %s-orientation", name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str()), ShorthandType::FallThrough);
	}
	else
	{
		ids.repeat = PropertyId::Invalid;
		RegisterShorthand(name, CreateString(256, "%s-src, %s-x, %s-y, %s-width, %s-height, %s-orientation", name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str()), ShorthandType::FallThrough);
	}

	tile_property_ids.push_back(ids);
}


// Loads a single texture coordinate value from the properties.
static void LoadTexCoord(const Property& property, float& tex_coord, bool& tex_coord_absolute)
{
	tex_coord = property.value.Get< float >();
	if (property.unit == Property::PX)
	{
		tex_coord_absolute = true;
	}
	else
	{
		tex_coord_absolute = false;
		if (property.unit == Property::PERCENT)
			tex_coord *= 0.01f;
	}
}


// Retrieves all the properties for a tile from the property dictionary.
bool DecoratorTiledInstancer::GetTileProperties(DecoratorTiled::Tile* tiles, Texture* textures, size_t num_tiles_and_textures, const PropertyDictionary& properties, const DecoratorInstancerInterface& interface) const
{
	RMLUI_ASSERT(num_tiles_and_textures == tile_property_ids.size());

	String previous_texture_name;
	Texture previous_texture;

	for(size_t i = 0; i < num_tiles_and_textures; i++)
	{
		const TilePropertyIds& ids = tile_property_ids[i];

		const Property* src_property = properties.GetProperty(ids.src);
		const String texture_name = src_property->Get< String >();

		// Skip the tile if it has no source name.
		// Declaring the name 'auto' is the same as an empty string. This gives an easy way to skip certain
		// tiles in a shorthand since we can't always declare an empty string.
		static const String auto_str = "auto";
		if (texture_name.empty() || texture_name == auto_str)
			continue;

		// We are required to set default values before instancing the tile, thus, all properties should always be dereferencable.
		// If the debugger captures a zero-dereference, check that all properties for every tile is set and default values are set just before instancing.

		DecoratorTiled::Tile& tile = tiles[i];
		Texture& texture = textures[i];

		// A tile is always either a sprite or a texture with position and dimensions specified.
		if (const Sprite * sprite = interface.GetSprite(texture_name))
		{
			tile.position.x = sprite->rectangle.x;
			tile.position.y = sprite->rectangle.y;
			tile.size.x = sprite->rectangle.width;
			tile.size.y = sprite->rectangle.height;

			tile.position_absolute[0] = tile.position_absolute[1] = true;
			tile.size_absolute[0] = tile.size_absolute[1] = true;

			texture = sprite->sprite_sheet->texture;
		}
		else
		{
			LoadTexCoord(*properties.GetProperty(ids.x), tile.position.x, tile.position_absolute[0]);
			LoadTexCoord(*properties.GetProperty(ids.y), tile.position.y, tile.position_absolute[1]);
			LoadTexCoord(*properties.GetProperty(ids.width), tile.size.x, tile.size_absolute[0]);
			LoadTexCoord(*properties.GetProperty(ids.height), tile.size.y, tile.size_absolute[1]);

			// Since the common use case is to specify the same texture for all tiles, we
			// check the previous texture first before fetching from the global database.
			if (texture_name == previous_texture_name)
			{
				texture = previous_texture;
			}
			else if (src_property->source && texture.Load(texture_name, src_property->source->path))
			{
				previous_texture_name = texture_name;
				previous_texture = texture;
			}
			else
			{
				auto& source = src_property->source;
				Log::Message(Log::LT_WARNING, "Texture name '%s' is neither a valid sprite name nor a texture file. Specified in decorator at %s:%d.", texture_name.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
				return false;
			}
		}

		if(ids.repeat != PropertyId::Invalid)
		{
			const Property& repeat_property = *properties.GetProperty(ids.repeat);
			tile.repeat_mode = (DecoratorTiled::TileRepeatMode)repeat_property.value.Get< int >();
		}

		if (ids.orientation != PropertyId::Invalid)
		{
			const Property& repeat_property = *properties.GetProperty(ids.orientation);
			tile.orientation = (DecoratorTiled::TileOrientation)repeat_property.value.Get< int >();
		}
	}

	return true;
}

}
}
