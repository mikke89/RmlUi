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

#include "DecoratorTiledImage.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

namespace Rml {

DecoratorTiledImage::DecoratorTiledImage() {}

DecoratorTiledImage::~DecoratorTiledImage() {}

bool DecoratorTiledImage::Initialise(const Tile& _tile, Texture _texture)
{
	tile = _tile;
	tile.texture_index = AddTexture(_texture);
	return (tile.texture_index >= 0);
}

DecoratorDataHandle DecoratorTiledImage::GenerateElementData(Element* element, BoxArea paint_area) const
{
	// Calculate the tile's dimensions for this element.
	tile.CalculateDimensions(GetTexture());

	const ComputedValues& computed = element->GetComputedValues();

	const Vector2f offset = element->GetBox().GetPosition(paint_area);
	const Vector2f size = element->GetBox().GetSize(paint_area);

	// Generate the geometry for the tile.
	Mesh mesh;
	tile.GenerateGeometry(mesh, computed, offset, size, tile.GetNaturalDimensions(element));

	Geometry* data = new Geometry(element->GetRenderManager()->MakeGeometry(std::move(mesh)));

	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorTiledImage::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<Geometry*>(element_data);
}

void DecoratorTiledImage::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	Geometry* data = reinterpret_cast<Geometry*>(element_data);
	data->Render(element->GetAbsoluteOffset(BoxArea::Border), GetTexture());
}

DecoratorTiledImageInstancer::DecoratorTiledImageInstancer() : DecoratorTiledInstancer(1)
{
	RegisterTileProperty("image", true);
	RegisterShorthand("decorator", "image", ShorthandType::RecursiveRepeat);
}

DecoratorTiledImageInstancer::~DecoratorTiledImageInstancer() {}

SharedPtr<Decorator> DecoratorTiledImageInstancer::InstanceDecorator(const String& /*name*/, const PropertyDictionary& properties,
	const DecoratorInstancerInterface& instancer_interface)
{
	DecoratorTiled::Tile tile;
	Texture texture;

	if (!GetTileProperties(&tile, &texture, 1, properties, instancer_interface))
		return nullptr;

	auto decorator = MakeShared<DecoratorTiledImage>();

	if (!decorator->Initialise(tile, texture))
		return nullptr;

	return decorator;
}

} // namespace Rml
