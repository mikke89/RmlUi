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

#include "DecoratorTiledBox.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

namespace Rml {

struct DecoratorTiledBoxData {
	DecoratorTiledBoxData(int num_textures) : num_textures(num_textures) { geometry = new Geometry[num_textures]; }

	~DecoratorTiledBoxData() { delete[] geometry; }

	const int num_textures;
	Geometry* geometry;
};

DecoratorTiledBox::DecoratorTiledBox() {}

DecoratorTiledBox::~DecoratorTiledBox() {}

bool DecoratorTiledBox::Initialise(const Tile* _tiles, const Texture* _textures)
{
	// Load the textures.
	for (int i = 0; i < 9; i++)
	{
		tiles[i] = _tiles[i];
		tiles[i].texture_index = AddTexture(_textures[i]);
	}

	// All corners must have a valid texture.
	for (int i = TOP_LEFT_CORNER; i <= BOTTOM_RIGHT_CORNER; i++)
	{
		if (tiles[i].texture_index == -1)
			return false;
	}
	// Check that the centre tile has been specified.
	if (tiles[CENTRE].texture_index < 0)
		return false;

	// If only one side of the left / right edges have been configured, then mirror the tile for the other side.
	if (tiles[LEFT_EDGE].texture_index == -1 && tiles[RIGHT_EDGE].texture_index > -1)
	{
		tiles[LEFT_EDGE] = tiles[RIGHT_EDGE];
		tiles[LEFT_EDGE].orientation = FLIP_HORIZONTAL;
	}
	else if (tiles[RIGHT_EDGE].texture_index == -1 && tiles[LEFT_EDGE].texture_index > -1)
	{
		tiles[RIGHT_EDGE] = tiles[LEFT_EDGE];
		tiles[RIGHT_EDGE].orientation = FLIP_HORIZONTAL;
	}
	else if (tiles[LEFT_EDGE].texture_index == -1 && tiles[RIGHT_EDGE].texture_index == -1)
		return false;

	// If only one side of the top / bottom edges have been configured, then mirror the tile for the other side.
	if (tiles[TOP_EDGE].texture_index == -1 && tiles[BOTTOM_EDGE].texture_index > -1)
	{
		tiles[TOP_EDGE] = tiles[BOTTOM_EDGE];
		tiles[TOP_EDGE].orientation = FLIP_VERTICAL;
	}
	else if (tiles[BOTTOM_EDGE].texture_index == -1 && tiles[TOP_EDGE].texture_index > -1)
	{
		tiles[BOTTOM_EDGE] = tiles[TOP_EDGE];
		tiles[BOTTOM_EDGE].orientation = FLIP_VERTICAL;
	}
	else if (tiles[TOP_EDGE].texture_index == -1 && tiles[BOTTOM_EDGE].texture_index == -1)
		return false;

	return true;
}

DecoratorDataHandle DecoratorTiledBox::GenerateElementData(Element* element, BoxArea paint_area) const
{
	// Initialise the tiles for this element.
	for (int i = 0; i < 9; i++)
	{
		RMLUI_ASSERT(tiles[i].texture_index >= 0);
		tiles[i].CalculateDimensions(GetTexture(tiles[i].texture_index));
	}

	const Vector2f offset = element->GetBox().GetPosition(paint_area);
	const Vector2f size = element->GetBox().GetSize(paint_area);

	// Calculate the natural dimensions of tile corners and edges.
	const Vector2f natural_top_left = tiles[TOP_LEFT_CORNER].GetNaturalDimensions(element);
	const Vector2f natural_top = tiles[TOP_EDGE].GetNaturalDimensions(element);
	const Vector2f natural_top_right = tiles[TOP_RIGHT_CORNER].GetNaturalDimensions(element);

	const Vector2f natural_bottom_left = tiles[BOTTOM_LEFT_CORNER].GetNaturalDimensions(element);
	const Vector2f natural_bottom = tiles[BOTTOM_EDGE].GetNaturalDimensions(element);
	const Vector2f natural_bottom_right = tiles[BOTTOM_RIGHT_CORNER].GetNaturalDimensions(element);

	const Vector2f natural_left = tiles[LEFT_EDGE].GetNaturalDimensions(element);
	const Vector2f natural_right = tiles[RIGHT_EDGE].GetNaturalDimensions(element);

	// Initialize the to-be-determined dimensions of the tiles.
	Vector2f top_left = natural_top_left;
	Vector2f top = natural_top;
	Vector2f top_right = natural_top_right;

	Vector2f bottom_left = natural_bottom_left;
	Vector2f bottom = natural_bottom;
	Vector2f bottom_right = natural_bottom_right;

	Vector2f left = natural_left;
	Vector2f right = natural_right;

	// Scale the top corners down if appropriate. If they are scaled, then the left and right edges are also scaled
	// if they shared a width with their corner. Best solution? Don't know.
	if (size.x < top_left.x + top_right.x)
	{
		float minimum_width = top_left.x + top_right.x;

		top_left.x = size.x * (top_left.x / minimum_width);
		if (natural_top_left.x == natural_left.x)
			left.x = top_left.x;

		top_right.x = size.x * (top_right.x / minimum_width);
		if (natural_top_right.x == natural_right.x)
			right.x = top_right.x;
	}

	// Scale the bottom corners down if appropriate. If they are scaled, then the left and right edges are also scaled
	// if they shared a width with their corner. Best solution? Don't know.
	if (size.x < bottom_left.x + bottom_right.x)
	{
		float minimum_width = bottom_left.x + bottom_right.x;

		bottom_left.x = size.x * (bottom_left.x / minimum_width);
		if (natural_bottom_left.x == natural_left.x)
			left.x = bottom_left.x;

		bottom_right.x = size.x * (bottom_right.x / minimum_width);
		if (natural_bottom_right.x == natural_right.x)
			right.x = bottom_right.x;
	}

	// Scale the left corners down if appropriate. If they are scaled, then the top and bottom edges are also scaled
	// if they shared a width with their corner. Best solution? Don't know.
	if (size.y < top_left.y + bottom_left.y)
	{
		float minimum_height = top_left.y + bottom_left.y;

		top_left.y = size.y * (top_left.y / minimum_height);
		if (natural_top_left.y == natural_top.y)
			top.y = top_left.y;

		bottom_left.y = size.y * (bottom_left.y / minimum_height);
		if (natural_bottom_left.y == natural_bottom.y)
			bottom.y = bottom_left.y;
	}

	// Scale the right corners down if appropriate. If they are scaled, then the top and bottom edges are also scaled
	// if they shared a width with their corner. Best solution? Don't know.
	if (size.y < top_right.y + bottom_right.y)
	{
		float minimum_height = top_right.y + bottom_right.y;

		top_right.y = size.y * (top_right.y / minimum_height);
		if (natural_top_right.y == natural_top.y)
			top.y = top_right.y;

		bottom_right.y = size.y * (bottom_right.y / minimum_height);
		if (natural_bottom_right.y == natural_bottom.y)
			bottom.y = bottom_right.y;
	}

	const ComputedValues& computed = element->GetComputedValues();
	Mesh mesh[COUNT];

	// Generate the geometry for the top-left tile.
	tiles[TOP_LEFT_CORNER].GenerateGeometry(mesh[tiles[TOP_LEFT_CORNER].texture_index], computed, offset, top_left, top_left);
	// Generate the geometry for the top edge tiles.
	tiles[TOP_EDGE].GenerateGeometry(mesh[tiles[TOP_EDGE].texture_index], computed, offset + Vector2f(top_left.x, 0),
		Vector2f(size.x - (top_left.x + top_right.x), top.y), top);
	// Generate the geometry for the top-right tile.
	tiles[TOP_RIGHT_CORNER].GenerateGeometry(mesh[tiles[TOP_RIGHT_CORNER].texture_index], computed, offset + Vector2f(size.x - top_right.x, 0),
		top_right, top_right);

	// Generate the geometry for the left side.
	tiles[LEFT_EDGE].GenerateGeometry(mesh[tiles[LEFT_EDGE].texture_index], computed, offset + Vector2f(0, top_left.y),
		Vector2f(left.x, size.y - (top_left.y + bottom_left.y)), left);

	// Generate the geometry for the right side.
	tiles[RIGHT_EDGE].GenerateGeometry(mesh[tiles[RIGHT_EDGE].texture_index], computed, offset + Vector2f((size.x - right.x), top_right.y),
		Vector2f(right.x, size.y - (top_right.y + bottom_right.y)), right);

	// Generate the geometry for the bottom-left tile.
	tiles[BOTTOM_LEFT_CORNER].GenerateGeometry(mesh[tiles[BOTTOM_LEFT_CORNER].texture_index], computed, offset + Vector2f(0, size.y - bottom_left.y),
		bottom_left, bottom_left);
	// Generate the geometry for the bottom edge tiles.
	tiles[BOTTOM_EDGE].GenerateGeometry(mesh[tiles[BOTTOM_EDGE].texture_index], computed, offset + Vector2f(bottom_left.x, size.y - bottom.y),
		Vector2f(size.x - (bottom_left.x + bottom_right.x), bottom.y), bottom);
	// Generate the geometry for the bottom-right tile.
	tiles[BOTTOM_RIGHT_CORNER].GenerateGeometry(mesh[tiles[BOTTOM_RIGHT_CORNER].texture_index], computed,
		offset + Vector2f(size.x - bottom_right.x, size.y - bottom_right.y), bottom_right, bottom_right);

	// Generate the centre geometry.
	Vector2f centre_dimensions = tiles[CENTRE].GetNaturalDimensions(element);
	Vector2f centre_surface_dimensions(size.x - (left.x + right.x), size.y - (top.y + bottom.y));

	tiles[CENTRE].GenerateGeometry(mesh[tiles[CENTRE].texture_index], computed, offset + Vector2f(left.x, top.y), centre_surface_dimensions,
		centre_dimensions);

	const int num_textures = GetNumTextures();
	DecoratorTiledBoxData* data = new DecoratorTiledBoxData(num_textures);
	RenderManager* render_manager = element->GetRenderManager();

	// Set the mesh and textures on the geometry.
	for (int i = 0; i < num_textures; i++)
		data->geometry[i] = render_manager->MakeGeometry(std::move(mesh[i]));

	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorTiledBox::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<DecoratorTiledBoxData*>(element_data);
}

void DecoratorTiledBox::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	Vector2f translation = element->GetAbsoluteOffset(BoxArea::Border);
	DecoratorTiledBoxData* data = reinterpret_cast<DecoratorTiledBoxData*>(element_data);

