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

#include "../../Include/RmlUi/Controls/ElementProgressBar.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/URL.h"
#include <algorithm>

namespace Rml {
namespace Controls {

ElementProgressBar::ElementProgressBar(const Core::String& tag) : Element(tag), direction(DefaultDirection), start_edge(DefaultStartEdge), value(0), fill(nullptr), rect_set(false), geometry(this)
{
	geometry_dirty = false;
	texture_dirty = true;

	// Add the fill element as a non-DOM element.
	Core::ElementPtr fill_element = Core::Factory::InstanceElement(this, "*", "fill", Core::XMLAttributes());
	RMLUI_ASSERT(fill_element);
	fill = AppendChild(std::move(fill_element), false);
}

ElementProgressBar::~ElementProgressBar()
{
}

float ElementProgressBar::GetValue() const
{
	return value;
}

void ElementProgressBar::SetValue(float in_value)
{
	SetAttribute("value", in_value);
}

void ElementProgressBar::OnRender()
{
	// Some properties may change geometry without dirtying the layout, eg. opacity.
	if (geometry_dirty)
		GenerateGeometry();

	// Render the geometry at the fill element's content region.
	geometry.Render(fill->GetAbsoluteOffset().Round());
}

void ElementProgressBar::OnAttributeChange(const Rml::Core::ElementAttributes& changed_attributes)
{
	Rml::Core::Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("value") != changed_attributes.end())
	{
		value = Core::Math::Clamp( GetAttribute< float >("value", 0.0f), 0.0f, 1.0f);
		geometry_dirty = true;
	}

	if (changed_attributes.find("direction") != changed_attributes.end())
	{
		using DirectionNameList = std::array<Core::String, size_t(Direction::Count)>;
		static const DirectionNameList names = { "top", "right", "bottom", "left", "clockwise", "counter-clockwise" };

		direction = DefaultDirection;

		Core::String name = Core::StringUtilities::ToLower( GetAttribute< Core::String >("direction", "") );
		auto it = std::find(names.begin(), names.end(), name);

		size_t index = size_t(it - names.begin());
		if (index < size_t(Direction::Count))
			direction = Direction(index);

		geometry_dirty = true;
	}

