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

#ifndef RMLUICOREFONTEFFECTOUTLINE_H
#define RMLUICOREFONTEFFECTOUTLINE_H

#include "../../Include/RmlUi/Core/ConvolutionFilter.h"
#include "../../Include/RmlUi/Core/FontEffect.h"

namespace Rml {
namespace Core {

class ConvolutionFilter;

/**
	A concrete font effect for rendering outlines around text.

	@author Peter Curry
 */

class FontEffectOutline : public FontEffect
{
public:
	FontEffectOutline();
	virtual ~FontEffectOutline();

	/// Initialise the outline effect.
	/// @param[in] width The width of the effect. This must be greater than zero.
	/// @return True if the effect initialised successfully, false if not.
	bool Initialise(int width);

	/// Returns true.
	/// @return True.
	bool HasUniqueTexture() const override;

	/// Resizes and repositions the glyph to fit the outline.
	/// @param[out] origin The desired origin of the effect's glyph bitmap, as a pixel offset from its original origin. This defaults to (0, 0).
	/// @param[out] dimensions The desired dimensions of the effect's glyph bitmap, in pixels. This defaults to the dimensions of the glyph's original bitmap.
	/// @param[in] glyph The glyph the effect is being asked to size.
	/// @return False if the effect is not providing support for the glyph, true otherwise.
	bool GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& glyph) const override;

	/// Expands the original glyph texture for the outline.
	/// @param[out] destination_data The top-left corner of the glyph's 32-bit, RGBA-ordered, destination texture. Note that they glyph shares its texture with other glyphs.
	/// @param[in] destination_dimensions The dimensions of the glyph's area on its texture.
	/// @param[in] destination_stride The stride of the glyph's texture.
	/// @param[in] glyph The glyph the effect is being asked to generate an effect texture for.
	void GenerateGlyphTexture(byte* destination_data, const Vector2i& destination_dimensions, int destination_stride, const FontGlyph& glyph) const override;

private:
	int width;
	ConvolutionFilter filter;
};

}
}

#endif
