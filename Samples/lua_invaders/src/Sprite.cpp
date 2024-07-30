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

#include "Sprite.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/RenderManager.h>

Sprite::Sprite(const Rml::Vector2f& dimensions, const Rml::Vector2f& top_left_texcoord, const Rml::Vector2f& bottom_right_texcoord) :
	dimensions(dimensions), top_left_texcoord(top_left_texcoord), bottom_right_texcoord(bottom_right_texcoord)
{}

Sprite::~Sprite() {}

void Sprite::Render(Rml::RenderManager& render_manager, Rml::Vector2f position, const float dp_ratio, Rml::ColourbPremultiplied color,
	Rml::Texture texture)
{
	position = dp_ratio * position;
	Rml::Vector2f dimensions_px = dp_ratio * dimensions;
	Rml::Math::SnapToPixelGrid(position, dimensions_px);

	Rml::Mesh mesh;
	Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(0.f), dimensions_px, color, top_left_texcoord, bottom_right_texcoord);

	Rml::Geometry geometry = render_manager.MakeGeometry(std::move(mesh));
	geometry.Render(position, texture);
}

void DrawPoints(Rml::RenderManager& render_manager, float point_size, const ColoredPointList& points)
{
	constexpr int num_quad_vertices = 4;
	constexpr int num_quad_indices = 6;

	const int num_points = (int)points.size();

	Rml::Mesh mesh;
	mesh.vertices.reserve(num_points * num_quad_vertices);
	mesh.indices.reserve(num_points * num_quad_indices);

	for (const ColoredPoint& point : points)
	{
		Rml::Vector2f position = point.position;
		Rml::Vector2f size = Rml::Vector2f(point_size);
		Rml::MeshUtilities::GenerateQuad(mesh, position, size, point.color);
	}

	Rml::Geometry geometry = render_manager.MakeGeometry(std::move(mesh));
	geometry.Render(Rml::Vector2f(0.f));
}