	if (changed_attributes.find("start-edge") != changed_attributes.end())
	{
		using StartEdgeNameList = std::array<Core::String, size_t(StartEdge::Count)>;
		static const StartEdgeNameList names = { "top", "right", "bottom", "left" };

		start_edge = DefaultStartEdge;

		Core::String name = Core::StringUtilities::ToLower(GetAttribute< Core::String >("start-edge", ""));
		auto it = std::find(names.begin(), names.end(), name);

		size_t index = size_t(it - names.begin());
		if (index < size_t(StartEdge::Count))
			start_edge = StartEdge(index);

		geometry_dirty = true;
	}
}

void ElementProgressBar::OnPropertyChange(const Core::PropertyIdSet& changed_properties)
{
    Element::OnPropertyChange(changed_properties);

    if (changed_properties.Contains(Core::PropertyId::ImageColor) ||
        changed_properties.Contains(Core::PropertyId::Opacity)) {
		geometry_dirty = true;
    }

	if (changed_properties.Contains(Core::PropertyId::FillImage)) {
		texture_dirty = true;
	}
}

void ElementProgressBar::OnResize()
{
	using Core::Box;
	using Core::Vector2f;

	const Vector2f element_size = GetBox().GetSize();

	// Build and set the 'fill' element's box. Here we are mainly interested in all the edge sizes set by the user.
	// The content size of the box is here scaled to fit inside the progress bar. Then, during 'CreateGeometry()',
	// the 'fill' element's content size is further shrunk according to 'value' along the proper direction.
	Box fill_box;

	Core::ElementUtilities::BuildBox(fill_box, element_size, fill);
	
	const Vector2f margin_top_left(
		fill_box.GetEdge(Box::MARGIN, Box::LEFT),
		fill_box.GetEdge(Box::MARGIN, Box::TOP)
	);
	const Vector2f edge_size = fill_box.GetSize(Box::MARGIN) - fill_box.GetSize(Box::CONTENT);

	fill_offset = GetBox().GetPosition() + margin_top_left;
	fill_size = element_size - edge_size;

	fill_box.SetContent(fill_size);
	fill->SetBox(fill_box);

	geometry_dirty = true;
}

void ElementProgressBar::GenerateGeometry()
{
	using Core::Vector2f;

	Vector2f render_size = fill_size;

	{
		// Size and offset the fill element depending on the progressbar value.
		Vector2f offset = fill_offset;

		switch (direction) {
		case Direction::Top:
			render_size.y = fill_size.y * value;
			offset.y = fill_offset.y + fill_size.y - render_size.y;
			break;
		case Direction::Right:
			render_size.x = fill_size.x * value;
			break;
		case Direction::Bottom:
			render_size.y = fill_size.y * value;
			break;
		case Direction::Left:
			render_size.x = fill_size.x * value;
			offset.x = fill_offset.x + fill_size.x - render_size.x;
			break;
		case Direction::Clockwise:
		case Direction::CounterClockwise:
			// Circular progress bars cannot use a box to shape the fill element, instead we need to manually create the geometry from the image texture.
			// Thus, we leave the size and offset untouched as a canvas for the manual geometry.
			break;

			RMLUI_UNUSED_SWITCH_ENUM(Direction::Count);
		}

		Core::Box fill_box = fill->GetBox();
		fill_box.SetContent(render_size);
		fill->SetBox(fill_box);
		fill->SetOffset(offset, this);
	}

	if (texture_dirty)
		LoadTexture();

	geometry.Release(true);
	geometry_dirty = false;

	// If we don't have a fill texture, then there is no need to generate manual geometry, and we are done here.
	// Instead, users can style the fill element eg. by decorators.
	if (!texture)
		return;

	// Otherwise, the 'fill-image' property is set, let's generate its geometry.
	auto& vertices = geometry.GetVertices();
	auto& indices = geometry.GetIndices();

	Vector2f texcoords[2];
	if (rect_set)
	{
		Vector2f texture_dimensions((float)texture.GetDimensions(GetRenderInterface()).x, (float)texture.GetDimensions(GetRenderInterface()).y);
		if (texture_dimensions.x == 0)
			texture_dimensions.x = 1;
		if (texture_dimensions.y == 0)
			texture_dimensions.y = 1;

		texcoords[0].x = rect.x / texture_dimensions.x;
		texcoords[0].y = rect.y / texture_dimensions.y;

		texcoords[1].x = (rect.x + rect.width) / texture_dimensions.x;
		texcoords[1].y = (rect.y + rect.height) / texture_dimensions.y;
	}
	else
	{
		texcoords[0] = Vector2f(0, 0);
		texcoords[1] = Vector2f(1, 1);
	}

	Core::Colourb quad_colour;
	{
		const Core::ComputedValues& computed = GetComputedValues();
		const float opacity = computed.opacity;
		quad_colour = computed.image_color;
		quad_colour.alpha = (Core::byte)(opacity * (float)quad_colour.alpha);
	}


	switch (direction) 
	{
		// For the top, right, bottom, left directions the fill element already describes where we should draw the fill,
		// we only need to generate the final texture coordinates here.
	case Direction::Top:    texcoords[0].y = texcoords[0].y + (1.0f - value) * (texcoords[1].y - texcoords[0].y); break;
	case Direction::Right:  texcoords[1].x = texcoords[0].x + value * (texcoords[1].x - texcoords[0].x);          break;
	case Direction::Bottom: texcoords[1].y = texcoords[0].y + value * (texcoords[1].y - texcoords[0].y);          break;
	case Direction::Left:   texcoords[0].x = texcoords[0].x + (1.0f - value) * (texcoords[1].x - texcoords[0].x); break;

	case Direction::Clockwise:
	case Direction::CounterClockwise:
	{
		// The circular directions require custom geometry as a box is insufficient.
		// We divide the "circle" into eight parts, here called octants, such that each part can be represented by a triangle.
		// 'num_octants' tells us how many of these are completely or partially filled.
		const int num_octants = Core::Math::Clamp(Core::Math::RoundUpToInteger(8.f * value), 0, 8);
		const int num_vertices = 2 + num_octants;
		const int num_triangles = num_octants;
		const bool cw = (direction == Direction::Clockwise);

		if (num_octants == 0)
			break;

		vertices.resize(num_vertices);
		indices.resize(3 * num_triangles);

		RMLUI_ASSERT(int(start_edge) >= int(StartEdge::Top) && int(start_edge) <= int(StartEdge::Left));

		// The octant our "circle" expands from.
		const int start_octant = 2 * int(start_edge);

		// Positions along the unit square (clockwise, index 0 on top)
		const float x[8] = {  0,  1, 1, 1, 0, -1, -1, -1 };
		const float y[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

		// Set the position of the octant vertices to be rendered.
		for (int i = 0; i <= num_octants; i++)
		{
			int j = (cw ? i : 8 - i);
			j = ((j + start_octant) % 8);
			vertices[i].position = Vector2f(x[j], y[j]);
		}

		// Find the position of the vertex representing the partially filled triangle.
		if (value < 1.f)
		{
			using namespace Core::Math;
			const float angle_offset = float(start_octant) / 8.f * 2.f * RMLUI_PI;
			const float angle = angle_offset + (cw ? 1.f : -1.f) * value * 2.f * RMLUI_PI;
			Vector2f pos(Sin(angle), -Cos(angle));
			// Project it from the circle towards the surrounding unit square.
			pos = pos / Max(AbsoluteValue(pos.x), AbsoluteValue(pos.y));
			vertices[num_octants].position = pos;
		}

		const int i_center = num_vertices - 1;
		vertices[i_center].position = Vector2f(0, 0);

		for (int i = 0; i < num_triangles; i++)
		{
			indices[i * 3 + 0] = i_center;
			indices[i * 3 + 2] = i;
			indices[i * 3 + 1] = i + 1;
		}

		for (int i = 0; i < num_vertices; i++)
		{
			// Transform position from [-1, 1] to [0, 1] and then to [0, size]
			const Vector2f pos = (Vector2f(1, 1) + vertices[i].position) * 0.5f;
			vertices[i].position = pos * render_size;
			vertices[i].tex_coord = texcoords[0] + pos * (texcoords[1] - texcoords[0]);
			vertices[i].colour = quad_colour;
		}
	}
		break;
		RMLUI_UNUSED_SWITCH_ENUM(Direction::Count);
	}

	const bool is_circular = (direction == Direction::Clockwise || direction == Direction::CounterClockwise);

	if(!is_circular)
	{
		vertices.resize(4);
		indices.resize(6);
		Rml::Core::GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0), render_size, quad_colour, texcoords[0], texcoords[1]);
	}
}

bool ElementProgressBar::LoadTexture()
{
	texture_dirty = false;
	geometry_dirty = true;
	rect_set = false;

	Core::String name;

	if (auto property = fill->GetLocalProperty(Core::PropertyId::FillImage))
		name = property->Get<Core::String>();

	Core::ElementDocument* document = GetOwnerDocument();

	bool texture_set = false;

	if(!name.empty() && document)
	{
		// Check for a sprite first, this takes precedence.
		if (auto& style_sheet = document->GetStyleSheet())
		{
			if (const Core::Sprite* sprite = style_sheet->GetSprite(name))
			{
				rect = sprite->rectangle;
				rect_set = true;
				texture = sprite->sprite_sheet->texture;
				texture_set = true;
			}
		}

		// Otherwise, treat it as a path
		if (!texture_set)
		{
			Core::URL source_url;
			source_url.SetURL(document->GetSourceURL());
			texture.Set(name, source_url.GetPath());
			texture_set = true;
		}
	}

	if (!texture_set)
	{
		texture = {};
		rect = {};
	}

	// Set the texture onto our geometry object.
	geometry.SetTexture(&texture);

	return true;
}

}
}
