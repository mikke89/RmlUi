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
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/Spritesheet.h"
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

void DecoratorTiled::Tile::CalculateDimensions(Texture texture) const
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

void DecoratorTiled::Tile::GenerateGeometry(Mesh& mesh, const ComputedValues& computed, const Vector2f surface_origin,
	const Vector2f surface_dimensions, const Vector2f tile_dimensions) const
{
	if (surface_dimensions.x <= 0 || surface_dimensions.y <= 0)
		return;

	const ColourbPremultiplied quad_colour = computed.image_color().ToPremultiplied(computed.opacity());

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

	// Generate the vertices for the tiled surface.
	Vector2f tile_position = (surface_origin + tile_offset);
	Math::SnapToPixelGrid(tile_position, final_tile_dimensions);

	MeshUtilities::GenerateQuad(mesh, tile_position, final_tile_dimensions, quad_colour, scaled_texcoords[0], scaled_texcoords[1]);
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

DecoratorTiledInstancer::DecoratorTiledInstancer(size_t num_tiles)
{
	tile_property_ids.reserve(num_tiles);
}

void DecoratorTiledInstancer::RegisterTileProperty(const String& name, bool register_fit_modes)
{
	TilePropertyIds ids = {};

	ids.src = RegisterProperty(CreateString("%s-src", name.c_str()), "").AddParser("string").GetId();

	String additional_modes;

	if (register_fit_modes)
	{
		String fit_name = CreateString("%s-fit", name.c_str());
		ids.fit = RegisterProperty(fit_name, "fill")
					  .AddParser("keyword", "fill, contain, cover, scale-none, scale-down, repeat, repeat-x, repeat-y")
					  .GetId();

		String align_x_name = CreateString("%s-align-x", name.c_str());
		ids.align_x = RegisterProperty(align_x_name, "center").AddParser("keyword", "left, center, right").AddParser("length_percent").GetId();

		String align_y_name = CreateString("%s-align-y", name.c_str());
		ids.align_y = RegisterProperty(align_y_name, "center").AddParser("keyword", "top, center, bottom").AddParser("length_percent").GetId();

		additional_modes += ", " + fit_name + ", " + align_x_name + ", " + align_y_name;
	}

	ids.orientation = RegisterProperty(CreateString("%s-orientation", name.c_str()), "none")
						  .AddParser("keyword", "none, flip-horizontal, flip-vertical, rotate-180")
						  .GetId();

	RegisterShorthand(name,
		CreateString(("%s-src, %s-orientation" + additional_modes).c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(),
			name.c_str()),
		ShorthandType::FallThrough);

	tile_property_ids.push_back(ids);
}

bool DecoratorTiledInstancer::GetTileProperties(DecoratorTiled::Tile* tiles, Texture* textures, size_t num_tiles_and_textures,
	const PropertyDictionary& properties, const DecoratorInstancerInterface& instancer_interface) const
{
	RMLUI_ASSERT(num_tiles_and_textures == tile_property_ids.size());

	String previous_texture_name;
	Texture previous_texture;

	for (size_t i = 0; i < num_tiles_and_textures; i++)
	{
		const TilePropertyIds& ids = tile_property_ids[i];

		const Property* src_property = properties.GetProperty(ids.src);
		const String texture_name = src_property->Get<String>();

		// Skip the tile if it has no source name.
		// Declaring the name 'auto' is the same as an empty string. This gives an easy way to skip certain
		// tiles in a shorthand since we can't always declare an empty string.
		if (texture_name.empty() || texture_name == "auto")
			continue;

		// We are required to set default values before instancing the tile, thus, all properties should always be
		// dereferencable. If the debugger captures a zero-dereference, check that all properties for every tile is set
		// and default values are set just before instancing.

		DecoratorTiled::Tile& tile = tiles[i];
		Texture& texture = textures[i];

		const Sprite* sprite = instancer_interface.GetSprite(texture_name);

		// A tile is always either a sprite or an image.
		if (sprite)
		{
			tile.position = sprite->rectangle.Position();
			tile.size = sprite->rectangle.Size();
			tile.display_scale = sprite->sprite_sheet->display_scale;

			texture = sprite->sprite_sheet->texture_source.GetTexture(instancer_interface.GetRenderManager());
		}
		else
		{
			// No sprite found, so assume that the name is an image source. Since the common use case is to specify the
			// same texture for all tiles, check the previous texture first before fetching from the global database.
			if (texture_name == previous_texture_name)
			{
				texture = previous_texture;
			}
			else
			{
				texture = instancer_interface.GetTexture(texture_name);

				if (!texture)
					return false;

				previous_texture_name = texture_name;
				previous_texture = texture;
			}
		}

		if (ids.fit != PropertyId::Invalid)
		{
			RMLUI_ASSERT(ids.align_x != PropertyId::Invalid && ids.align_y != PropertyId::Invalid);
			const Property& fit_property = *properties.GetProperty(ids.fit);
			tile.fit_mode = (DecoratorTiled::TileFitMode)fit_property.value.Get<int>();

			if (sprite &&
				(tile.fit_mode == DecoratorTiled::TileFitMode::REPEAT || tile.fit_mode == DecoratorTiled::TileFitMode::REPEAT_X ||
					tile.fit_mode == DecoratorTiled::TileFitMode::REPEAT_Y))
			{
				Log::Message(Log::LT_WARNING, "Decorator 'fit' value is '%s', which is incompatible with sprites", fit_property.ToString().c_str());
				return false;
			}

			const Property* align_properties[2] = {properties.GetProperty(ids.align_x), properties.GetProperty(ids.align_y)};

			for (int dimension = 0; dimension < 2; dimension++)
			{
				using Style::LengthPercentage;

				LengthPercentage& align = tile.align[dimension];
				const Property& property = *align_properties[dimension];
				if (property.unit == Unit::KEYWORD)
				{
					enum { TOP_LEFT, CENTER, BOTTOM_RIGHT };
					switch (property.Get<int>())
					{
					case TOP_LEFT: align = LengthPercentage(LengthPercentage::Percentage, 0.0f); break;
					case CENTER: align = LengthPercentage(LengthPercentage::Percentage, 50.0f); break;
					case BOTTOM_RIGHT: align = LengthPercentage(LengthPercentage::Percentage, 100.0f); break;
					}
				}
				else if (property.unit == Unit::PERCENT)
				{
					align = LengthPercentage(LengthPercentage::Percentage, property.Get<float>());
				}
				else if (property.unit == Unit::PX)
				{
					align = LengthPercentage(LengthPercentage::Length, property.Get<float>());
				}
				else
				{
					Log::Message(Log::LT_WARNING, "Decorator alignment value is '%s' which uses an unsupported unit (use px, %%, or keyword)",
						property.ToString().c_str());
				}
			}
		}

		if (ids.orientation != PropertyId::Invalid)
		{
			const Property& orientation_property = *properties.GetProperty(ids.orientation);
			tile.orientation = (DecoratorTiled::TileOrientation)orientation_property.value.Get<int>();
		}
	}

	return true;
}

} // namespace Rml
