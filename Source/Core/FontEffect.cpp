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

#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"

namespace Rml {

FontEffect::FontEffect() : layer(Layer::Back), colour(255, 255, 255), fingerprint(0) {}

FontEffect::~FontEffect() {}

bool FontEffect::HasUniqueTexture() const
{
	return false;
}

bool FontEffect::GetGlyphMetrics(Vector2i& /*origin*/, Vector2i& /*dimensions*/, const FontGlyph& /*glyph*/) const
{
	return false;
}

void FontEffect::GenerateGlyphTexture(byte* /*destination_data*/, Vector2i /*destination_dimensions*/, int /*destination_stride*/,
	const FontGlyph& /*glyph*/) const
{}

void FontEffect::SetColour(const Colourb _colour)
{
	colour = _colour;
}

Colourb FontEffect::GetColour() const
{
	return colour;
}

FontEffect::Layer FontEffect::GetLayer() const
{
	return layer;
}

void FontEffect::SetLayer(Layer _layer)
{
	layer = _layer;
}

size_t FontEffect::GetFingerprint() const
{
	return fingerprint;
}

void FontEffect::SetFingerprint(size_t _fingerprint)
{
	fingerprint = _fingerprint;
}

void FontEffect::FillColorValuesFromAlpha(byte* destination, Vector2i dimensions, int stride)
{
	for (int y = 0; y < dimensions.y; ++y)
	{
		for (int x = 0; x < dimensions.x; ++x)
		{
			const int i = y * stride + x * 4;
			const byte alpha = destination[i + 3];
			destination[i + 0] = destination[i + 1] = destination[i + 2] = alpha;
		}
	}
}

} // namespace Rml
