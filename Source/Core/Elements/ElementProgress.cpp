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

#include "../../../Include/RmlUi/Core/Elements/ElementProgress.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/ElementDocument.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../../Include/RmlUi/Core/StyleSheet.h"
#include "../../../Include/RmlUi/Core/URL.h"
#include <algorithm>

namespace Rml {

ElementProgress::ElementProgress(const String& tag) :
	Element(tag), direction(DefaultDirection), start_edge(DefaultStartEdge), fill(nullptr), rect_set(false)
{
	if (tag == "progressbar")
		Log::Message(Log::LT_WARNING, "Deprecation notice: Element '<progressbar>' renamed to '<progress>', please adjust RML tags accordingly.");

	geometry_dirty = false;

	// Add the fill element as a non-DOM element.
	ElementPtr fill_element = Factory::InstanceElement(this, "*", "fill", XMLAttributes());
	RMLUI_ASSERT(fill_element);
	fill = AppendChild(std::move(fill_element), false);
}

ElementProgress::~ElementProgress() {}

float ElementProgress::GetValue() const
{
	const float max_value = GetMax();
	return Math::Clamp(GetAttribute<float>("value", 0.0f), 0.0f, max_value);
}

float ElementProgress::GetMax() const
{
	const float max_value = GetAttribute<float>("max", 1.0f);
	return max_value <= 0.0f ? 1.0f : max_value;
}

void ElementProgress::SetMax(float max_value)
{
	SetAttribute("max", max_value);
}

void ElementProgress::SetValue(float in_value)
{
	SetAttribute("value", in_value);
}

bool ElementProgress::GetIntrinsicDimensions(Vector2f& dimensions, float& /*ratio*/)
{
	dimensions.x = 256;
	dimensions.y = 16;
	return true;
}

void ElementProgress::OnRender()
{
	// Some properties may change geometry without dirtying the layout, eg. opacity.
	if (geometry_dirty)
		GenerateGeometry();

	// Render the geometry at the fill element's content region.
	geometry.Render(fill->GetAbsoluteOffset(), texture);
}

void ElementProgress::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("value") != changed_attributes.end() || changed_attributes.find("max") != changed_attributes.end())
	{
		geometry_dirty = true;
	}

	if (changed_attributes.find("direction") != changed_attributes.end())
	{
		using DirectionNameList = Array<String, size_t(Direction::Count)>;
		static const DirectionNameList names = {"top", "right", "bottom", "left", "clockwise", "counter-clockwise"};

		direction = DefaultDirection;

		String name = StringUtilities::ToLower(GetAttribute<String>("direction", ""));
		auto it = std::find(names.begin(), names.end(), name);

		size_t index = size_t(it - names.begin());
		if (index < size_t(Direction::Count))
			direction = Direction(index);

		geometry_dirty = true;
	}

	if (changed_attributes.find("start-edge") != changed_attributes.end())
	{
		using StartEdgeNameList = Array<String, size_t(StartEdge::Count)>;
		static const StartEdgeNameList names = {"top", "right", "bottom", "left"};

		start_edge = DefaultStartEdge;

		String name = StringUtilities::ToLower(GetAttribute<String>("start-edge", ""));
		auto it = std::find(names.begin(), names.end(), name);

		size_t index = size_t(it - names.begin());
		if (index < size_t(StartEdge::Count))
			start_edge = StartEdge(index);

		geometry_dirty = true;
	}
}

void ElementProgress::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	if (changed_properties.Contains(PropertyId::ImageColor) || changed_properties.Contains(PropertyId::Opacity))
	{
		geometry_dirty = true;
	}

	if (changed_properties.Contains(PropertyId::FillImage))
	{
		LoadTexture();
	}
}

void ElementProgress::OnResize()
{
	const Vector2f element_size = GetBox().GetSize();

	// Build and set the 'fill' element's box. Here we are mainly interested in all the edge sizes set by the user.
	// The content size of the box is here scaled to fit inside the progress bar. Then, during 'CreateGeometry()',
	// the 'fill' element's content size is further shrunk according to 'value' along the proper direction.
	Box fill_box;

	ElementUtilities::BuildBox(fill_box, element_size, fill);

	const Vector2f margin_top_left(fill_box.GetEdge(BoxArea::Margin, BoxEdge::Left), fill_box.GetEdge(BoxArea::Margin, BoxEdge::Top));
	const Vector2f edge_size = fill_box.GetSize(BoxArea::Margin) - fill_box.GetSize(BoxArea::Content);

	fill_offset = GetBox().GetPosition() + margin_top_left;
	fill_size = element_size - edge_size;

	fill_box.SetContent(fill_size);
	fill->SetBox(fill_box);

	geometry_dirty = true;
}

