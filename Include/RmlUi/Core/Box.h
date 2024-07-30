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

#ifndef RMLUI_CORE_BOX_H
#define RMLUI_CORE_BOX_H

#include "Types.h"

namespace Rml {

enum class BoxEdge { Top, Right, Bottom, Left };
enum class BoxDirection { Vertical, Horizontal };

/**
    Stores a box with four sized areas; content, padding, a border and margin. See
    http://www.w3.org/TR/REC-CSS2/box.html#box-dimensions for a diagram.

    @author Peter Curry
 */

class RMLUICORE_API Box {
public:
	static constexpr int num_areas = 3; // ignores content box
	static constexpr int num_edges = 4;

	/// Initialises a zero-sized box.
	Box();
	/// Initialises a box with a default content area and no padding, borders and margins.
	explicit Box(Vector2f content);
	~Box();

	/// Returns the top-left position of one of the box's areas, relative to the top-left of the border area. This
	/// means the position of the margin area is likely to be negative.
	/// @param area[in] The desired area.
	/// @return The position of the area.
	Vector2f GetPosition(BoxArea area = BoxArea::Content) const;
	/// Returns the size of the box's content area.
	/// @return The size of the content area.
	Vector2f GetSize() const;
	/// Returns the size of one of the box's areas. This will include all inner areas.
	/// @param area[in] The desired area.
	/// @return The size of the requested area.
	Vector2f GetSize(BoxArea area) const;

	/// Sets the size of the content area.
	/// @param content[in] The size of the new content area.
	void SetContent(Vector2f content);
	/// Sets the size of one of the edges of one of the box's outer areas.
	/// @param area[in] The area to change.
	/// @param edge[in] The area edge to change.
	/// @param size[in] The new size of the area segment.
	void SetEdge(BoxArea area, BoxEdge edge, float size);

	/// Returns the size of one of the area edges.
	/// @param area[in] The desired area.
	/// @param edge[in] The desired edge.
	/// @return The size of the requested area edge.
	float GetEdge(BoxArea area, BoxEdge edge) const;
	/// Returns the cumulative size of one edge up to one of the box's areas.
	/// @param area[in] The area to measure up to (and including). So, Margin will return the width of the margin, and Padding will be the sum of the
	/// margin, border and padding.
	/// @param edge[in] The desired edge.
	/// @return The cumulative size of the edge.
	float GetCumulativeEdge(BoxArea area, BoxEdge edge) const;

	/// Returns the size along a single direction of the given 'area', including all inner areas up-to and including 'area_end'.
	/// @param direction The desired direction.
	/// @param area The widest area to include.
	/// @param area_end The last area to include, anything inside this is excluded.
	/// @example GetSizeAcross(Horizontal, Border, Padding) returns the total width of the horizontal borders and paddings.
	float GetSizeAcross(BoxDirection direction, BoxArea area, BoxArea area_end = BoxArea::Content) const;

	/// Returns the size of the frame defined by the given area, not including inner areas.
	/// @param area The area to use.
	Vector2f GetFrameSize(BoxArea area) const;

	/// Compares the size of the content area and the other area edges.
	/// @return True if the boxes represent the same area.
	bool operator==(const Box& rhs) const;
	/// Compares the size of the content area and the other area edges.
	/// @return True if the boxes do not represent the same area.
	bool operator!=(const Box& rhs) const;

private:
	Vector2f content;
	float area_edges[num_areas][num_edges] = {};
};

} // namespace Rml
#endif
