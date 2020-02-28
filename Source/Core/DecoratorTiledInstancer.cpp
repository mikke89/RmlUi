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

#include "DecoratorTiledInstancer.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/Spritesheet.h"

namespace Rml {
namespace Core {

DecoratorTiledInstancer::DecoratorTiledInstancer(size_t num_tiles)
{
	tile_property_ids.reserve(num_tiles);
}

// Adds the property declarations for a tile.
void DecoratorTiledInstancer::RegisterTileProperty(const String& name, bool register_fit_modes)
{
	TilePropertyIds ids = {};

	ids.src = RegisterProperty(CreateString(32, "%s-src", name.c_str()), "").AddParser("string").GetId();

	String additional_modes;

	if (register_fit_modes)
	{
		String fit_name = CreateString(32, "%s-fit", name.c_str());
		ids.fit = RegisterProperty(fit_name, "fill")
			.AddParser("keyword", "fill, contain, cover, scale-none, scale-down")
			.GetId();

		String align_x_name = CreateString(32, "%s-align-x", name.c_str());
		ids.align_x = RegisterProperty(align_x_name, "center")
			.AddParser("keyword", "left, center, right")
			.AddParser("length_percent")
			.GetId();

		String align_y_name = CreateString(32, "%s-align-y", name.c_str());
		ids.align_y = RegisterProperty(align_y_name, "center")
			.AddParser("keyword", "top, center, bottom")
			.AddParser("length_percent")
			.GetId();

		additional_modes += ", " + fit_name + ", " + align_x_name + ", " + align_y_name;
	}

	ids.orientation = RegisterProperty(CreateString(32, "%s-orientation", name.c_str()), "none")
		.AddParser("keyword", "none, flip-horizontal, flip-vertical, rotate-180")
		.GetId();

	RegisterShorthand(name, CreateString(256, ("%s-src, %s-orientation" + additional_modes).c_str(),
		name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str()),
		ShorthandType::FallThrough);

	tile_property_ids.push_back(ids);
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

		// A tile is always either a sprite or an image.
		if (const Sprite * sprite = interface.GetSprite(texture_name))
		{
			tile.position.x = sprite->rectangle.x;
			tile.position.y = sprite->rectangle.y;
			tile.size.x = sprite->rectangle.width;
			tile.size.y = sprite->rectangle.height;

			texture = sprite->sprite_sheet->texture;
		}
		else
		{
			// No sprite found, we assume then that the name is an image source.
			// Since the common use case is to specify the same texture for all tiles, we
			// check the previous texture first before fetching from the global database.
			if (texture_name == previous_texture_name)
			{
				texture = previous_texture;
			}
			else if (src_property->source)
			{
				texture.Set(texture_name, src_property->source->path);
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

		if (ids.fit != PropertyId::Invalid)
		{
			RMLUI_ASSERT(ids.align_x != PropertyId::Invalid && ids.align_y != PropertyId::Invalid);
			const Property& fit_property = *properties.GetProperty(ids.fit);
			tile.fit_mode = (DecoratorTiled::TileFitMode)fit_property.value.Get< int >();

			const Property* align_properties[2] = {
				properties.GetProperty(ids.align_x),
				properties.GetProperty(ids.align_y)
			};

			for (int i = 0; i < 2; i++)
			{
				using Style::LengthPercentage;

				auto& align = tile.align[i];
				auto& property = *align_properties[i];
				if (property.unit == Property::KEYWORD)
				{
					enum { TOP_LEFT, CENTER, BOTTOM_RIGHT };
					switch (property.Get<int>())
					{
					case TOP_LEFT:     align = LengthPercentage(LengthPercentage::Percentage, 0.0f); break;
					case CENTER:       align = LengthPercentage(LengthPercentage::Percentage, 50.0f); break;
					case BOTTOM_RIGHT: align = LengthPercentage(LengthPercentage::Percentage, 100.0f); break;
					}
				}
				else if (property.unit == Property::PERCENT)
				{
					align = LengthPercentage(LengthPercentage::Percentage, property.Get<float>());
				}
				else if(property.unit == Property::PX) 
				{
					align = LengthPercentage(LengthPercentage::Length, property.Get<float>());
				}
				else
				{
					Log::Message(Log::LT_WARNING, "Decorator alignment value is '%s' which uses an unsupported unit (use px, %%, or keyword)", property.ToString().c_str());
				}
			}
		}

		if (ids.orientation != PropertyId::Invalid)
		{
			const Property& orientation_property = *properties.GetProperty(ids.orientation);
			tile.orientation = (DecoratorTiled::TileOrientation)orientation_property.value.Get< int >();
		}
	}

	return true;
}

}
}