void ElementProgress::GenerateGeometry()
{
	geometry_dirty = false;

	// Warn the user when using the old approach of adding the 'fill-image' property to the 'fill' element.
	if (fill->GetLocalProperty(PropertyId::FillImage))
		Log::Message(Log::LT_WARNING,
			"Breaking change: The 'fill-image' property now needs to be set on the <progress> element, instead of its inner <fill> element. Please "
			"update your RCSS source to fix progress bars in this document.");

	const float normalized_value = GetValue() / GetMax();
	Vector2f render_size = fill_size;

	{
		// Size and offset the fill element depending on the progress value.
		Vector2f offset = fill_offset;

		switch (direction)
		{
		case Direction::Top:
			render_size.y = fill_size.y * normalized_value;
			offset.y = fill_offset.y + fill_size.y - render_size.y;
			break;
		case Direction::Right: render_size.x = fill_size.x * normalized_value; break;
		case Direction::Bottom: render_size.y = fill_size.y * normalized_value; break;
		case Direction::Left:
			render_size.x = fill_size.x * normalized_value;
			offset.x = fill_offset.x + fill_size.x - render_size.x;
			break;
		case Direction::Clockwise:
		case Direction::CounterClockwise:
			// Circular progress bars cannot use a box to shape the fill element, instead we need to manually create the geometry from the image
			// texture. Thus, we leave the size and offset untouched as a canvas for the manual geometry.
			break;
		case Direction::Count: break;
		}

		Box fill_box = fill->GetBox();
		fill_box.SetContent(render_size);
		fill->SetBox(fill_box);
		fill->SetOffset(offset, this);
	}

	Mesh mesh = geometry.Release(Geometry::ReleaseMode::ClearMesh);

	// If we don't have a fill texture, then there is no need to generate manual geometry, and we are done here.
	// Instead, users can style the fill element eg. by decorators.
	if (!texture)
		return;

	// Otherwise, the 'fill-image' property is set, let's generate its geometry.
	Vector<Vertex>& vertices = mesh.vertices;
	Vector<int>& indices = mesh.indices;

	Vector2f texcoords[2];
	if (rect_set)
	{
		Vector2f texture_dimensions = Vector2f(Math::Max(texture.GetDimensions(), Vector2i(1)));
		texcoords[0] = rect.TopLeft() / texture_dimensions;
		texcoords[1] = rect.BottomRight() / texture_dimensions;
	}
	else
	{
		texcoords[0] = Vector2f(0, 0);
		texcoords[1] = Vector2f(1, 1);
	}

	const ComputedValues& computed = GetComputedValues();
	const ColourbPremultiplied quad_colour = computed.image_color().ToPremultiplied(computed.opacity());

	switch (direction)
	{
		// For the top, right, bottom, left directions the fill element already describes where we should draw the fill,
		// we only need to generate the final texture coordinates here.
	case Direction::Top: texcoords[0].y = texcoords[0].y + (1.0f - normalized_value) * (texcoords[1].y - texcoords[0].y); break;
	case Direction::Right: texcoords[1].x = texcoords[0].x + normalized_value * (texcoords[1].x - texcoords[0].x); break;
	case Direction::Bottom: texcoords[1].y = texcoords[0].y + normalized_value * (texcoords[1].y - texcoords[0].y); break;
	case Direction::Left: texcoords[0].x = texcoords[0].x + (1.0f - normalized_value) * (texcoords[1].x - texcoords[0].x); break;

	case Direction::Clockwise:
	case Direction::CounterClockwise:
	{
		// The circular directions require custom geometry as a box is insufficient.
		// We divide the "circle" into eight parts, here called octants, such that each part can be represented by a triangle.
		// 'num_octants' tells us how many of these are completely or partially filled.
		const int num_octants = Math::Clamp(Math::RoundUpToInteger(8.f * normalized_value), 0, 8);
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
		const float x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
		const float y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

		// Set the position of the octant vertices to be rendered.
		for (int i = 0; i <= num_octants; i++)
		{
			int j = (cw ? i : 8 - i);
			j = ((j + start_octant) % 8);
			vertices[i].position = Vector2f(x[j], y[j]);
		}

		// Find the position of the vertex representing the partially filled triangle.
		if (normalized_value < 1.f)
		{
			using namespace Math;
			const float angle_offset = float(start_octant) / 8.f * 2.f * RMLUI_PI;
			const float angle = angle_offset + (cw ? 1.f : -1.f) * normalized_value * 2.f * RMLUI_PI;
			Vector2f pos(Sin(angle), -Cos(angle));
			// Project it from the circle towards the surrounding unit square.
			pos = pos / Max(Absolute(pos.x), Absolute(pos.y));
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

	case Direction::Count: break;
	}

	const bool is_circular = (direction == Direction::Clockwise || direction == Direction::CounterClockwise);

	if (!is_circular)
	{
		MeshUtilities::GenerateQuad(mesh, Vector2f(0), render_size, quad_colour, texcoords[0], texcoords[1]);
	}

	geometry = GetRenderManager()->MakeGeometry(std::move(mesh));
}

bool ElementProgress::LoadTexture()
{
	geometry_dirty = true;
	rect_set = false;

	String name;

	if (const Property* property = GetLocalProperty(PropertyId::FillImage))
		name = property->Get<String>();

	RenderManager* render_manager = GetRenderManager();
	if (!render_manager)
		return false;

	ElementDocument* document = GetOwnerDocument();

	bool texture_set = false;

	if (!name.empty() && document)
	{
		// Check for a sprite first, this takes precedence.
		if (const StyleSheet* style_sheet = document->GetStyleSheet())
		{
			if (const Sprite* sprite = style_sheet->GetSprite(name))
			{
				rect = sprite->rectangle;
				rect_set = true;
				texture = sprite->sprite_sheet->texture_source.GetTexture(*render_manager);
				texture_set = true;
			}
		}

		// Otherwise, treat it as a path
		if (!texture_set)
		{
			URL source_url;
			source_url.SetURL(document->GetSourceURL());
			texture = render_manager->LoadTexture(name, source_url.GetPath());
			texture_set = true;
		}
	}

	if (!texture_set)
	{
		texture = {};
		rect = {};
	}

	return true;
}

} // namespace Rml
