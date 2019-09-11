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

#include "precompiled.h"
#include "DecoratorNinePatch.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"

namespace Rml {
namespace Core {

DecoratorNinePatch::DecoratorNinePatch()
{
}

DecoratorNinePatch::~DecoratorNinePatch()
{
}

bool DecoratorNinePatch::Initialise(const Rectangle& _rect_outer, const Rectangle& _rect_inner, const Texture& _texture)
{
	rect_outer = _rect_outer;
	rect_inner = _rect_inner;
	int texture_index = AddTexture(_texture);
	return (texture_index >= 0);
}

DecoratorDataHandle DecoratorNinePatch::GenerateElementData(Element* element) const
{
	RenderInterface* render_interface = element->GetRenderInterface();
	const auto& computed = element->GetComputedValues();

	Geometry* data = new Geometry(element);

	const Texture* texture = GetTexture();
	data->SetTexture(texture);
	const Vector2i texture_dimensions_i = texture->GetDimensions(render_interface);
	const Vector2f texture_dimensions = { (float)texture_dimensions_i.x, (float)texture_dimensions_i.y };

	const Vector2f surface_dimensions = element->GetBox().GetSize(Box::PADDING);

	const float opacity = computed.opacity;
	Colourb quad_colour = computed.image_color;

	quad_colour.alpha = (byte)(opacity * (float)quad_colour.alpha);


	/* In the following, we operate on the four diagonal vertices in the grid, as they define the whole grid. */

	// Absolute texture coordinates 'px'
	Vector2f tex_pos[4];
	tex_pos[0] = { rect_outer.x,                    rect_outer.y };
	tex_pos[1] = { rect_inner.x,                    rect_inner.y };
	tex_pos[2] = { rect_inner.x + rect_inner.width, rect_inner.y + rect_inner.height };
	tex_pos[3] = { rect_outer.x + rect_outer.width, rect_outer.y + rect_outer.height };

	// Normalized texture coordinates [0, 1]
	Vector2f tex_coords[4];
	for (int i = 0; i < 4; i++)
		tex_coords[i] = tex_pos[i] / texture_dimensions;

	// Surface position [0, surface_dimensions]
	// Need to keep the four corner patches at their native pixel size, but stretch the inner patches.
	Vector2f surface_pos[4];
	surface_pos[0] = { 0, 0 };
	surface_pos[1] = tex_pos[1] - tex_pos[0];
	surface_pos[2] = surface_dimensions - (tex_pos[3] - tex_pos[2]);
	surface_pos[3] = surface_dimensions;

	// In case the surface dimensions are less than the size of the edges, we need to scale down the corner rectangles, one dimension at a time.
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

	/* Now we have all the coordinates we need. Expand the diagonal vertices to the 16 individual vertices. */

	std::vector<Vertex>& vertices = data->GetVertices();
	std::vector<int>& indices = data->GetIndices();

	vertices.resize(4 * 4);

	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			Vertex& vertex = vertices[y * 4 + x];
			vertex.colour = quad_colour;
			vertex.position = { surface_pos[x].x, surface_pos[y].y };
			vertex.tex_coord = { tex_coords[x].x, tex_coords[y].y };
		}
	}

	// Nine rectangles, two triangles per rectangle, three indices per triangle.
	indices.resize(9 * 2 * 3);

	// Fill in the indices one rectangle at a time.
	const int top_left_indices[9] = { 0, 1, 2, 4, 5, 6, 8, 9, 10 };
	for (int rectangle = 0; rectangle < 9; rectangle++)
	{
		int i = rectangle * 6;
		int top_left_index = top_left_indices[rectangle];
		indices[i]     = top_left_index;
		indices[i + 1] = top_left_index + 4;
		indices[i + 2] = top_left_index + 1;
		indices[i + 3] = top_left_index + 1;
		indices[i + 4] = top_left_index + 4;
		indices[i + 5] = top_left_index + 5;
	}

	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorNinePatch::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast< Geometry* >(element_data);
}

void DecoratorNinePatch::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	Geometry* data = reinterpret_cast< Geometry* >(element_data);
	data->Render(element->GetAbsoluteOffset(Box::PADDING).Round());
}




DecoratorNinePatchInstancer::DecoratorNinePatchInstancer()
{
	sprite_outer_id = RegisterProperty("outer", "").AddParser("string").GetId();
	sprite_inner_id = RegisterProperty("inner", "").AddParser("string").GetId();
	
	RMLUI_ASSERT(sprite_outer_id != PropertyId::Invalid && sprite_inner_id != PropertyId::Invalid);

	RegisterShorthand("decorator", "outer, inner", ShorthandType::RecursiveCommaSeparated);
}

DecoratorNinePatchInstancer::~DecoratorNinePatchInstancer()
{
}


SharedPtr<Decorator> DecoratorNinePatchInstancer::InstanceDecorator(const String& RMLUI_UNUSED_PARAMETER(name), const PropertyDictionary& properties, const DecoratorInstancerInterface& interface)
{
	RMLUI_UNUSED(name);

	const Sprite* sprite_outer = nullptr;
	const Sprite* sprite_inner = nullptr;

	{
		const Property* property = properties.GetProperty(sprite_outer_id);
		const String sprite_name = property->Get< String >();
		sprite_outer = interface.GetSprite(sprite_name);
		if (!sprite_outer)
		{
			Log::Message(Log::LT_WARNING, "Could not find sprite named '%s' in ninepatch decorator.", sprite_name.c_str());
			return nullptr;
		}
	}
	{
		const Property* property = properties.GetProperty(sprite_inner_id);
		const String sprite_name = property->Get< String >();
		sprite_inner = interface.GetSprite(sprite_name);
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

	auto decorator = std::make_shared<DecoratorNinePatch>();

	if (!decorator->Initialise(sprite_outer->rectangle, sprite_inner->rectangle, sprite_outer->sprite_sheet->texture))
		return nullptr;

	return decorator;
}




}
}
