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

#include "TextureLayoutRectangle.h"

namespace Rml {

TextureLayoutRectangle::TextureLayoutRectangle(const int _id, const Vector2i dimensions) : dimensions(dimensions), texture_position(0, 0)
{
	id = _id;
	texture_index = -1;

	texture_data = nullptr;
	texture_stride = 0;
}

TextureLayoutRectangle::~TextureLayoutRectangle() {}

int TextureLayoutRectangle::GetId() const
{
	return id;
}

Vector2i TextureLayoutRectangle::GetPosition() const
{
	return texture_position;
}

Vector2i TextureLayoutRectangle::GetDimensions() const
{
	return dimensions;
}

void TextureLayoutRectangle::Place(const int _texture_index, const Vector2i position)
{
	texture_index = _texture_index;
	texture_position = position;
}

void TextureLayoutRectangle::Unplace()
{
	texture_index = -1;
}

bool TextureLayoutRectangle::IsPlaced() const
{
	return texture_index > -1;
}

void TextureLayoutRectangle::Allocate(byte* _texture_data, int _texture_stride)
{
	texture_data = _texture_data + ((texture_position.y * _texture_stride) + texture_position.x * 4);
	texture_stride = _texture_stride;
}

int TextureLayoutRectangle::GetTextureIndex()
{
	return texture_index;
}

byte* TextureLayoutRectangle::GetTextureData()
{
	return texture_data;
}

int TextureLayoutRectangle::GetTextureStride() const
{
	return texture_stride;
}

} // namespace Rml
