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
	RegisterProperty(CreateString(32, "%s-src", name.c_str()), "").AddParser("string");
	RegisterProperty(CreateString(32, "%s-s-begin", name.c_str()), "0").AddParser("length");
	RegisterProperty(CreateString(32, "%s-s-end", name.c_str()), "1").AddParser("length");
	RegisterProperty(CreateString(32, "%s-t-begin", name.c_str()), "0").AddParser("length");
	RegisterProperty(CreateString(32, "%s-t-end", name.c_str()), "1").AddParser("length");
	RegisterShorthand(CreateString(32, "%s-s", name.c_str()), CreateString(64, "%s-s-begin, %s-s-end", name.c_str(), name.c_str()));
	RegisterShorthand(CreateString(32, "%s-t", name.c_str()), CreateString(64, "%s-t-begin, %s-t-end", name.c_str(), name.c_str()));

	if (register_repeat_modes)
	{
		RegisterProperty(CreateString(32, "%s-repeat", name.c_str()), "stretch")
			.AddParser("keyword", "stretch, clamp-stretch, clamp-truncate, repeat-stretch, repeat-truncate");
		RegisterShorthand(name, CreateString(256, "%s-src, %s-repeat, %s-s-begin, %s-t-begin, %s-s-end, %s-t-end", name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str()));
	}
	else
		RegisterShorthand(name, CreateString(256, "%s-src, %s-s-begin, %s-t-begin, %s-s-end, %s-t-end", name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str()));
}

// Retrieves all the properties for a tile from the property dictionary.
void DecoratorTiledInstancer::GetTileProperties(DecoratorTiled::Tile& tile, String& texture_name, String& rcss_path, const PropertyDictionary& properties, const String& name)
{
	LoadTexCoord(properties, CreateString(32, "%s-s-begin", name.c_str()), tile.texcoords[0].x, tile.texcoords_absolute[0][0]);
	LoadTexCoord(properties, CreateString(32, "%s-t-begin", name.c_str()), tile.texcoords[0].y, tile.texcoords_absolute[0][1]);
	LoadTexCoord(properties, CreateString(32, "%s-s-end", name.c_str()), tile.texcoords[1].x, tile.texcoords_absolute[1][0]);
	LoadTexCoord(properties, CreateString(32, "%s-t-end", name.c_str()), tile.texcoords[1].y, tile.texcoords_absolute[1][1]);

	const Property* repeat_property = properties.GetProperty(CreateString(32, "%s-repeat", name.c_str()));
	if (repeat_property != NULL)
		tile.repeat_mode = (DecoratorTiled::TileRepeatMode) repeat_property->value.Get< int >();

	const Property* texture_property = properties.GetProperty(CreateString(32, "%s-src", name.c_str()));
	texture_name = texture_property->Get< String >();
	rcss_path = texture_property->source;
}

// Loads a single texture coordinate value from the properties.
void DecoratorTiledInstancer::LoadTexCoord(const PropertyDictionary& properties, const String& name, float& tex_coord, bool& tex_coord_absolute)
{
	const Property* property = properties.GetProperty(name);
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
