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

#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"

namespace Rml {

FontEffect::FontEffect() : colour(255, 255, 255)
{
	layer = Layer::Back;
}

FontEffect::~FontEffect()
{
}

bool FontEffect::HasUniqueTexture() const
{
	return false;
}

bool FontEffect::GetGlyphMetrics(Vector2i& RMLUI_UNUSED_PARAMETER(origin), Vector2i& RMLUI_UNUSED_PARAMETER(dimensions), const FontGlyph& RMLUI_UNUSED_PARAMETER(glyph)) const
{
	RMLUI_UNUSED(origin);
	RMLUI_UNUSED(dimensions);
	RMLUI_UNUSED(glyph);

	return false;
}

void FontEffect::GenerateGlyphTexture(byte* RMLUI_UNUSED_PARAMETER(destination_data), Vector2i RMLUI_UNUSED_PARAMETER(destination_dimensions), int RMLUI_UNUSED_PARAMETER(destination_stride), const FontGlyph& RMLUI_UNUSED_PARAMETER(glyph)) const
{
	RMLUI_UNUSED(destination_data);
	RMLUI_UNUSED(destination_dimensions);
	RMLUI_UNUSED(destination_stride);
	RMLUI_UNUSED(glyph);
}

void FontEffect::SetColour(const Colourb& _colour)
{
	colour = _colour;
}

const Colourb& FontEffect::GetColour() const
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

} // namespace Rml
