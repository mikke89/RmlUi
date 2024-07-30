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

#include "GeometryBackgroundBorder.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/Math.h"
#include <algorithm>
#include <float.h>

namespace Rml {

GeometryBackgroundBorder::GeometryBackgroundBorder(Vector<Vertex>& vertices, Vector<int>& indices) : vertices(vertices), indices(indices) {}

BorderMetrics GeometryBackgroundBorder::ComputeBorderMetrics(Vector2f outer_position, EdgeSizes edge_sizes, Vector2f inner_size,
	Vector4f outer_radii_def)
{
	BorderMetrics metrics = {};

	// -- Find the corner positions --

	const Vector2f inner_position = outer_position + Vector2f(edge_sizes[LEFT], edge_sizes[TOP]);
	const Vector2f outer_size = inner_size + Vector2f(edge_sizes[LEFT] + edge_sizes[RIGHT], edge_sizes[TOP] + edge_sizes[BOTTOM]);

	metrics.positions_outer = {
		outer_position,
		outer_position + Vector2f(outer_size.x, 0),
		outer_position + outer_size,
		outer_position + Vector2f(0, outer_size.y),
	};

	metrics.positions_inner = {
		inner_position,
		inner_position + Vector2f(inner_size.x, 0),
		inner_position + inner_size,
		inner_position + Vector2f(0, inner_size.y),
	};

	// -- For curved borders, find the positions to draw ellipses around, and the scaled outer and inner radii --

	const float sum_radius = (outer_radii_def[TOP_LEFT] + outer_radii_def[TOP_RIGHT] + outer_radii_def[BOTTOM_RIGHT] + outer_radii_def[BOTTOM_LEFT]);
	const bool has_radius = (sum_radius > 1.f);

	if (has_radius)
	{
		auto& outer_radii = metrics.outer_radii;
		outer_radii = {outer_radii_def.x, outer_radii_def.y, outer_radii_def.z, outer_radii_def.w};

		// Scale the radii such that we have no overlapping curves.
		float scale_factor = FLT_MAX;
		scale_factor = Math::Min(scale_factor, inner_size.x / (outer_radii[TOP_LEFT] + outer_radii[TOP_RIGHT]));       // Top
		scale_factor = Math::Min(scale_factor, inner_size.y / (outer_radii[TOP_RIGHT] + outer_radii[BOTTOM_RIGHT]));   // Right
		scale_factor = Math::Min(scale_factor, inner_size.x / (outer_radii[BOTTOM_RIGHT] + outer_radii[BOTTOM_LEFT])); // Bottom
		scale_factor = Math::Min(scale_factor, inner_size.y / (outer_radii[BOTTOM_LEFT] + outer_radii[TOP_LEFT]));     // Left
		scale_factor = Math::Min(1.0f, scale_factor);

		for (float& radius : outer_radii)
			radius = Math::Round(radius * scale_factor);

		// Place the circle/ellipse centers
		metrics.positions_circle_center = {
			metrics.positions_outer[TOP_LEFT] + Vector2f(1, 1) * outer_radii[TOP_LEFT],
			metrics.positions_outer[TOP_RIGHT] + Vector2f(-1, 1) * outer_radii[TOP_RIGHT],
			metrics.positions_outer[BOTTOM_RIGHT] + Vector2f(-1, -1) * outer_radii[BOTTOM_RIGHT],
			metrics.positions_outer[BOTTOM_LEFT] + Vector2f(1, -1) * outer_radii[BOTTOM_LEFT],
		};

		metrics.inner_radii = {
			Vector2f(outer_radii[TOP_LEFT]) - Vector2f(edge_sizes[LEFT], edge_sizes[TOP]),
			Vector2f(outer_radii[TOP_RIGHT]) - Vector2f(edge_sizes[RIGHT], edge_sizes[TOP]),
			Vector2f(outer_radii[BOTTOM_RIGHT]) - Vector2f(edge_sizes[RIGHT], edge_sizes[BOTTOM]),
			Vector2f(outer_radii[BOTTOM_LEFT]) - Vector2f(edge_sizes[LEFT], edge_sizes[BOTTOM]),
		};
	}

	return metrics;
}

void GeometryBackgroundBorder::DrawBackground(const BorderMetrics& metrics, ColourbPremultiplied color)
{
	const int offset_vertices = (int)vertices.size();

	for (int corner = 0; corner < 4; corner++)
		DrawBackgroundCorner(Corner(corner), metrics.positions_inner[corner], metrics.positions_circle_center[corner], metrics.outer_radii[corner],
			metrics.inner_radii[corner], color);

	FillBackground(offset_vertices);
}

void GeometryBackgroundBorder::DrawBorder(const BorderMetrics& metrics, EdgeSizes edge_sizes, const ColourbPremultiplied border_colors[4])
{
	RMLUI_ASSERT(border_colors);

	const int offset_vertices = (int)vertices.size();

	const bool draw_edge[4] = {
		edge_sizes[TOP] > 0 && border_colors[TOP].alpha > 0,
		edge_sizes[RIGHT] > 0 && border_colors[RIGHT].alpha > 0,
		edge_sizes[BOTTOM] > 0 && border_colors[BOTTOM].alpha > 0,
		edge_sizes[LEFT] > 0 && border_colors[LEFT].alpha > 0,
	};

	const bool draw_corner[4] = {
		draw_edge[TOP] || draw_edge[LEFT],
		draw_edge[TOP] || draw_edge[RIGHT],
		draw_edge[BOTTOM] || draw_edge[RIGHT],
		draw_edge[BOTTOM] || draw_edge[LEFT],
	};

	for (int corner = 0; corner < 4; corner++)
	{
		const Edge edge0 = Edge((corner + 3) % 4);
		const Edge edge1 = Edge(corner);

		if (draw_corner[corner])
		{
			DrawBorderCorner(Corner(corner), metrics.positions_outer[corner], metrics.positions_inner[corner],
				metrics.positions_circle_center[corner], metrics.outer_radii[corner], metrics.inner_radii[corner], border_colors[edge0],
				border_colors[edge1]);
		}

		if (draw_edge[edge1])
		{
			RMLUI_ASSERTMSG(draw_corner[corner] && draw_corner[(corner + 1) % 4],
				"Border edges can only be drawn if both of its connected corners are drawn.");

			FillEdge(edge1 == LEFT ? offset_vertices : (int)vertices.size());
		}
	}
}

void GeometryBackgroundBorder::DrawBackgroundCorner(Corner corner, Vector2f pos_inner, Vector2f pos_circle_center, float R, Vector2f r,
	ColourbPremultiplied color)
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

void GeometryBackgroundBorder::DrawPoint(Vector2f pos, ColourbPremultiplied color)
{
	const int offset_vertices = (int)vertices.size();

	vertices.resize(offset_vertices + 1);

	vertices[offset_vertices].position = pos;
	vertices[offset_vertices].colour = color;
}

void GeometryBackgroundBorder::DrawArc(Vector2f pos_center, Vector2f r, float a0, float a1, ColourbPremultiplied color0, ColourbPremultiplied color1,
	int num_points)
{
	RMLUI_ASSERT(num_points >= 2 && r.x > 0 && r.y > 0);

	const int offset_vertices = (int)vertices.size();

	vertices.resize(offset_vertices + num_points);

	for (int i = 0; i < num_points; i++)
	{
		const float t = float(i) / float(num_points - 1);

		const float a = Math::Lerp(t, a0, a1);

		const Vector2f unit_vector(Math::Cos(a), Math::Sin(a));

		vertices[offset_vertices + i].position = unit_vector * r + pos_center;
		vertices[offset_vertices + i].colour = Math::RoundedLerp(t, color0, color1);
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

void GeometryBackgroundBorder::DrawBorderCorner(Corner corner, Vector2f pos_outer, Vector2f pos_inner, Vector2f pos_circle_center, float R,
	Vector2f r, ColourbPremultiplied color0, ColourbPremultiplied color1)
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

void GeometryBackgroundBorder::DrawPointPoint(Vector2f pos_outer, Vector2f pos_inner, ColourbPremultiplied color0, ColourbPremultiplied color1)
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

void GeometryBackgroundBorder::DrawArcArc(Vector2f pos_center, float R, Vector2f r, float a0, float a1, ColourbPremultiplied color0,
	ColourbPremultiplied color1, int num_points)
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
		const ColourbPremultiplied color = Math::RoundedLerp(t, color0, color1);
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

void GeometryBackgroundBorder::DrawArcPoint(Vector2f pos_center, Vector2f pos_inner, float R, float a0, float a1, ColourbPremultiplied color0,
	ColourbPremultiplied color1, int num_points)
{
	RMLUI_ASSERT(R > 0 && num_points >= 2);

	const int offset_vertices = (int)vertices.size();
	vertices.reserve(offset_vertices + num_points + 2);

	// Generate the vertices. We could also split the arc mid-way to create a sharp color transition.
	DrawPoint(pos_inner, color0);
	DrawArc(pos_center, Vector2f(R), a0, a1, color0, color1, num_points);
	DrawPoint(pos_inner, color1);

	RMLUI_ASSERT((int)vertices.size() - offset_vertices == num_points + 2);

	// Swap the last two vertices such that the outer edge vertex is last, see the comment for the border drawing functions. Their colors should
	// already be the same.
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
