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

#ifndef RMLUICOREDECORATORTILEDINSTANCER_H
#define RMLUICOREDECORATORTILEDINSTANCER_H

#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "DecoratorTiled.h"

namespace Rml {
namespace Core {

class StyleSheet;

/**
	@author Peter Curry
 */


class DecoratorTiledInstancer : public DecoratorInstancer
{
public:
	DecoratorTiledInstancer(size_t num_tiles);

protected:
	/// Adds the property declarations for a tile.
	/// @param[in] name The name of the tile property.
	/// @param[in] register_fit_modes If true, the tile will have the fit modes registered.
	void RegisterTileProperty(const String& name, bool register_fit_modes = false);

	/// Retrieves all the properties for a tile from the property dictionary.
	/// @param[out] tile The tile structure for storing the tile properties.
	/// @param[out] textures Holds the textures declared for the tile.
	/// @param[in] properties The user-defined list of parameters for the decorator.
	/// @param[in] interface An interface for querying the active style sheet.
	bool GetTileProperties(DecoratorTiled::Tile* tiles, Texture* textures, size_t num_tiles_and_textures, const PropertyDictionary& properties, const DecoratorInstancerInterface& interface) const;

private:
	struct TilePropertyIds {
		PropertyId src, fit, align_x, align_y, orientation;
	};

	std::vector<TilePropertyIds> tile_property_ids;
};

}
}

#endif
