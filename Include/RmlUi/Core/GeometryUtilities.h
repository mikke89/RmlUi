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

#ifndef RMLUI_CORE_GEOMETRYUTILITIES_H
#define RMLUI_CORE_GEOMETRYUTILITIES_H

#include "Header.h"
#include "Types.h"
#include "Vertex.h"
#include "StyleTypes.h"

namespace Rml {

class Box;
class Geometry;

/**
	A class containing helper functions for rendering geometry.

	@author Robert Curry
 */

class RMLUICORE_API GeometryUtilities
{
public:
	/// Generates a quad from a position, size and colour.
	/// @param[out] vertices An array of at least four vertices that the generated vertex data will be written into.
	/// @param[out] indices An array of at least six indices that the generated index data will be written into.
	/// @param[in] origin The origin of the quad to generate.
	/// @param[in] dimensions The dimensions of the quad to generate.
	/// @param[in] colour The colour to be assigned to each of the quad's vertices.
	/// @param[in] index_offset The offset to be added to the generated indices; this should be the number of vertices already in the array.
	static void GenerateQuad(Vertex* vertices, int* indices, Vector2f origin, Vector2f dimensions, Colourb colour, int index_offset = 0);
	/// Generates a quad from a position, size, colour and texture coordinates.
	/// @param[out] vertices An array of at least four vertices that the generated vertex data will be written into.
	/// @param[out] indices An array of at least six indices that the generated index data will be written into.
	/// @param[in] origin The origin of the quad to generate.
	/// @param[in] dimensions The dimensions of the quad to generate.
	/// @param[in] colour The colour to be assigned to each of the quad's vertices.
	/// @param[in] top_left_texcoord The texture coordinates at the top-left of the quad.
	/// @param[in] bottom_right_texcoord The texture coordinates at the bottom-right of the quad.
	/// @param[in] index_offset The offset to be added to the generated indices; this should be the number of vertices already in the array.
	static void GenerateQuad(Vertex* vertices, int* indices, Vector2f origin, Vector2f dimensions, Colourb colour, Vector2f top_left_texcoord, Vector2f bottom_right_texcoord, int index_offset = 0);

	/// Generates the geometry required to render a line above, below or through a line of text.
	/// @param[in] font_face_handle The font face handle to derive the line's metrics from.
	/// @param[out] geometry The geometry to append the newly created geometry into.
	/// @param[in] position The position of the baseline of the lined text.
	/// @param[in] width The width of the string to line.
	/// @param[in] decoration_type The type for vertical positioning of line.
	/// @param[in] colour The colour to draw the line in.
	static void GenerateLine(FontFaceHandle font_face_handle, Geometry* geometry, Vector2f position, int width, Style::TextDecoration decoration_type, Colourb colour);

	/// Generates a geometry in the same way as element backgrounds and borders are generated, with support for the border-radius property.
	/// Vertex positions are relative to the border-box, vertex texture coordinates are default initialized.
	/// @param[out] geometry The geometry to append the newly created vertices and indices into.
	/// @param[in] box The box which determines the background and border geometry.
	/// @param[in] offset Offset the position of the generated vertices.
	/// @param[in] border_radius The border radius in pixel units in the following order: top-left, top-right, bottom-right, bottom-left.
	/// @param[in] background_colour The colour applied to the background, set alpha to zero to not generate the background.
	/// @param[in] border_colours Pointer to a four-element array of border colors in top-right-bottom-left order, or nullptr to not generate borders.
	static void GenerateBackgroundBorder(Geometry* geometry, const Box& box, Vector2f offset, Vector4f border_radius, Colourb background_colour, const Colourb* border_colours = nullptr);

private:
	GeometryUtilities();
	~GeometryUtilities();
};

} // namespace Rml
#endif
