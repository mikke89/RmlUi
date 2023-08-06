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

#include "DecoratorTiled.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Math.h"
#include <algorithm>

namespace Rml {

DecoratorTiled::DecoratorTiled() {}

DecoratorTiled::~DecoratorTiled() {}

static const Vector2f oriented_texcoords[4][2] = {
	{Vector2f(0, 0), Vector2f(1, 1)}, // ORIENTATION_NONE
	{Vector2f(1, 0), Vector2f(0, 1)}, // FLIP_HORIZONTAL
	{Vector2f(0, 1), Vector2f(1, 0)}, // FLIP_VERTICAL
	{Vector2f(1, 1), Vector2f(0, 0)}  // ROTATE_180
};

DecoratorTiled::Tile::Tile() : display_scale(1), position(0, 0), size(0, 0)
{
	texture_index = -1;
	fit_mode = FILL;
	orientation = ORIENTATION_NONE;
}

void DecoratorTiled::Tile::CalculateDimensions(const Texture& texture) const
{
	if (!tile_data_calculated)
	{
		tile_data_calculated = true;
		tile_data = {};

		const Vector2f texture_dimensions(texture.GetDimensions());
		if (texture_dimensions.x == 0 || texture_dimensions.y == 0)
		{
			tile_data.size = Vector2f(0, 0);
			tile_data.texcoords[0] = Vector2f(0, 0);
			tile_data.texcoords[1] = Vector2f(0, 0);
		}
		else
		{
			// Need to scale the coordinates to normalized units and 'size' to absolute size (pixels)
			if (size.x == 0 && size.y == 0 && position.x == 0 && position.y == 0)
				tile_data.size = texture_dimensions;
			else
				tile_data.size = size;

			const Vector2f size_relative = tile_data.size / texture_dimensions;

			tile_data.size = Vector2f(Math::Absolute(tile_data.size.x), Math::Absolute(tile_data.size.y));

			tile_data.texcoords[0] = position / texture_dimensions;
			tile_data.texcoords[1] = size_relative + tile_data.texcoords[0];
		}
	}
}

Vector2f DecoratorTiled::Tile::GetNaturalDimensions(Element* element) const
{
	if (!tile_data_calculated)
		return Vector2f(0, 0);

	const float scale_raw_to_natural_dimensions = ElementUtilities::GetDensityIndependentPixelRatio(element) * display_scale;
	const Vector2f raw_dimensions = tile_data.size;

	return raw_dimensions * scale_raw_to_natural_dimensions;
}

void DecoratorTiled::Tile::GenerateGeometry(Vector<Vertex>& vertices, Vector<int>& indices, const ComputedValues& computed,
	const Vector2f surface_origin, const Vector2f surface_dimensions, const Vector2f tile_dimensions) const
{
	if (surface_dimensions.x <= 0 || surface_dimensions.y <= 0)
		return;

	float opacity = computed.opacity();
	Colourb quad_colour = computed.image_color();

	// Apply opacity
	quad_colour.alpha = (byte)(opacity * (float)quad_colour.alpha);

	if (!tile_data_calculated)
		return;

	// Generate the oriented texture coordinates for the tiles.
	Vector2f scaled_texcoords[2];
	for (int i = 0; i < 2; i++)
	{
		scaled_texcoords[i] = tile_data.texcoords[0] + oriented_texcoords[orientation][i] * (tile_data.texcoords[1] - tile_data.texcoords[0]);
	}

	Vector2f final_tile_dimensions;
	bool offset_and_clip_tile = false;
	Vector2f repeat_factor = Vector2f(1);

	switch (fit_mode)
	{
	case FILL:
	{
		final_tile_dimensions = surface_dimensions;
	}
	break;
	case CONTAIN:
	{
		Vector2f scale_factor = surface_dimensions / tile_dimensions;
		float min_factor = std::min(scale_factor.x, scale_factor.y);
		final_tile_dimensions = tile_dimensions * min_factor;

		offset_and_clip_tile = true;
	}
	break;
	case COVER:
	{
		Vector2f scale_factor = surface_dimensions / tile_dimensions;
		float max_factor = std::max(scale_factor.x, scale_factor.y);
		final_tile_dimensions = tile_dimensions * max_factor;

		offset_and_clip_tile = true;
	}
	break;
	case SCALE_NONE:
	{
		final_tile_dimensions = tile_dimensions;

		offset_and_clip_tile = true;
	}
	break;
	case SCALE_DOWN:
	{
		Vector2f scale_factor = surface_dimensions / tile_dimensions;
		float min_factor = std::min(scale_factor.x, scale_factor.y);
		if (min_factor < 1.0f)
			final_tile_dimensions = tile_dimensions * min_factor;
		else
			final_tile_dimensions = tile_dimensions;

		offset_and_clip_tile = true;
	}
	break;
	case REPEAT:
		final_tile_dimensions = surface_dimensions;
		repeat_factor = surface_dimensions / tile_dimensions;
	break;
	case REPEAT_X:
		final_tile_dimensions = Vector2f(surface_dimensions.x, tile_dimensions.y);
		repeat_factor.x = surface_dimensions.x / tile_dimensions.x;
		offset_and_clip_tile = true;
	break;
	case REPEAT_Y:
		final_tile_dimensions = Vector2f(tile_dimensions.x, surface_dimensions.y);
		repeat_factor.y = surface_dimensions.y / tile_dimensions.y;
		offset_and_clip_tile = true;
	break;
	}

	Vector2f tile_offset(0, 0);

	if (offset_and_clip_tile)
	{
		// Offset tile along each dimension.
		for (int i = 0; i < 2; i++)
		{
			switch (align[i].type)
			{
			case Style::LengthPercentage::Length: tile_offset[i] = align[i].value; break;
			case Style::LengthPercentage::Percentage:
				tile_offset[i] = (surface_dimensions[i] - final_tile_dimensions[i]) * align[i].value * 0.01f;
				break;
			}
		}
		tile_offset = tile_offset.Round();

		// Clip tile. See if our tile extends outside the boundary at either side, along each dimension.
		for (int i = 0; i < 2; i++)
		{
			// Left/right acts as top/bottom during the second iteration.
			float overshoot_left = std::max(-tile_offset[i], 0.0f);
			float overshoot_right = std::max(tile_offset[i] + final_tile_dimensions[i] - surface_dimensions[i], 0.0f);

			if (overshoot_left > 0.f || overshoot_right > 0.f)
			{
				float& left = scaled_texcoords[0][i];
				float& right = scaled_texcoords[1][i];
				float width = right - left;

				left += overshoot_left / final_tile_dimensions[i] * width;
				right -= overshoot_right / final_tile_dimensions[i] * width;

				final_tile_dimensions[i] -= overshoot_left + overshoot_right;
				tile_offset[i] += overshoot_left;
			}
		}
	}

	scaled_texcoords[0] *= repeat_factor;
	scaled_texcoords[1] *= repeat_factor;

	// Resize the vertex and index arrays to fit the new geometry.
	int index_offset = (int)vertices.size();
	vertices.resize(vertices.size() + 4);
	Vertex* new_vertices = &vertices[0] + index_offset;

	size_t num_indices = indices.size();
	indices.resize(indices.size() + 6);
	int* new_indices = &indices[0] + num_indices;

	// Generate the vertices for the tiled surface.
	Vector2f tile_position = (surface_origin + tile_offset);

	Math::SnapToPixelGrid(tile_position, final_tile_dimensions);

	GeometryUtilities::GenerateQuad(new_vertices, new_indices, tile_position, final_tile_dimensions, quad_colour, scaled_texcoords[0],
		scaled_texcoords[1], index_offset);
}

void DecoratorTiled::ScaleTileDimensions(Vector2f& tile_dimensions, float axis_value, Axis axis_enum) const
{
	int axis = static_cast<int>(axis_enum);
	if (tile_dimensions[axis] != axis_value)
	{
		tile_dimensions[1 - axis] = tile_dimensions[1 - axis] * (axis_value / tile_dimensions[axis]);
		tile_dimensions[axis] = axis_value;
	}
}

} // namespace Rml
