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

#include "Geometry.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"

namespace Rml {
namespace Debugger {

static Context* context;

Geometry::Geometry()
{
}

void Geometry::SetContext(Context* _context)
{
	context = _context;
}

// Renders a one-pixel rectangular outline.
void Geometry::RenderOutline(const Vector2f& origin, const Vector2f& dimensions, const Colourb& colour, float width)
{
	if (context == nullptr)
		return;

	RenderInterface* render_interface = context->GetRenderInterface();

	Vertex vertices[4 * 4];
	int indices[6 * 4];

	GeometryUtilities::GenerateQuad(vertices + 0, indices + 0, Vector2f(0, 0), Vector2f(dimensions.x, width), colour, 0);
	GeometryUtilities::GenerateQuad(vertices + 4, indices + 6, Vector2f(0, dimensions.y - width), Vector2f(dimensions.x, width), colour, 4);
	GeometryUtilities::GenerateQuad(vertices + 8, indices + 12, Vector2f(0, 0), Vector2f(width, dimensions.y), colour, 8);
	GeometryUtilities::GenerateQuad(vertices + 12, indices + 18, Vector2f(dimensions.x - width, 0), Vector2f(width, dimensions.y), colour, 12);

	render_interface->RenderGeometry(vertices, 4 * 4, indices, 6 * 4, 0, origin);
}

// Renders a box.
void Geometry::RenderBox(const Vector2f& origin, const Vector2f& dimensions, const Colourb& colour)
{
	if (context == nullptr)
		return;

	RenderInterface* render_interface = context->GetRenderInterface();

	Vertex vertices[4];
	int indices[6];

	GeometryUtilities::GenerateQuad(vertices, indices, Vector2f(0, 0), Vector2f(dimensions.x, dimensions.y), colour, 0);

	render_interface->RenderGeometry(vertices, 4, indices, 6, 0, origin);
}

// Renders a box with a hole in the middle.
void Geometry::RenderBox(const Vector2f& origin, const Vector2f& dimensions, const Vector2f& hole_origin, const Vector2f& hole_dimensions, const Colourb& colour)
{
	// Top box.
	float top_y_dimensions = hole_origin.y - origin.y;
	if (top_y_dimensions > 0)
	{
		RenderBox(origin, Vector2f(dimensions.x, top_y_dimensions), colour);
	}

	// Bottom box.
	float bottom_y_dimensions = (origin.y + dimensions.y) - (hole_origin.y + hole_dimensions.y);
	if (bottom_y_dimensions > 0)
	{
		RenderBox(Vector2f(origin.x, hole_origin.y + hole_dimensions.y), Vector2f(dimensions.x, bottom_y_dimensions), colour);
	}

	// Left box.
	float left_x_dimensions = hole_origin.x - origin.x;
	if (left_x_dimensions > 0)
	{
		RenderBox(Vector2f(origin.x, hole_origin.y), Vector2f(left_x_dimensions, hole_dimensions.y), colour);
	}

	// Right box.
	float right_x_dimensions = (origin.x + dimensions.x) - (hole_origin.x + hole_dimensions.x);
	if (right_x_dimensions > 0)
	{
		RenderBox(Vector2f(hole_origin.x + hole_dimensions.x, hole_origin.y), Vector2f(right_x_dimensions, hole_dimensions.y), colour);
	}
}

}
}
