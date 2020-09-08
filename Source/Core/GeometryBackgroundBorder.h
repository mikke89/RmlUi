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

#ifndef RMLUI_CORE_GEOMETRYBACKGROUNDBORDER_H
#define RMLUI_CORE_GEOMETRYBACKGROUNDBORDER_H

#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/Vertex.h"

namespace Rml {

class Box;

// Ordered by top, right, bottom, left.
using EdgeSizes = Array<float, 4>;

// Ordered by top-left, top-right, bottom-right, bottom-left.
using CornerSizes = Array<float, 4>;
using CornerSizes2 = Array<Vector2f, 4>;
using CornerPositions = Array<Vector2f, 4>;


class GeometryBackgroundBorder {
public:

	/// Generate geometry for background and borders.
	/// @param[out] vertices Destination vector for generated vertices.
	/// @param[out] indices Destination vector for generated indices.
	/// @param[in] radii The radius of each corner.
	/// @param[in] box The box used for positioning and sizing of the background and borders.
	/// @param[in] offset Offset the position of the generated vertices.
	/// @param[in] background_color Color of the background, set alpha to zero to not generate a background.
	/// @param[in] border_colors Pointer to a four-element array of border colors in top-right-bottom-left order, or nullptr to not generate borders.
	static void Draw(Vector<Vertex>& vertices, Vector<int>& indices, CornerSizes radii, const Box& box, Vector2f offset, Colourb background_color, const Colourb* border_colors);

private:
	enum Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT };

	GeometryBackgroundBorder(Vector<Vertex>& vertices, Vector<int>& indices);

	// -- Background --
	// All draw operations place vertices in clockwise order.

	// Draw the corner, delegate to the specific corner shape drawing function.
	void DrawBackgroundCorner(Corner corner, Vector2f pos_inner, Vector2f pos_circle_center, float R, Vector2f r, Colourb color);

	// Add a single point.
	void DrawPoint(Vector2f pos, Colourb color);

	// Draw an arc by placing vertices along the ellipse formed by the two-axis radius r, spaced evenly between angles a0,a1 (inclusive). Colors are interpolated.
	void DrawArc(Vector2f pos_center, Vector2f r, float a0, float a1, Colourb color0, Colourb color1, int num_points);

	// Generates triangles by connecting the added vertices.
	void FillBackground(int index_start);


	// -- Border --
	// All draw operations place the first and last vertices in the following manner.
	// Let N be the number of vertices placed, and the numbers be indices into the newly placed vertices:
	//   0: Inner edge, aligned with the previous corner
	//   1: Outer edge, aligned with the previous corner
	//   N-2: Inner edge, aligned with the next corner
	//   N-1: Outer edge, aligned with the next corner
	// Where 'next' corner means along the clockwise direction. This way we can easily fill the triangles of the edges in FillEdge().

	// Draw the corner, delegate to the specific corner shape drawing function.
	void DrawBorderCorner(Corner corner, Vector2f pos_outer, Vector2f pos_inner, Vector2f pos_circle_center, float R, Vector2f r, Colourb color0, Colourb color1);

	// Draw a sharp border corner, ie. no border-radius. Does not produce any triangles.
	void DrawPointPoint(Vector2f pos_outer, Vector2f pos_inner, Colourb color0, Colourb color1);

	// Draw an arc along the outer edge (radius R), and an arc along the inner edge (two-axis radius r),
	// spaced evenly between angles a0,a1 (inclusive). Connect them by triangles. Colors are interpolated.
	void DrawArcArc(Vector2f pos_center, float R, Vector2f r, float a0, float a1, Colourb color0, Colourb color1, int num_points);

	// Draw an arc along the outer edge, and connect them by triangles to a point on the inner edge.
	void DrawArcPoint(Vector2f pos_center, Vector2f pos_inner, float R, float a0, float a1, Colourb color0, Colourb color1, int num_points);

	// Add triangles between the previous corner to another one specified by the index (possibly yet-to-be-drawn).
	void FillEdge(int index_next_corner);


	// -- Tools --
	int GetNumPoints(float R) const;

	Vector<Vertex>& vertices;
	Vector<int>& indices;
};

} // namespace Rml
#endif
