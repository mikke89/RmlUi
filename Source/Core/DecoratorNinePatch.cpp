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

#include "DecoratorNinePatch.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {

DecoratorNinePatch::DecoratorNinePatch() {}

DecoratorNinePatch::~DecoratorNinePatch() {}

bool DecoratorNinePatch::Initialise(const Rectanglef& _rect_outer, const Rectanglef& _rect_inner, const Array<NumericValue, 4>* _edges,
	Texture _texture, float _display_scale)
{
	rect_outer = _rect_outer;
	rect_inner = _rect_inner;

	display_scale = _display_scale;

	if (_edges)
		edges = MakeUnique<Array<NumericValue, 4>>(*_edges);

	int texture_index = AddTexture(_texture);
	return (texture_index >= 0);
}

DecoratorDataHandle DecoratorNinePatch::GenerateElementData(Element* element, BoxArea paint_area) const
{
	const auto& computed = element->GetComputedValues();

	Texture texture = GetTexture();
	const Vector2f texture_dimensions(texture.GetDimensions());

	const Vector2f surface_offset = element->GetBox().GetPosition(paint_area);
	const Vector2f surface_dimensions = element->GetBox().GetSize(paint_area).Round();

	const ColourbPremultiplied quad_colour = computed.image_color().ToPremultiplied(computed.opacity());

	/* In the following, we operate on the four diagonal vertices in the grid, as they define the whole grid. */

	// Absolute texture coordinates 'px'
	Vector2f tex_pos[4] = {
		rect_outer.TopLeft(),
		rect_inner.TopLeft(),
		rect_inner.BottomRight(),
		rect_outer.BottomRight(),
	};

	// Normalized texture coordinates [0, 1]
	Vector2f tex_coords[4];
	for (int i = 0; i < 4; i++)
		tex_coords[i] = tex_pos[i] / texture_dimensions;

	// Natural size is determined from the raw pixel size multiplied by the dp-ratio and the sprite's
	// display scale (determined by eg. the inverse of spritesheet's 'src-scale').
	const float scale_raw_to_natural_dimensions = ElementUtilities::GetDensityIndependentPixelRatio(element) * display_scale;

	// Surface position in pixels [0, surface_dimensions]
	// Need to keep the corner patches at their natural size, but stretch the inner patches.
	Vector2f surface_pos[4];
	surface_pos[0] = {0, 0};
	surface_pos[1] = (tex_pos[1] - tex_pos[0]) * scale_raw_to_natural_dimensions;
	surface_pos[2] = surface_dimensions - (tex_pos[3] - tex_pos[2]) * scale_raw_to_natural_dimensions;
	surface_pos[3] = surface_dimensions;

	// Change the size of the edges if specified.
	if (edges)
	{
		float lengths[4]; // top, right, bottom, left
		lengths[0] = element->ResolveNumericValue((*edges)[0], (surface_pos[1].y - surface_pos[0].y));
		lengths[1] = element->ResolveNumericValue((*edges)[1], (surface_pos[3].x - surface_pos[2].x));
		lengths[2] = element->ResolveNumericValue((*edges)[2], (surface_pos[3].y - surface_pos[2].y));
		lengths[3] = element->ResolveNumericValue((*edges)[3], (surface_pos[1].x - surface_pos[0].x));

		surface_pos[1].y = lengths[0];
		surface_pos[2].x = surface_dimensions.x - lengths[1];
		surface_pos[2].y = surface_dimensions.y - lengths[2];
		surface_pos[1].x = lengths[3];
	}

	// In case the surface dimensions are less than the size of the corners, we need to scale down the corner rectangles, one dimension at a time.
	const Vector2f surface_center_size = surface_pos[2] - surface_pos[1];
	for (int i = 0; i < 2; i++)
	{
		if (surface_center_size[i] < 0.0f)
		{
			float top_left_size = tex_pos[1][i] - tex_pos[0][i];
			float bottom_right_size = tex_pos[3][i] - tex_pos[2][i];
			surface_pos[1][i] = top_left_size / (top_left_size + bottom_right_size) * surface_dimensions[i];
			surface_pos[2][i] = surface_pos[1][i];
		}
	}

	// Now offset all positions, relative to the border box.
	for (Vector2f& surface_pos_entry : surface_pos)
		surface_pos_entry += surface_offset;

	// Round the inner corners
	surface_pos[1] = surface_pos[1].Round();
	surface_pos[2] = surface_pos[2].Round();

	/* Now we have all the coordinates we need. Expand the diagonal vertices to the 16 individual vertices. */
	Mesh mesh;
	Vector<Vertex>& vertices = mesh.vertices;
	Vector<int>& indices = mesh.indices;

	vertices.resize(4 * 4);

	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			Vertex& vertex = vertices[y * 4 + x];
			vertex.colour = quad_colour;
			vertex.position = {surface_pos[x].x, surface_pos[y].y};
			vertex.tex_coord = {tex_coords[x].x, tex_coords[y].y};
		}
	}

	// Nine rectangles, two triangles per rectangle, three indices per triangle.
	indices.resize(9 * 2 * 3);

	// Fill in the indices one rectangle at a time.
	const int top_left_indices[9] = {0, 1, 2, 4, 5, 6, 8, 9, 10};
	for (int rectangle = 0; rectangle < 9; rectangle++)
	{
		int i = rectangle * 6;
		int top_left_index = top_left_indices[rectangle];
		indices[i] = top_left_index;
		indices[i + 1] = top_left_index + 4;
		indices[i + 2] = top_left_index + 1;
		indices[i + 3] = top_left_index + 1;
		indices[i + 4] = top_left_index + 4;
		indices[i + 5] = top_left_index + 5;
	}

	Geometry* data = new Geometry(element->GetRenderManager()->MakeGeometry(std::move(mesh)));

	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorNinePatch::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<Geometry*>(element_data);
}

