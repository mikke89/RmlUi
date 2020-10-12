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

#include "GeometryBackgroundBorder.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/Math.h"
#include <algorithm>
#include <float.h>

namespace Rml {

GeometryBackgroundBorder::GeometryBackgroundBorder(Vector<Vertex>& vertices, Vector<int>& indices) : vertices(vertices), indices(indices)
{}

void GeometryBackgroundBorder::Draw(Vector<Vertex>& vertices, Vector<int>& indices, CornerSizes radii, const Box& box, const Vector2f offset, const Colourb background_color, const Colourb* border_colors)
{
	using Edge = Box::Edge;

	EdgeSizes border_widths = {
		Math::RoundFloat(box.GetEdge(Box::BORDER, Edge::TOP)),
		Math::RoundFloat(box.GetEdge(Box::BORDER, Edge::RIGHT)),
		Math::RoundFloat(box.GetEdge(Box::BORDER, Edge::BOTTOM)),
		Math::RoundFloat(box.GetEdge(Box::BORDER, Edge::LEFT)),
	};

	int num_borders = 0;

	if (border_colors)
	{
		for (int i = 0; i < 4; i++)
			if (border_colors[i].alpha > 0 && border_widths[i] > 0)
				num_borders += 1;
	}

	const Vector2f padding_size = box.GetSize(Box::PADDING).Round();

	const bool has_background = (background_color.alpha > 0 && padding_size.x > 0 && padding_size.y > 0);
	const bool has_border = (num_borders > 0);

	if (!has_background && !has_border)
		return;

	// -- Find the corner positions --

	const Vector2f border_position = (offset + box.GetPosition(Box::BORDER)).Round();
	const Vector2f padding_position = border_position + Vector2f(border_widths[Edge::LEFT], border_widths[Edge::TOP]);
	const Vector2f border_size = padding_size + Vector2f(border_widths[Edge::LEFT] + border_widths[Edge::RIGHT], border_widths[Edge::TOP] + border_widths[Edge::BOTTOM]);

	// Border edge positions
	CornerPositions positions_outer = {
		border_position,
		border_position + Vector2f(border_size.x, 0),
		border_position + border_size,
		border_position + Vector2f(0, border_size.y)
	};

	// Padding edge positions
	CornerPositions positions_inner = {
		padding_position,
		padding_position + Vector2f(padding_size.x, 0),
		padding_position + padding_size,
		padding_position + Vector2f(0, padding_size.y)
	};


	// -- For curved borders, find the positions to draw ellipses around, and the scaled outer and inner radii --

	const float sum_radius = (radii[TOP_LEFT] + radii[TOP_RIGHT] + radii[BOTTOM_RIGHT] + radii[BOTTOM_LEFT]);
	const bool has_radius = (sum_radius > 1.f);

	// Curved borders are drawn as circles (outer border) and ellipses (inner border) around the centers.
	CornerPositions positions_circle_center;

	// Radii of the padding edges, 2-dimensional as these can be ellipses.
	// The inner radii is effectively the (signed) distance from the circle center to the padding edge.
	// They can also be zero or negative, in which case a sharp corner should be drawn instead of an arc.
	CornerSizes2 inner_radii;

	if (has_radius)
	{
		// Scale the radii such that we have no overlapping curves.
		float scale_factor = FLT_MAX;
		scale_factor = Math::Min(scale_factor, padding_size.x / (radii[TOP_LEFT] + radii[TOP_RIGHT]));       // Top
		scale_factor = Math::Min(scale_factor, padding_size.y / (radii[TOP_RIGHT] + radii[BOTTOM_RIGHT]));   // Right
		scale_factor = Math::Min(scale_factor, padding_size.x / (radii[BOTTOM_RIGHT] + radii[BOTTOM_LEFT])); // Bottom
		scale_factor = Math::Min(scale_factor, padding_size.y / (radii[BOTTOM_LEFT] + radii[TOP_LEFT]));     // Left

		scale_factor = Math::Min(1.0f, scale_factor);

		for (float& radius : radii)
			radius = Math::RoundFloat(radius * scale_factor);

		// Place the circle/ellipse centers
		positions_circle_center = {
			positions_outer[TOP_LEFT]     + Vector2f(1, 1) * radii[TOP_LEFT],
			positions_outer[TOP_RIGHT]    + Vector2f(-1, 1) * radii[TOP_RIGHT],
			positions_outer[BOTTOM_RIGHT] + Vector2f(-1, -1) * radii[BOTTOM_RIGHT],
			positions_outer[BOTTOM_LEFT]  + Vector2f(1, -1) * radii[BOTTOM_LEFT]
		};

		inner_radii = {
			Vector2f(radii[TOP_LEFT])     - Vector2f(border_widths[Edge::LEFT], border_widths[Edge::TOP]),
			Vector2f(radii[TOP_RIGHT])    - Vector2f(border_widths[Edge::RIGHT], border_widths[Edge::TOP]),
			Vector2f(radii[BOTTOM_RIGHT]) - Vector2f(border_widths[Edge::RIGHT], border_widths[Edge::BOTTOM]),
			Vector2f(radii[BOTTOM_LEFT])  - Vector2f(border_widths[Edge::LEFT], border_widths[Edge::BOTTOM])
		};
	}

	// -- Generate the geometry --

	GeometryBackgroundBorder geometry(vertices, indices);

	{
		// Reserve geometry. A conservative estimate, does not take border-radii into account and assumes same-colored borders.
		const int estimated_num_vertices = 4 * int(has_background) + 2 * num_borders;
		const int estimated_num_triangles = 2 * int(has_background) + 2 * num_borders;

		vertices.reserve((int)vertices.size() + estimated_num_vertices);
		indices.reserve((int)indices.size() + 3 * estimated_num_triangles);
	}

	// Draw the background
	if (has_background)
	{
		const int offset_vertices = (int)vertices.size();

		for (int corner = 0; corner < 4; corner++)
			geometry.DrawBackgroundCorner(Corner(corner), positions_inner[corner], positions_circle_center[corner], radii[corner], inner_radii[corner], background_color);

		geometry.FillBackground(offset_vertices);
	}

	// Draw the border
	if (has_border)
	{
		using Edge = Box::Edge;
		const int offset_vertices = (int)vertices.size();

		const bool draw_edge[4] = {
			border_widths[Edge::TOP] > 0 && border_colors[Edge::TOP].alpha > 0,
			border_widths[Edge::RIGHT] > 0 && border_colors[Edge::RIGHT].alpha > 0,
			border_widths[Edge::BOTTOM] > 0 && border_colors[Edge::BOTTOM].alpha > 0,
			border_widths[Edge::LEFT] > 0 && border_colors[Edge::LEFT].alpha > 0
		};

		const bool draw_corner[4] = {
			draw_edge[Edge::TOP] || draw_edge[Edge::LEFT],
			draw_edge[Edge::TOP] || draw_edge[Edge::RIGHT],
			draw_edge[Edge::BOTTOM] || draw_edge[Edge::RIGHT],
			draw_edge[Edge::BOTTOM] || draw_edge[Edge::LEFT]
		};

		for (int corner = 0; corner < 4; corner++)
		{
			const Edge edge0 = Edge((corner + 3) % 4);
			const Edge edge1 = Edge(corner);

			if (draw_corner[corner])
				geometry.DrawBorderCorner(Corner(corner), positions_outer[corner], positions_inner[corner], positions_circle_center[corner],
					radii[corner], inner_radii[corner], border_colors[edge0], border_colors[edge1]);

			if (draw_edge[edge1])
			{
				RMLUI_ASSERTMSG(draw_corner[corner] && draw_corner[(corner + 1) % 4], "Border edges can only be drawn if both of its connected corners are drawn.");
				geometry.FillEdge(edge1 == Edge::LEFT ? offset_vertices : (int)vertices.size());
			}
		}
	}

#if 0
	// Debug draw vertices
	if (has_radius)
	{
		const int num_vertices = vertices.size();
		const int num_indices = indices.size();

		vertices.resize(num_vertices + 4 * num_vertices);
		indices.resize(num_indices + 6 * num_indices);

		for (int i = 0; i < num_vertices; i++)
		{
			GeometryUtilities::GenerateQuad(vertices.data() + num_vertices + 4 * i, indices.data() + num_indices + 6 * i, vertices[i].position, Vector2f(3, 3), Colourb(255, 0, (i % 2) == 0 ? 0 : 255), num_vertices + 4 * i);
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



void GeometryBackgroundBorder::DrawBackgroundCorner(Corner corner, Vector2f pos_inner, Vector2f pos_circle_center, float R, Vector2f r, Colourb color)
{
	if (R == 0 || r.x <= 0 || r.y <= 0)
	{
		DrawPoint(pos_inner, color);
	}
	else if (r.x > 0 && r.y > 0)
	{
		const float a0 = float((int)corner + 2) * 0.5f * Math::RMLUI_PI;
		const float a1 = float((int)corner + 3) * 0.5f * Math::RMLUI_PI;
		const int num_points = GetNumPoints(R);
		DrawArc(pos_circle_center, r, a0, a1, color, color, num_points);
	}
}

void GeometryBackgroundBorder::DrawPoint(Vector2f pos, Colourb color)
{
	const int offset_vertices = (int)vertices.size();

	vertices.resize(offset_vertices + 1);

	vertices[offset_vertices].position = pos;
	vertices[offset_vertices].colour = color;
}

void GeometryBackgroundBorder::DrawArc(Vector2f pos_center, Vector2f r, float a0, float a1, Colourb color0, Colourb color1, int num_points)
{
	RMLUI_ASSERT(num_points >= 2 && r.x > 0 && r.y > 0);

	const int offset_vertices = (int)vertices.size();

	vertices.resize(offset_vertices + num_points);

	for (int i = 0; i < num_points; i++)
	{
		const float t = float(i) / float(num_points - 1);

		const float a = Math::Lerp(t, a0, a1);
		const Colourb color = Math::Lerp(t, color0, color1);

		const Vector2f unit_vector(Math::Cos(a), Math::Sin(a));

		vertices[offset_vertices + i].position = unit_vector * r + pos_center;
		vertices[offset_vertices + i].colour = color;
	}
}

void GeometryBackgroundBorder::FillBackground(int index_start)
{
	const int num_added_vertices = (int)vertices.size() - index_start;
	const int offset_indices = (int)indices.size();

	const int num_triangles = (num_added_vertices - 2);

	indices.resize(offset_indices + 3 * num_triangles);

	for (int i = 0; i < num_triangles; i++)
	{
		indices[offset_indices + 3 * i] = index_start;
		indices[offset_indices + 3 * i + 1] = index_start + i + 2;
		indices[offset_indices + 3 * i + 2] = index_start + i + 1;
	}
}

void GeometryBackgroundBorder::DrawBorderCorner(Corner corner, Vector2f pos_outer, Vector2f pos_inner, Vector2f pos_circle_center, float R, Vector2f r, Colourb color0, Colourb color1)
{
	const float a0 = float((int)corner + 2) * 0.5f * Math::RMLUI_PI;
	const float a1 = float((int)corner + 3) * 0.5f * Math::RMLUI_PI;

	if (R == 0)
	{
		DrawPointPoint(pos_outer, pos_inner, color0, color1);
	}
	else if (r.x > 0 && r.y > 0)
	{
		DrawArcArc(pos_circle_center, R, r, a0, a1, color0, color1, GetNumPoints(R));
	}
	else
	{
		DrawArcPoint(pos_circle_center, pos_inner, R, a0, a1, color0, color1, GetNumPoints(R));
	}
}

void GeometryBackgroundBorder::DrawPointPoint(Vector2f pos_outer, Vector2f pos_inner, Colourb color0, Colourb color1)
{
	const bool different_color = (color0 != color1);

	vertices.reserve((int)vertices.size() + (different_color ? 4 : 2));

	DrawPoint(pos_inner, color0);
	DrawPoint(pos_outer, color0);

	if (different_color)
	{
		DrawPoint(pos_inner, color1);
		DrawPoint(pos_outer, color1);
	}
}

void GeometryBackgroundBorder::DrawArcArc(Vector2f pos_center, float R, Vector2f r, float a0, float a1, Colourb color0, Colourb color1, int num_points)
{
	RMLUI_ASSERT(num_points >= 2 && R > 0 && r.x > 0 && r.y > 0);

	const int num_triangles = 2 * (num_points - 1);

	const int offset_vertices = (int)vertices.size();
	const int offset_indices = (int)indices.size();

	vertices.resize(offset_vertices + 2 * num_points);
	indices.resize(offset_indices + 3 * num_triangles);

	for (int i = 0; i < num_points; i++)
	{
		const float t = float(i) / float(num_points - 1);

		const float a = Math::Lerp(t, a0, a1);
		const Colourb color = Math::Lerp(t, color0, color1);

		const Vector2f unit_vector(Math::Cos(a), Math::Sin(a));

		vertices[offset_vertices + 2 * i].position = unit_vector * r + pos_center;
		vertices[offset_vertices + 2 * i].colour = color;
		vertices[offset_vertices + 2 * i + 1].position = unit_vector * R + pos_center;
		vertices[offset_vertices + 2 * i + 1].colour = color;
	}

	for (int i = 0; i < num_triangles; i += 2)
	{
		indices[offset_indices + 3 * i + 0] = offset_vertices + i + 0;
		indices[offset_indices + 3 * i + 1] = offset_vertices + i + 2;
		indices[offset_indices + 3 * i + 2] = offset_vertices + i + 1;

		indices[offset_indices + 3 * i + 3] = offset_vertices + i + 1;
		indices[offset_indices + 3 * i + 4] = offset_vertices + i + 2;
		indices[offset_indices + 3 * i + 5] = offset_vertices + i + 3;
	}
}

void GeometryBackgroundBorder::DrawArcPoint(Vector2f pos_center, Vector2f pos_inner, float R, float a0, float a1, Colourb color0, Colourb color1, int num_points)
{
	RMLUI_ASSERT(R > 0 && num_points >= 2);

	const int offset_vertices = (int)vertices.size();
	vertices.reserve(offset_vertices + num_points + 2);

	// Generate the vertices. We could also split the arc mid-way to create a sharp color transition.
	DrawPoint(pos_inner, color0);
	DrawArc(pos_center, Vector2f(R), a0, a1, color0, color1, num_points);
	DrawPoint(pos_inner, color1);

	RMLUI_ASSERT((int)vertices.size() - offset_vertices == num_points + 2);

	// Swap the last two vertices such that the outer edge vertex is last, see the comment for the border drawing functions. Their colors should already be the same.
	const int last_vertex = (int)vertices.size() - 1;
	std::swap(vertices[last_vertex - 1].position, vertices[last_vertex].position);

	// Generate the indices
	const int num_triangles = (num_points - 1);

	const int i_vertex_inner0 = offset_vertices;
	const int i_vertex_inner1 = last_vertex - 1;

	const int offset_indices = (int)indices.size();
	indices.resize(offset_indices + 3 * num_triangles);

	for (int i = 0; i < num_triangles; i++)
	{
		indices[offset_indices + 3 * i + 0] = (i > num_triangles / 2 ? i_vertex_inner1 : i_vertex_inner0);
		indices[offset_indices + 3 * i + 1] = offset_vertices + i + 2;
		indices[offset_indices + 3 * i + 2] = offset_vertices + i + 1;
	}

	// Since we swapped the last two vertices we also need to change the last triangle.
	indices[offset_indices + 3 * (num_triangles - 1) + 1] = last_vertex;
}

void GeometryBackgroundBorder::FillEdge(int index_next_corner)
{
	const int offset_indices = (int)indices.size();
	const int num_vertices = (int)vertices.size();
	RMLUI_ASSERT(num_vertices >= 2);

	indices.resize(offset_indices + 6);

	indices[offset_indices + 0] = num_vertices - 2;
	indices[offset_indices + 1] = index_next_corner;
	indices[offset_indices + 2] = num_vertices - 1;

	indices[offset_indices + 3] = num_vertices - 1;
	indices[offset_indices + 4] = index_next_corner;
	indices[offset_indices + 5] = index_next_corner + 1;
}

int GeometryBackgroundBorder::GetNumPoints(float R) const
{
	return Math::Clamp(3 + Math::RoundToInteger(R / 6.f), 2, 100);
}

} // namespace Rml
