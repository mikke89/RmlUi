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

#ifndef RMLUI_CORE_DECORATORTILED_H
#define RMLUI_CORE_DECORATORTILED_H

#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Vertex.h"

namespace Rml {

struct Texture;

/**
	Base class for tiled decorators.

	@author Peter Curry
 */

class DecoratorTiled : public Decorator
{
public:
	DecoratorTiled();
	virtual ~DecoratorTiled();

	/**
		Stores the orientation of a tile.
	 */
	enum TileOrientation
	{
		ORIENTATION_NONE,       // No orientation.
		FLIP_HORIZONTAL,        // Flipped horizontally.
		FLIP_VERTICAL,          // Flipped vertically.
		ROTATE_180,             // Rotated 180 degrees clockwise.
	};
	/**
		Stores the fit mode of a tile.
	 */
	enum TileFitMode
	{
		FILL,       // Tile is stretched to boundaries.
		CONTAIN,    // Tile is stretched to boundaries, keeping aspect ratio fixed, 'letter-boxed'.
		COVER,      // Tile is stretched to cover the boundaries, keeping aspect ratio fixed.
		SCALE_NONE, // Tile is never scaled.
		SCALE_DOWN, // Tile acts like 'scale-none' if smaller than boundaries, or like 'contain' otherwise.
	};

	/**
		Structure for storing the different tiles the tiled decorator uses internally over its
		surface.

		@author Peter Curry
	 */
	struct Tile
	{
		/// Constructs the tile with safe default values.
		Tile();

		/// Calculates the tile's dimensions from the texture and texture coordinates.
		void CalculateDimensions(Element* element, const Texture& texture) const;
		/// Get this tile's dimensions.
		Vector2f GetDimensions(Element* element) const;

		/// Generates geometry to render this tile across a surface.
		/// @param[out] vertices The array to store the generated vertex data.
		/// @param[out] indices The array to store the generated index data.
		/// @param[in] element The element hosting the decorator.
		/// @param[in] surface_origin The starting point of the first tile to generate.
		/// @param[in] surface_dimensions The dimensions of the surface to be tiled.
		/// @param[in] tile_dimensions The dimensions to render this tile at.
		void GenerateGeometry(Vector< Vertex >& vertices, Vector< int >& indices, Element* element, const Vector2f& surface_origin, const Vector2f& surface_dimensions, const Vector2f& tile_dimensions) const;

		struct TileData
		{
			Vector2f size; // 'px' units
			Vector2f texcoords[2]; // relative units
		};

		using TileDataMap = SmallUnorderedMap< RenderInterface*, TileData >;

		int texture_index;

		// Position and size within the texture, absolute units (px)
		Vector2f position, size;

		mutable TileDataMap data;

		TileOrientation orientation;

		TileFitMode fit_mode;
		Style::LengthPercentage align[2]; 

	};

protected:
	/// Scales a tile dimensions by a fixed value along one axis.
	/// @param tile_dimensions[in, out] The tile dimensions to scale.
	/// @param axis_value[in] The fixed value to scale against.
	/// @param axis[in] The axis to scale against; either 0 (for x) or 1 (for y).
	void ScaleTileDimensions(Vector2f& tile_dimensions, float axis_value, int axis) const;
};

} // namespace Rml
#endif
