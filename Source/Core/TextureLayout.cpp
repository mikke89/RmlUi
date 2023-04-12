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

#include "TextureLayout.h"
#include "TextureLayoutRectangle.h"
#include "TextureLayoutTexture.h"
#include <algorithm>

namespace Rml {

struct RectangleSort {
	bool operator()(const TextureLayoutRectangle& lhs, const TextureLayoutRectangle& rhs) const
	{
		return lhs.GetDimensions().y > rhs.GetDimensions().y;
	}
};

TextureLayout::TextureLayout() {}

TextureLayout::~TextureLayout() {}

void TextureLayout::AddRectangle(int id, Vector2i dimensions)
{
	rectangles.push_back(TextureLayoutRectangle(id, dimensions));
}

TextureLayoutRectangle& TextureLayout::GetRectangle(int index)
{
	RMLUI_ASSERT(index >= 0);
	RMLUI_ASSERT(index < GetNumRectangles());

	return rectangles[index];
}

int TextureLayout::GetNumRectangles() const
{
	return (int)rectangles.size();
}

TextureLayoutTexture& TextureLayout::GetTexture(int index)
{
	RMLUI_ASSERT(index >= 0);
	RMLUI_ASSERT(index < GetNumTextures());

	return textures[index];
}

int TextureLayout::GetNumTextures() const
{
	return (int)textures.size();
}

bool TextureLayout::GenerateLayout(int max_texture_dimensions)
{
	// Sort the rectangles by height.
	std::sort(rectangles.begin(), rectangles.end(), RectangleSort());

	int num_placed_rectangles = 0;
	while (num_placed_rectangles != GetNumRectangles())
	{
		TextureLayoutTexture texture;
		int texture_size = texture.Generate(*this, max_texture_dimensions);
		if (texture_size == 0)
			return false;

		textures.push_back(texture);
		num_placed_rectangles += texture_size;
	}

	return true;
}

} // namespace Rml
