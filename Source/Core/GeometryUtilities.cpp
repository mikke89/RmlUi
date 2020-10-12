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

#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "GeometryBackgroundBorder.h"

namespace Rml {

GeometryUtilities::GeometryUtilities()
{
}

GeometryUtilities::~GeometryUtilities()
{
}

// Generates a quad from a position, size and colour.
void GeometryUtilities::GenerateQuad(Vertex* vertices, int* indices, Vector2f origin, Vector2f dimensions, Colourb colour, int index_offset)
{
	GenerateQuad(vertices, indices, origin, dimensions, colour, Vector2f(0, 0), Vector2f(1, 1), index_offset);
}

// Generates a quad from a position, size, colour and texture coordinates.
void GeometryUtilities::GenerateQuad(Vertex* vertices, int* indices, Vector2f origin, Vector2f dimensions, Colourb colour, Vector2f top_left_texcoord, Vector2f bottom_right_texcoord, int index_offset)
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

// Generates the geometry required to render a line above, below or through a line of text.
void GeometryUtilities::GenerateLine(FontFaceHandle font_face_handle, Geometry* geometry, Vector2f position, int width, Style::TextDecoration height, Colourb colour)
{
	Vector< Vertex >& line_vertices = geometry->GetVertices();
	Vector< int >& line_indices = geometry->GetIndices();
	float underline_thickness = 0;
	float underline_position = GetFontEngineInterface()->GetUnderline(font_face_handle, underline_thickness);
	int size = GetFontEngineInterface()->GetSize(font_face_handle);
	int x_height = GetFontEngineInterface()->GetXHeight(font_face_handle);

	float offset;
	switch (height)
	{
		case Style::TextDecoration::Underline:       offset = -underline_position; break;
		case Style::TextDecoration::Overline:        offset = -underline_position - (float)size; break;
		case Style::TextDecoration::LineThrough:     offset = -0.65f * (float)x_height; break;
		default: return;
	}

	line_vertices.resize(line_vertices.size() + 4);
	line_indices.resize(line_indices.size() + 6);
	GeometryUtilities::GenerateQuad(
									&line_vertices[0] + ((int)line_vertices.size() - 4),
									&line_indices[0] + ((int)line_indices.size() - 6),
									Vector2f(position.x, position.y + offset).Round(),
									Vector2f((float) width, underline_thickness),
									colour,
									(int)line_vertices.size() - 4
									);
}

void GeometryUtilities::GenerateBackgroundBorder(Geometry* geometry, const Box& box, Vector2f offset, Vector4f border_radius, Colourb background_colour, const Colourb* border_colours)
{
	Vector<Vertex>& vertices = geometry->GetVertices();
	Vector<int>& indices = geometry->GetIndices();

	CornerSizes corner_sizes{ border_radius.x, border_radius.y, border_radius.z, border_radius.w };
	GeometryBackgroundBorder::Draw(vertices, indices, corner_sizes, box, offset, background_colour, border_colours);
}

} // namespace Rml