void DecoratorNinePatch::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	Geometry* data = reinterpret_cast<Geometry*>(element_data);
	data->Render(element->GetAbsoluteOffset(BoxArea::Border), GetTexture());
}

DecoratorNinePatchInstancer::DecoratorNinePatchInstancer()
{
	sprite_outer_id = RegisterProperty("outer", "").AddParser("string").GetId();
	sprite_inner_id = RegisterProperty("inner", "").AddParser("string").GetId();
	edge_ids[0] = RegisterProperty("edge-top", "0px").AddParser("number_length_percent").GetId();
	edge_ids[1] = RegisterProperty("edge-right", "0px").AddParser("number_length_percent").GetId();
	edge_ids[2] = RegisterProperty("edge-bottom", "0px").AddParser("number_length_percent").GetId();
	edge_ids[3] = RegisterProperty("edge-left", "0px").AddParser("number_length_percent").GetId();

	RegisterShorthand("edge", "edge-top, edge-right, edge-bottom, edge-left", ShorthandType::Box);

	RMLUI_ASSERT(sprite_outer_id != PropertyId::Invalid && sprite_inner_id != PropertyId::Invalid);

	RegisterShorthand("decorator", "outer, inner, edge?", ShorthandType::RecursiveCommaSeparated);
}

DecoratorNinePatchInstancer::~DecoratorNinePatchInstancer() {}

SharedPtr<Decorator> DecoratorNinePatchInstancer::InstanceDecorator(const String& /*name*/, const PropertyDictionary& properties,
	const DecoratorInstancerInterface& instancer_interface)
{
	bool edges_set = false;
	Array<NumericValue, 4> edges;
	for (int i = 0; i < 4; i++)
	{
		edges[i] = properties.GetProperty(edge_ids[i])->GetNumericValue();
		if (edges[i].number != 0.0f)
		{
			edges_set = true;
		}
	}

	const Sprite* sprite_outer = nullptr;
	const Sprite* sprite_inner = nullptr;

	{
		const String sprite_name = properties.GetProperty(sprite_outer_id)->Get<String>();
		sprite_outer = instancer_interface.GetSprite(sprite_name);
		if (!sprite_outer)
		{
			Log::Message(Log::LT_WARNING, "Could not find sprite named '%s' in ninepatch decorator.", sprite_name.c_str());
			return nullptr;
		}
	}
	{
		const String sprite_name = properties.GetProperty(sprite_inner_id)->Get<String>();
		sprite_inner = instancer_interface.GetSprite(sprite_name);
		if (!sprite_inner)
		{
			Log::Message(Log::LT_WARNING, "Could not find sprite named '%s' in ninepatch decorator.", sprite_name.c_str());
			return nullptr;
		}
	}

	if (sprite_outer->sprite_sheet != sprite_inner->sprite_sheet)
	{
		Log::Message(Log::LT_WARNING, "The outer and inner sprites in a ninepatch decorator must be from the same sprite sheet.");
		return nullptr;
	}

	auto decorator = MakeShared<DecoratorNinePatch>();

	if (!decorator->Initialise(sprite_outer->rectangle, sprite_inner->rectangle, (edges_set ? &edges : nullptr),
			sprite_outer->sprite_sheet->texture_source.GetTexture(instancer_interface.GetRenderManager()), sprite_outer->sprite_sheet->display_scale))
	{
		return nullptr;
	}

	return decorator;
}

} // namespace Rml
