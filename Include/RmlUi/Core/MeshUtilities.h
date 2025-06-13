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

#ifndef RMLUI_CORE_MESHUTILITIES_H
#define RMLUI_CORE_MESHUTILITIES_H

#include "Header.h"
#include "RenderBox.h"
#include "Types.h"

namespace Rml {

struct Mesh;

/**
    A class containing helper functions for generating meshes.

 */
class RMLUICORE_API MeshUtilities {
public:
	/// Generates a quad from a position, size and color.
	/// @param[out] mesh A mesh to append the generated vertices and indices into.
	/// @param[in] origin The origin of the quad to generate.
	/// @param[in] dimensions The dimensions of the quad to generate.
	/// @param[in] color The color to be assigned to each of the quad's vertices.
	static void GenerateQuad(Mesh& mesh, Vector2f origin, Vector2f dimensions, ColourbPremultiplied color);
	/// Generates a quad from a position, size, color and texture coordinates.
	/// @param[out] mesh A mesh to append the generated vertices and indices into.
	/// @param[in] origin The origin of the quad to generate.
	/// @param[in] dimensions The dimensions of the quad to generate.
	/// @param[in] color The color to be assigned to each of the quad's vertices.
	/// @param[in] top_left_texcoord The texture coordinates at the top-left of the quad.
	/// @param[in] bottom_right_texcoord The texture coordinates at the bottom-right of the quad.
	static void GenerateQuad(Mesh& mesh, Vector2f origin, Vector2f dimensions, ColourbPremultiplied color, Vector2f top_left_texcoord,
		Vector2f bottom_right_texcoord);

	/// Generates the geometry required to render a line.
	/// @param[out] mesh A mesh to append the generated vertices and indices into.
	/// @param[in] position The top-left position the line.
	/// @param[in] size The size of the line.
	/// @param[in] color The color to draw the line in.
	static void GenerateLine(Mesh& mesh, Vector2f position, Vector2f size, ColourbPremultiplied color);

	/// Generates the geometry for an element's background and border, with support for border-radius.
	/// @param[out] mesh A mesh to append the generated vertices and indices into.
	/// @param[in] render_box The render box which determines the background and border geometry.
	/// @param[in] background_color The color applied to the background, set alpha to zero to not generate the background.
	/// @param[in] border_colors A four-element array of border colors in top-right-bottom-left order.
	/// @note Vertex positions are relative to the border-box, vertex texture coordinates are default initialized.
	static void GenerateBackgroundBorder(Mesh& mesh, const RenderBox& render_box, ColourbPremultiplied background_color,
		const ColourbPremultiplied border_colors[4]);

	/// Generates the background geometry for an element's area, with support for border-radius.
	/// @param[out] mesh A mesh to append the generated vertices and indices into.
	/// @param[in] render_box The render box which determines the background geometry.
	/// @param[in] color The color applied to the background.
	/// @note Vertex positions are relative to the border-box, vertex texture coordinates are default initialized.
	static void GenerateBackground(Mesh& mesh, const RenderBox& render_box, ColourbPremultiplied color);

private:
	MeshUtilities() = delete;
};

} // namespace Rml
#endif
