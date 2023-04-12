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

#include "../../Include/RmlUi/Core/Box.h"
#include <string.h>

namespace Rml {

Box::Box() {}
Box::Box(Vector2f content) : content(content) {}

Box::~Box() {}

Vector2f Box::GetPosition(Area area) const
{
	Vector2f area_position(-area_edges[MARGIN][LEFT], -area_edges[MARGIN][TOP]);
	for (int i = 0; i < area; i++)
	{
		area_position.x += area_edges[i][LEFT];
		area_position.y += area_edges[i][TOP];
	}

	return area_position;
}

Vector2f Box::GetSize() const
{
	return content;
}

Vector2f Box::GetSize(Area area) const
{
	Vector2f area_size(content);
	for (int i = area; i <= PADDING; i++)
	{
		area_size.x += (area_edges[i][LEFT] + area_edges[i][RIGHT]);
		area_size.y += (area_edges[i][TOP] + area_edges[i][BOTTOM]);
	}

	return area_size;
}

void Box::SetContent(Vector2f _content)
{
	content = _content;
}

void Box::SetEdge(Area area, Edge edge, float size)
{
	area_edges[area][edge] = size;
}

float Box::GetEdge(Area area, Edge edge) const
{
	return area_edges[area][edge];
}

float Box::GetCumulativeEdge(Area area, Edge edge) const
{
	float size = 0;
	int max_area = Math::Min((int)area, (int)PADDING);
	for (int i = 0; i <= max_area; i++)
		size += area_edges[i][edge];

	return size;
}

float Box::GetSizeAcross(Direction direction, Area area_outer, Area area_inner) const
{
	static_assert(HORIZONTAL == 1 && VERTICAL == 0, "");
	RMLUI_ASSERT(area_outer <= area_inner && direction <= 1);

	float size = 0.0f;

	if (area_inner == CONTENT)
		size = (direction == HORIZONTAL ? content.x : content.y);

	for (int i = area_outer; i <= area_inner && i < CONTENT; i++)
		size += (area_edges[i][TOP + (int)direction] + area_edges[i][BOTTOM + (int)direction]);

	return size;
}

Vector2f Box::GetFrameSize(Area area) const
{
	if (area == CONTENT)
		return content;

	return {
		area_edges[area][RIGHT] + area_edges[area][LEFT],
		area_edges[area][TOP] + area_edges[area][BOTTOM],
	};
}

// Compares the size of the content area and the other area edges.
bool Box::operator==(const Box& rhs) const
{
	return content == rhs.content && memcmp(area_edges, rhs.area_edges, sizeof(area_edges)) == 0;
}

// Compares the size of the content area and the other area edges.
bool Box::operator!=(const Box& rhs) const
{
	return !(*this == rhs);
}

} // namespace Rml
