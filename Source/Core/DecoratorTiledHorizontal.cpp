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

#include "DecoratorTiledHorizontal.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Texture.h"

namespace Rml {

struct DecoratorTiledHorizontalData
{
	DecoratorTiledHorizontalData(Element* host_element, int num_textures) : num_textures(num_textures)
	{
		geometry = new Geometry[num_textures];
		for (int i = 0; i < num_textures; i++)
			geometry[i].SetHostElement(host_element);
	}

	~DecoratorTiledHorizontalData()
	{
		delete[] geometry;
	}

	const int num_textures;
	Geometry* geometry;
};

DecoratorTiledHorizontal::DecoratorTiledHorizontal()
{
}

DecoratorTiledHorizontal::~DecoratorTiledHorizontal()
{
}

// Initialises the tiles for the decorator.
bool DecoratorTiledHorizontal::Initialise(const Tile* _tiles, const Texture* _textures)
{
	// Load the textures.
	for (int i = 0; i < 3; i++)
	{
		tiles[i] = _tiles[i];
		tiles[i].texture_index = AddTexture(_textures[i]);
	}

	// If only one side of the decorator has been configured, then mirror the texture for the other side.
	if (tiles[LEFT].texture_index == -1 && tiles[RIGHT].texture_index > -1)
	{
		tiles[LEFT] = tiles[RIGHT];
		tiles[LEFT].orientation = FLIP_HORIZONTAL;
	}
	else if (tiles[RIGHT].texture_index == -1 && tiles[LEFT].texture_index > -1)
	{
		tiles[RIGHT] = tiles[LEFT];
		tiles[RIGHT].orientation = FLIP_HORIZONTAL;
	}
	else if (tiles[LEFT].texture_index == -1 && tiles[RIGHT].texture_index == -1)
		return false;

	if (tiles[CENTRE].texture_index == -1)
		return false;

	return true;
}

// Called on a decorator to generate any required per-element data for a newly decorated element.
DecoratorDataHandle DecoratorTiledHorizontal::GenerateElementData(Element* element) const
{
	// Initialise the tiles for this element.
	for (int i = 0; i < 3; i++)
		tiles[i].CalculateDimensions(element, *(GetTexture(tiles[i].texture_index)));

	const int num_textures = GetNumTextures();
	DecoratorTiledHorizontalData* data = new DecoratorTiledHorizontalData(element, num_textures);

	Vector2f padded_size = element->GetBox().GetSize(Box::PADDING);

	Vector2f left_dimensions = tiles[LEFT].GetNaturalDimensions(element);
	Vector2f right_dimensions = tiles[RIGHT].GetNaturalDimensions(element);
	Vector2f centre_dimensions = tiles[CENTRE].GetNaturalDimensions(element);

	// Scale the tile sizes by the height scale.
	ScaleTileDimensions(left_dimensions, padded_size.y, Axis::Vertical);
	ScaleTileDimensions(right_dimensions, padded_size.y, Axis::Vertical);
	ScaleTileDimensions(centre_dimensions, padded_size.y, Axis::Vertical);

	// Round the outer tile widths now so that we don't get gaps when rounding again in GenerateGeometry.
	left_dimensions.x = Math::RoundFloat(left_dimensions.x);
	right_dimensions.x = Math::RoundFloat(right_dimensions.x);

	// Shrink the x-sizes on the left and right tiles if necessary.
	if (padded_size.x < left_dimensions.x + right_dimensions.x)
	{
		float minimum_width = left_dimensions.x + right_dimensions.x;
		left_dimensions.x = padded_size.x * (left_dimensions.x / minimum_width);
		right_dimensions.x = padded_size.x * (right_dimensions.x / minimum_width);
	}

	// Generate the geometry for the left tile.
	tiles[LEFT].GenerateGeometry(data->geometry[tiles[LEFT].texture_index].GetVertices(), data->geometry[tiles[LEFT].texture_index].GetIndices(), element, Vector2f(0, 0), left_dimensions, left_dimensions);
	// Generate the geometry for the centre tiles.
	tiles[CENTRE].GenerateGeometry(data->geometry[tiles[CENTRE].texture_index].GetVertices(), data->geometry[tiles[CENTRE].texture_index].GetIndices(), element, Vector2f(left_dimensions.x, 0), Vector2f(padded_size.x - (left_dimensions.x + right_dimensions.x), centre_dimensions.y), centre_dimensions);
	// Generate the geometry for the right tile.
	tiles[RIGHT].GenerateGeometry(data->geometry[tiles[RIGHT].texture_index].GetVertices(), data->geometry[tiles[RIGHT].texture_index].GetIndices(), element, Vector2f(padded_size.x - right_dimensions.x, 0), right_dimensions, right_dimensions);

	// Set the textures on the geometry.
	const Texture* texture = nullptr;
	int texture_index = 0;
	while ((texture = GetTexture(texture_index)) != nullptr)
		data->geometry[texture_index++].SetTexture(texture);

	return reinterpret_cast<DecoratorDataHandle>(data);
}

// Called to release element data generated by this decorator.
void DecoratorTiledHorizontal::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast< DecoratorTiledHorizontalData* >(element_data);
}

// Called to render the decorator on an element.
void DecoratorTiledHorizontal::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	Vector2f translation = element->GetAbsoluteOffset(Box::PADDING).Round();
	DecoratorTiledHorizontalData* data = reinterpret_cast< DecoratorTiledHorizontalData* >(element_data);

	for (int i = 0; i < data->num_textures; i++)
		data->geometry[i].Render(translation);
}

} // namespace Rml
