#include "DecoratorTiledHorizontal.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/Texture.h"

namespace Rml {

struct DecoratorTiledHorizontalData {
	DecoratorTiledHorizontalData(int num_textures) : num_textures(num_textures) { geometry = new Geometry[num_textures]; }

	~DecoratorTiledHorizontalData() { delete[] geometry; }

	const int num_textures;
	Geometry* geometry;
};

DecoratorTiledHorizontal::DecoratorTiledHorizontal() {}

DecoratorTiledHorizontal::~DecoratorTiledHorizontal() {}

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

DecoratorDataHandle DecoratorTiledHorizontal::GenerateElementData(Element* element, BoxArea paint_area) const
{
	// Initialise the tiles for this element.
	for (int i = 0; i < 3; i++)
		tiles[i].CalculateDimensions(GetTexture(tiles[i].texture_index));

	const RenderBox render_box = element->GetRenderBox(paint_area);
	const Vector2f offset = render_box.GetFillOffset();
	const Vector2f size = render_box.GetFillSize();

	Vector2f left_dimensions = tiles[LEFT].GetNaturalDimensions(element);
	Vector2f right_dimensions = tiles[RIGHT].GetNaturalDimensions(element);
	Vector2f centre_dimensions = tiles[CENTRE].GetNaturalDimensions(element);

	// Scale the tile sizes by the height scale.
	ScaleTileDimensions(left_dimensions, size.y, Axis::Vertical);
	ScaleTileDimensions(right_dimensions, size.y, Axis::Vertical);
	ScaleTileDimensions(centre_dimensions, size.y, Axis::Vertical);

	// Round the outer tile widths now so that we don't get gaps when rounding again in GenerateGeometry.
	left_dimensions.x = Math::Round(left_dimensions.x);
	right_dimensions.x = Math::Round(right_dimensions.x);

	// Shrink the x-sizes on the left and right tiles if necessary.
	if (size.x < left_dimensions.x + right_dimensions.x)
	{
		float minimum_width = left_dimensions.x + right_dimensions.x;
		left_dimensions.x = size.x * (left_dimensions.x / minimum_width);
		right_dimensions.x = size.x * (right_dimensions.x / minimum_width);
	}

	const ComputedValues& computed = element->GetComputedValues();
	Mesh mesh[COUNT];

	tiles[LEFT].GenerateGeometry(mesh[tiles[LEFT].texture_index], computed, offset, left_dimensions, left_dimensions);

	tiles[CENTRE].GenerateGeometry(mesh[tiles[CENTRE].texture_index], computed, offset + Vector2f(left_dimensions.x, 0),
		Vector2f(size.x - (left_dimensions.x + right_dimensions.x), centre_dimensions.y), centre_dimensions);

	tiles[RIGHT].GenerateGeometry(mesh[tiles[RIGHT].texture_index], computed, offset + Vector2f(size.x - right_dimensions.x, 0), right_dimensions,
		right_dimensions);

	const int num_textures = GetNumTextures();
	DecoratorTiledHorizontalData* data = new DecoratorTiledHorizontalData(num_textures);
	RenderManager* render_manager = element->GetRenderManager();

	// Set the mesh and textures on the geometry.
	for (int i = 0; i < num_textures; i++)
		data->geometry[i] = render_manager->MakeGeometry(std::move(mesh[i]));

	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorTiledHorizontal::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<DecoratorTiledHorizontalData*>(element_data);
}

void DecoratorTiledHorizontal::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	Vector2f translation = element->GetAbsoluteOffset(BoxArea::Border);
	DecoratorTiledHorizontalData* data = reinterpret_cast<DecoratorTiledHorizontalData*>(element_data);

	for (int i = 0; i < data->num_textures; i++)
		data->geometry[i].Render(translation, GetTexture(i));
}

DecoratorTiledHorizontalInstancer::DecoratorTiledHorizontalInstancer() : DecoratorTiledInstancer(3)
{
	RegisterTileProperty("left-image");
	RegisterTileProperty("right-image");
	RegisterTileProperty("center-image");
	RegisterShorthand("decorator", "left-image, center-image, right-image", ShorthandType::RecursiveCommaSeparated);
}

DecoratorTiledHorizontalInstancer::~DecoratorTiledHorizontalInstancer() {}

SharedPtr<Decorator> DecoratorTiledHorizontalInstancer::InstanceDecorator(const String& /*name*/, const PropertyDictionary& properties,
	const DecoratorInstancerInterface& instancer_interface)
{
	constexpr size_t num_tiles = 3;

	DecoratorTiled::Tile tiles[num_tiles];
	Texture textures[num_tiles];

	if (!GetTileProperties(tiles, textures, num_tiles, properties, instancer_interface))
		return nullptr;

	auto decorator = MakeShared<DecoratorTiledHorizontal>();
	if (!decorator->Initialise(tiles, textures))
		return nullptr;

	return decorator;
}
} // namespace Rml
