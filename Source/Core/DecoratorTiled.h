/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef RMLUI_CORE_DECORATORTILED_H
#define RMLUI_CORE_DECORATORTILED_H

#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Vertex.h"

namespace Rml {

class Texture;
struct Mesh;

/**
    Base class for tiled decorators.

    @author Peter Curry
 */

class DecoratorTiled : public Decorator {
public:
	DecoratorTiled();
	virtual ~DecoratorTiled();

	/**
	    Stores the orientation of a tile.
	 */
	enum TileOrientation {
		ORIENTATION_NONE, // No orientation.
		FLIP_HORIZONTAL,  // Flipped horizontally.
		FLIP_VERTICAL,    // Flipped vertically.
		ROTATE_180,       // Rotated 180 degrees clockwise.
	};
	/**
	    Stores the fit mode of a tile.
	 */
	enum TileFitMode {
		FILL,       // Tile is stretched to boundaries.
		CONTAIN,    // Tile is stretched to boundaries, keeping aspect ratio fixed, 'letter-boxed'.
		COVER,      // Tile is stretched to cover the boundaries, keeping aspect ratio fixed.
		SCALE_NONE, // Tile is never scaled.
		SCALE_DOWN, // Tile acts like 'scale-none' if smaller than boundaries, or like 'contain' otherwise.
		REPEAT,     // Tile is repeated on both x and y axis
		REPEAT_X,   // Tile is repeated on the x axis
		REPEAT_Y,   // Tile is repeated on the y axis
	};

	/**
	    Structure for storing the different tiles the tiled decorator uses internally over its
	    surface.

	    @author Peter Curry
	 */
	struct Tile {
		/// Constructs the tile with safe default values.
		Tile();

		/// Calculates the tile's dimensions from the texture and texture coordinates.
		void CalculateDimensions(Texture texture) const;
		/// Get the dimensions (in px) that this tile is ideally displayed as.
		/// Uses the dp-ratio of the current element and 'display_scale' to calculate the dimensions.
		Vector2f GetNaturalDimensions(Element* element) const;

		/// Generates geometry to render this tile across a surface.
		/// @param[out] vertices The array to store the generated vertex data.
		/// @param[out] indices The array to store the generated index data.
		/// @param[in] computed_values The computed values of the element being decorated.
		/// @param[in] surface_origin The starting point of the first tile to generate.
		/// @param[in] surface_dimensions The dimensions of the surface to be tiled.
		/// @param[in] tile_dimensions The dimensions to render this tile at.
		void GenerateGeometry(Mesh& mesh, const ComputedValues& computed_values, Vector2f surface_origin, Vector2f surface_dimensions,
			Vector2f tile_dimensions) const;

		struct TileData {
			Vector2f size;         // 'px' units
			Vector2f texcoords[2]; // relative units
		};

		int texture_index;

		// Scales the desired displayed size of the tile from raw pixel size. Eg. the 'display_scale' in a sprite sheet.
		float display_scale;

		// Position and size within the texture, absolute units (px)
		Vector2f position, size;

		mutable TileData tile_data;
		mutable bool tile_data_calculated = false;

		TileOrientation orientation;

		TileFitMode fit_mode;
		Style::LengthPercentage align[2];
	};

protected:
	enum class Axis { Horizontal, Vertical };

	/// Scales a tile dimensions by a fixed value along one axis.
	/// @param tile_dimensions[in, out] The tile dimensions to scale.
	/// @param axis_value[in] The fixed value to scale against.
	/// @param axis[in] The axis to scale against.
	void ScaleTileDimensions(Vector2f& tile_dimensions, float axis_value, Axis axis) const;
};

class DecoratorTiledInstancer : public DecoratorInstancer {
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
	/// @param[in] instancer_interface An interface for querying the active style sheet.
	bool GetTileProperties(DecoratorTiled::Tile* tiles, Texture* textures, size_t num_tiles_and_textures, const PropertyDictionary& properties,
		const DecoratorInstancerInterface& instancer_interface) const;

private:
	struct TilePropertyIds {
		PropertyId src, fit, align_x, align_y, orientation;
	};

	Vector<TilePropertyIds> tile_property_ids;
};

} // namespace Rml
#endif
