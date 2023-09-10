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

#include "DecoratorTiledVertical.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/Texture.h"

namespace Rml {

struct DecoratorTiledVerticalData {
	DecoratorTiledVerticalData(int num_textures) : num_textures(num_textures) { geometry = new Geometry[num_textures]; }

	~DecoratorTiledVerticalData() { delete[] geometry; }

	const int num_textures;
	Geometry* geometry;
};

DecoratorTiledVertical::DecoratorTiledVertical() {}

DecoratorTiledVertical::~DecoratorTiledVertical() {}

bool DecoratorTiledVertical::Initialise(const Tile* _tiles, const Texture* _textures)
{
	// Load the textures.
	for (int i = 0; i < 3; i++)
	{
		tiles[i] = _tiles[i];
		tiles[i].texture_index = AddTexture(_textures[i]);
	}

	// If only one side of the decorator has been configured, then mirror the texture for the other side.
	if (tiles[TOP].texture_index == -1 && tiles[BOTTOM].texture_index > -1)
	{
		tiles[TOP] = tiles[BOTTOM];
		tiles[TOP].orientation = FLIP_HORIZONTAL;
	}
	else if (tiles[BOTTOM].texture_index == -1 && tiles[TOP].texture_index > -1)
	{
		tiles[BOTTOM] = tiles[TOP];
		tiles[BOTTOM].orientation = FLIP_HORIZONTAL;
	}
	else if (tiles[TOP].texture_index == -1 && tiles[BOTTOM].texture_index == -1)
		return false;

	if (tiles[CENTRE].texture_index == -1)
		return false;

	return true;
}

DecoratorDataHandle DecoratorTiledVertical::GenerateElementData(Element* element, BoxArea paint_area) const
{
	// Initialise the tile for this element.
	for (int i = 0; i < 3; i++)
		tiles[i].CalculateDimensions(GetTexture(tiles[i].texture_index));

	const Vector2f offset = element->GetBox().GetPosition(paint_area);
	const Vector2f size = element->GetBox().GetSize(paint_area);

	Vector2f top_dimensions = tiles[TOP].GetNaturalDimensions(element);
	Vector2f bottom_dimensions = tiles[BOTTOM].GetNaturalDimensions(element);
	Vector2f centre_dimensions = tiles[CENTRE].GetNaturalDimensions(element);

	// Scale the tile sizes by the width scale.
	ScaleTileDimensions(top_dimensions, size.x, Axis::Horizontal);
	ScaleTileDimensions(bottom_dimensions, size.x, Axis::Horizontal);
	ScaleTileDimensions(centre_dimensions, size.x, Axis::Horizontal);

	// Round the outer tile heights now so that we don't get gaps when rounding again in GenerateGeometry.
	top_dimensions.y = Math::Round(top_dimensions.y);
	bottom_dimensions.y = Math::Round(bottom_dimensions.y);

	// Shrink the y-sizes on the left and right tiles if necessary.
	if (size.y < top_dimensions.y + bottom_dimensions.y)
	{
		float minimum_height = top_dimensions.y + bottom_dimensions.y;
		top_dimensions.y = size.y * (top_dimensions.y / minimum_height);
		bottom_dimensions.y = size.y * (bottom_dimensions.y / minimum_height);
	}

	const ComputedValues& computed = element->GetComputedValues();
	Mesh mesh[COUNT];

	tiles[TOP].GenerateGeometry(mesh[tiles[TOP].texture_index], computed, offset, top_dimensions, top_dimensions);

	tiles[CENTRE].GenerateGeometry(mesh[tiles[CENTRE].texture_index], computed, offset + Vector2f(0, top_dimensions.y),
		Vector2f(centre_dimensions.x, size.y - (top_dimensions.y + bottom_dimensions.y)), centre_dimensions);

	tiles[BOTTOM].GenerateGeometry(mesh[tiles[BOTTOM].texture_index], computed, offset + Vector2f(0, size.y - bottom_dimensions.y), bottom_dimensions,
		bottom_dimensions);

	const int num_textures = GetNumTextures();
	DecoratorTiledVerticalData* data = new DecoratorTiledVerticalData(num_textures);

	// Set the mesh and textures on the geometry.
	RenderManager* render_manager = element->GetRenderManager();
	for (int i = 0; i < num_textures; i++)
		data->geometry[i] = render_manager->MakeGeometry(std::move(mesh[i]));

	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorTiledVertical::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<DecoratorTiledVerticalData*>(element_data);
}

void DecoratorTiledVertical::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	Vector2f translation = element->GetAbsoluteOffset(BoxArea::Padding).Round();
	DecoratorTiledVerticalData* data = reinterpret_cast<DecoratorTiledVerticalData*>(element_data);

	for (int i = 0; i < data->num_textures; i++)
		data->geometry[i].Render(translation, GetTexture(i));
}

DecoratorTiledVerticalInstancer::DecoratorTiledVerticalInstancer() : DecoratorTiledInstancer(3)
{
	RegisterTileProperty("top-image");
	RegisterTileProperty("bottom-image");
	RegisterTileProperty("center-image");
	RegisterShorthand("decorator", "top-image, center-image, bottom-image", ShorthandType::RecursiveCommaSeparated);
}

DecoratorTiledVerticalInstancer::~DecoratorTiledVerticalInstancer() {}

SharedPtr<Decorator> DecoratorTiledVerticalInstancer::InstanceDecorator(const String& /*name*/, const PropertyDictionary& properties,
	const DecoratorInstancerInterface& instancer_interface)
{
	constexpr size_t num_tiles = 3;

	DecoratorTiled::Tile tiles[num_tiles];
	Texture textures[num_tiles];

	if (!GetTileProperties(tiles, textures, num_tiles, properties, instancer_interface))
		return nullptr;

	auto decorator = MakeShared<DecoratorTiledVertical>();
	if (!decorator->Initialise(tiles, textures))
		return nullptr;

	return decorator;
}

} // namespace Rml
