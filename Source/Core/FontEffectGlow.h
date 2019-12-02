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

#ifndef RMLUICOREFONTEFFECTGLOW_H
#define RMLUICOREFONTEFFECTGLOW_H

#include "../../Include/RmlUi/Core/ConvolutionFilter.h"
#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontEffectInstancer.h"

namespace Rml {
namespace Core {

/**
	A font effect for rendering glow around text.

	Glow consists of an outline pass followed by a Gaussian blur pass.

 */

class FontEffectGlow : public FontEffect
{
public:
	FontEffectGlow();
	virtual ~FontEffectGlow();

	bool Initialise(int width_outline, int width_blur);

	bool HasUniqueTexture() const override;

	bool GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& glyph) const override;

	/// Expands the original glyph texture for the outline.
	/// @param[out] destination_data The top-left corner of the glyph's 32-bit, RGBA-ordered, destination texture. Note that they glyph shares its texture with other glyphs.
	/// @param[in] destination_dimensions The dimensions of the glyph's area on its texture.
	/// @param[in] destination_stride The stride of the glyph's texture.
	/// @param[in] glyph The glyph the effect is being asked to generate an effect texture for.
	void GenerateGlyphTexture(byte* destination_data, const Vector2i& destination_dimensions, int destination_stride, const FontGlyph& glyph) const override;

private:
	int width_outline, width_blur, combined_width;
	ConvolutionFilter filter_outline, filter_blur;
};



/**
	A concrete font effect instancer for the glow effect.
 */

class FontEffectGlowInstancer : public FontEffectInstancer
{
public:
	FontEffectGlowInstancer();
	virtual ~FontEffectGlowInstancer();

	SharedPtr<FontEffect> InstanceFontEffect(const String& name, const PropertyDictionary& properties) override;

private:
	PropertyId id_width_outline, id_width_blur, id_color;
};


}
}

#endif
