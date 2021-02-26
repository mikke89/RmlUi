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

#include "Sprite.h"
#include <RmlUi/Core/Math.h>
#include <ShellOpenGL.h>

Sprite::Sprite(const Rml::Vector2f& dimensions, const Rml::Vector2f& top_left_texcoord, const Rml::Vector2f& bottom_right_texcoord) : dimensions(dimensions), top_left_texcoord(top_left_texcoord), bottom_right_texcoord(bottom_right_texcoord)
{
}

Sprite::~Sprite()
{
}

void Sprite::Render(Rml::Vector2f position, const float dp_ratio)
{
	position = (dp_ratio * position);
	Rml::Vector2f dimensions_px = dp_ratio * dimensions;

	Rml::Math::SnapToPixelGrid(position, dimensions_px);

	glTexCoord2f(top_left_texcoord.x, top_left_texcoord.y);
	glVertex2f(position.x, position.y);

	glTexCoord2f(top_left_texcoord.x, bottom_right_texcoord.y);
	glVertex2f(position.x, position.y + dimensions_px.y);

	glTexCoord2f(bottom_right_texcoord.x, bottom_right_texcoord.y);
	glVertex2f(position.x + dimensions_px.x, position.y + dimensions_px.y);

	glTexCoord2f(bottom_right_texcoord.x, top_left_texcoord.y);
	glVertex2f(position.x + dimensions_px.x, position.y);
}
