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

	const RenderBox render_box = element->GetRenderBox(paint_area);
	const Vector2f offset = render_box.GetFillOffset();
	const Vector2f size = render_box.GetFillSize();

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
