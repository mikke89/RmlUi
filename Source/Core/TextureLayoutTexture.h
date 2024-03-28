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

#ifndef TEXTURELAYOUTTEXTURE_H
#define TEXTURELAYOUTTEXTURE_H

#include "../../Include/RmlUi/Core/Texture.h"
#include "TextureLayoutRow.h"

namespace Rml {

class TextureLayout;

/**
    A texture layout texture is a single rectangular area which sub-rectangles are placed on within
    a complete texture layout.

    @author Peter
 */

class TextureLayoutTexture {
public:
	TextureLayoutTexture();
	~TextureLayoutTexture();

	/// Returns the texture's dimensions. This is only valid after the texture has been generated.
	/// @return The texture's dimensions.
	Vector2i GetDimensions() const;

	/// Attempts to position unplaced rectangles from the layout into this texture. The size of
	/// this texture will be determined by its contents.
	/// @param[in] layout The layout to position rectangles from.
	/// @param[in] maximum_dimensions The maximum dimensions of this texture. If this is not big enough to place all the rectangles, then as many will
	/// be placed as possible.
	/// @return The number of placed rectangles.
	int Generate(TextureLayout& layout, int maximum_dimensions);

	/// Allocates the texture.
	/// @return The allocated texture data.
	Vector<byte> AllocateTexture();

private:
	using RowList = Vector<TextureLayoutRow>;

	Vector2i dimensions;
	RowList rows;
};

} // namespace Rml
#endif
