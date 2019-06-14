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

#include "precompiled.h"
#include "DecoratorTiledInstancer.h"
#include "../../Include/Rocket/Core/PropertyDefinition.h"

namespace Rocket {
namespace Core {

DecoratorTiledInstancer::~DecoratorTiledInstancer()
{
}

// Adds the property declarations for a tile.
void DecoratorTiledInstancer::RegisterTileProperty(const String& name, bool register_repeat_modes)
{
	TilePropertyIds tile = {};

	tile.src = RegisterProperty(CreateString(32, "%s-src", name.c_str()), "").AddParser("string").GetId();
	tile.s_begin = RegisterProperty(CreateString(32, "%s-s-begin", name.c_str()), "0").AddParser("length").GetId();
	tile.s_end = RegisterProperty(CreateString(32, "%s-s-end", name.c_str()), "1").AddParser("length").GetId();
	tile.t_begin = RegisterProperty(CreateString(32, "%s-t-begin", name.c_str()), "0").AddParser("length").GetId();
	tile.t_end = RegisterProperty(CreateString(32, "%s-t-end", name.c_str()), "1").AddParser("length").GetId();
	RegisterShorthand(CreateString(32, "%s-s", name.c_str()), CreateString(64, "%s-s-begin, %s-s-end", name.c_str(), name.c_str()), ShorthandType::FallThrough);
	RegisterShorthand(CreateString(32, "%s-t", name.c_str()), CreateString(64, "%s-t-begin, %s-t-end", name.c_str(), name.c_str()), ShorthandType::FallThrough);

	if (register_repeat_modes)
	{
		tile.repeat = RegisterProperty(CreateString(32, "%s-repeat", name.c_str()), "stretch")
			.AddParser("keyword", "stretch, clamp-stretch, clamp-truncate, repeat-stretch, repeat-truncate")
			.GetId();
		RegisterShorthand(name, CreateString(256, "%s-src, %s-repeat, %s-s-begin, %s-t-begin, %s-s-end, %s-t-end", name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str()), ShorthandType::FallThrough);
	}
	else
		RegisterShorthand(name, CreateString(256, "%s-src, %s-s-begin, %s-t-begin, %s-s-end, %s-t-end", name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str()), ShorthandType::FallThrough);

	// @performance: Reserve number of tiles on construction
	tile_property_ids.push_back(tile);
}

// Retrieves all the properties for a tile from the property dictionary.
void DecoratorTiledInstancer::GetTileProperties(size_t tile_index, DecoratorTiled::Tile& tile, String& texture_name, String& rcss_path, const PropertyDictionary& properties)
{
	ROCKET_ASSERT(tile_index < tile_property_ids.size());

	const TilePropertyIds& ids = tile_property_ids[tile_index];

	LoadTexCoord(properties, ids.s_begin, tile.texcoords[0].x, tile.texcoords_absolute[0][0]);
	LoadTexCoord(properties, ids.t_begin, tile.texcoords[0].y, tile.texcoords_absolute[0][1]);
	LoadTexCoord(properties, ids.s_end, tile.texcoords[1].x, tile.texcoords_absolute[1][0]);
	LoadTexCoord(properties, ids.t_end, tile.texcoords[1].y, tile.texcoords_absolute[1][1]);

	const Property* repeat_property = properties.GetProperty(ids.repeat);
	if (repeat_property != NULL)
		tile.repeat_mode = (DecoratorTiled::TileRepeatMode) repeat_property->value.Get< int >();

	const Property* texture_property = properties.GetProperty(ids.src);
	texture_name = texture_property->Get< String >();
	rcss_path = texture_property->source;

	// Declaring the name 'none' is the same as an empty string. This gives an easy way to skip certain
	// tiles in a shorthand since we can't always declare an empty string.
	static const String none_texture_name = "none";
	if (texture_name == none_texture_name)
		texture_name.clear();
}

// Loads a single texture coordinate value from the properties.
void DecoratorTiledInstancer::LoadTexCoord(const PropertyDictionary& properties, PropertyId id, float& tex_coord, bool& tex_coord_absolute)
{
	const Property* property = properties.GetProperty(id);
	if (property == NULL)
		return;

	tex_coord = property->value.Get< float >();
	if (property->unit == Property::PX)
		tex_coord_absolute = true;
	else
	{
		tex_coord_absolute = false;
		if (property->unit == Property::PERCENT)
			tex_coord *= 0.01f;
	}
}

}
}
