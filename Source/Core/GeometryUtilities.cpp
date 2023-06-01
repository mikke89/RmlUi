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

#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "GeometryBackgroundBorder.h"

namespace Rml {

GeometryUtilities::GeometryUtilities() {}

GeometryUtilities::~GeometryUtilities() {}

void GeometryUtilities::GenerateQuad(Vertex* vertices, int* indices, Vector2f origin, Vector2f dimensions, Colourb colour, int index_offset)
{
	GenerateQuad(vertices, indices, origin, dimensions, colour, Vector2f(0, 0), Vector2f(1, 1), index_offset);
}

void GeometryUtilities::GenerateQuad(Vertex* vertices, int* indices, Vector2f origin, Vector2f dimensions, Colourb colour, Vector2f top_left_texcoord,
	Vector2f bottom_right_texcoord, int index_offset)
{
	vertices[0].position = origin;
	vertices[0].colour = colour;
	vertices[0].tex_coord = top_left_texcoord;

	vertices[1].position = Vector2f(origin.x + dimensions.x, origin.y);
	vertices[1].colour = colour;
	vertices[1].tex_coord = Vector2f(bottom_right_texcoord.x, top_left_texcoord.y);

	vertices[2].position = origin + dimensions;
	vertices[2].colour = colour;
	vertices[2].tex_coord = bottom_right_texcoord;

	vertices[3].position = Vector2f(origin.x, origin.y + dimensions.y);
	vertices[3].colour = colour;
	vertices[3].tex_coord = Vector2f(top_left_texcoord.x, bottom_right_texcoord.y);

	indices[0] = index_offset + 0;
	indices[1] = index_offset + 3;
	indices[2] = index_offset + 1;

	indices[3] = index_offset + 1;
	indices[4] = index_offset + 3;
	indices[5] = index_offset + 2;
}

void GeometryUtilities::GenerateLine(Geometry* geometry, Vector2f position, Vector2f size, Colourb color)
{
	Math::SnapToPixelGrid(position, size);

	Vector<Vertex>& line_vertices = geometry->GetVertices();
	Vector<int>& line_indices = geometry->GetIndices();

	const int vertices_i0 = (int)line_vertices.size();
	const int indices_i0 = (int)line_indices.size();

	line_vertices.resize(line_vertices.size() + 4);
	line_indices.resize(line_indices.size() + 6);

	GeometryUtilities::GenerateQuad(line_vertices.data() + vertices_i0, line_indices.data() + indices_i0, position, size, color, vertices_i0);
}

void GeometryUtilities::GenerateBackgroundBorder(Geometry* out_geometry, const Box& box, Vector2f offset, Vector4f border_radius,
	Colourb background_color, const Colourb border_colors[4])
{
	RMLUI_ASSERT(border_colors);

	Vector<Vertex>& vertices = out_geometry->GetVertices();
	Vector<int>& indices = out_geometry->GetIndices();

	EdgeSizes border_widths = {
		// TODO: Move rounding to computed values (round border only).
		Math::Round(box.GetEdge(BoxArea::Border, BoxEdge::Top)),
		Math::Round(box.GetEdge(BoxArea::Border, BoxEdge::Right)),
		Math::Round(box.GetEdge(BoxArea::Border, BoxEdge::Bottom)),
		Math::Round(box.GetEdge(BoxArea::Border, BoxEdge::Left)),
	};

	int num_borders = 0;

	for (int i = 0; i < 4; i++)
		if (border_colors[i].alpha > 0 && border_widths[i] > 0)
			num_borders += 1;

	const Vector2f padding_size = box.GetSize(BoxArea::Padding).Round();

	const bool has_background = (background_color.alpha > 0 && padding_size.x > 0 && padding_size.y > 0);
	const bool has_border = (num_borders > 0);

	if (!has_background && !has_border)
		return;

	// Reserve geometry. A conservative estimate, does not take border-radii into account and assumes same-colored borders.
	const int estimated_num_vertices = 4 * int(has_background) + 2 * num_borders;
	const int estimated_num_triangles = 2 * int(has_background) + 2 * num_borders;
	vertices.reserve((int)vertices.size() + estimated_num_vertices);
	indices.reserve((int)indices.size() + 3 * estimated_num_triangles);

	// Generate the geometry.
	GeometryBackgroundBorder geometry(vertices, indices);
	const BorderMetrics metrics = GeometryBackgroundBorder::ComputeBorderMetrics(offset.Round(), border_widths, padding_size, border_radius);

	if (has_background)
		geometry.DrawBackground(metrics, background_color);

	if (has_border)
		geometry.DrawBorder(metrics, border_widths, border_colors);

#if 0
	// Debug draw vertices
	if (border_radius != Vector4f(0))
	{
		const int num_vertices = (int)vertices.size();
		const int num_indices = (int)indices.size();

		vertices.resize(num_vertices + 4 * num_vertices);
		indices.resize(num_indices + 6 * num_indices);

		for (int i = 0; i < num_vertices; i++)
		{
			GeometryUtilities::GenerateQuad(vertices.data() + num_vertices + 4 * i, indices.data() + num_indices + 6 * i, vertices[i].position,
				Vector2f(3, 3), Colourb(255, 0, (i % 2) == 0 ? 0 : 255), num_vertices + 4 * i);
		}
	}
#endif

#ifdef RMLUI_DEBUG
	const int num_vertices = (int)vertices.size();
	for (int index : indices)
	{
		RMLUI_ASSERT(index < num_vertices);
	}
#endif
}

void GeometryUtilities::GenerateBackground(Geometry* out_geometry, const Box& box, Vector2f offset, Vector4f border_radius, Colourb color,
	BoxArea fill_area)
{
	RMLUI_ASSERTMSG(fill_area >= BoxArea::Border && fill_area <= BoxArea::Content,
		"Rectangle geometry only supports border, padding and content boxes.");

	EdgeSizes edge_sizes = {};
	for (int area = (int)BoxArea::Border; area < (int)fill_area; area++)
	{
		// TODO: Move rounding to computed values (round border only).
		edge_sizes[0] += Math::Round(box.GetEdge(BoxArea(area), BoxEdge::Top));
		edge_sizes[1] += Math::Round(box.GetEdge(BoxArea(area), BoxEdge::Right));
		edge_sizes[2] += Math::Round(box.GetEdge(BoxArea(area), BoxEdge::Bottom));
		edge_sizes[3] += Math::Round(box.GetEdge(BoxArea(area), BoxEdge::Left));
	}

	const Vector2f inner_size = box.GetSize(fill_area).Round();

	const bool has_background = (color.alpha > 0 && inner_size.x > 0 && inner_size.y > 0);
	if (!has_background)
		return;

	const BorderMetrics metrics = GeometryBackgroundBorder::ComputeBorderMetrics(offset.Round(), edge_sizes, inner_size, border_radius);

	Vector<Vertex>& vertices = out_geometry->GetVertices();
	Vector<int>& indices = out_geometry->GetIndices();

	// Reserve geometry. A conservative estimate, does not take border-radii into account.
	vertices.reserve((int)vertices.size() + 4);
	indices.reserve((int)indices.size() + 6);

	// Generate the geometry
	GeometryBackgroundBorder geometry(vertices, indices);
	geometry.DrawBackground(metrics, color);
}

} // namespace Rml