	for (int i = 0; i < data->num_textures; i++)
		data->geometry[i].Render(translation, GetTexture(i));
}

DecoratorTiledBoxInstancer::DecoratorTiledBoxInstancer() : DecoratorTiledInstancer(9)
{
	RegisterTileProperty("top-left-image");
	RegisterTileProperty("top-right-image");
	RegisterTileProperty("bottom-left-image");
	RegisterTileProperty("bottom-right-image");

	RegisterTileProperty("left-image");
	RegisterTileProperty("right-image");
	RegisterTileProperty("top-image");
	RegisterTileProperty("bottom-image");

	RegisterTileProperty("center-image");

	RegisterShorthand("decorator",
		"top-left-image, top-image, top-right-image, left-image, center-image, right-image, bottom-left-image, bottom-image, bottom-right-image",
		ShorthandType::RecursiveCommaSeparated);
}

DecoratorTiledBoxInstancer::~DecoratorTiledBoxInstancer() {}

SharedPtr<Decorator> DecoratorTiledBoxInstancer::InstanceDecorator(const String& /*name*/, const PropertyDictionary& properties,
	const DecoratorInstancerInterface& instancer_interface)
{
	constexpr size_t num_tiles = 9;

	DecoratorTiled::Tile tiles[num_tiles];
	Texture textures[num_tiles];

	if (!GetTileProperties(tiles, textures, num_tiles, properties, instancer_interface))
		return nullptr;

	auto decorator = MakeShared<DecoratorTiledBox>();
	if (!decorator->Initialise(tiles, textures))
		return nullptr;

	return decorator;
}

} // namespace Rml
