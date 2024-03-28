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

#ifndef RMLUI_CORE_FONTEFFECT_H
#define RMLUI_CORE_FONTEFFECT_H

#include "FontGlyph.h"

namespace Rml {

/**
    @author Peter Curry
 */

class RMLUICORE_API FontEffect {
public:
	// Behind or in front of the main text.
	enum class Layer { Back, Front };

	FontEffect();
	virtual ~FontEffect();

	/// Asks the font effect if it requires, and will generate, its own unique texture. If it does
	/// not, it will share the font's base layer's textures instead.
	/// @return True if the effect generates its own textures, false if not. The default implementation returns false.
	virtual bool HasUniqueTexture() const;

	/// Requests the effect for a size and position of a single glyph's bitmap.
	/// @param[out] origin The desired origin of the effect's glyph bitmap, as a pixel offset from its original origin. This defaults to (0, 0).
	/// @param[out] dimensions The desired dimensions of the effect's glyph bitmap, in pixels. This defaults to the dimensions of the glyph's original
	/// bitmap. If the font effect is not generating a unique texture, this will be ignored.
	/// @param[in] glyph The glyph the effect is being asked to size.
	/// @return False if the effect is not providing support for the glyph, true otherwise.
	virtual bool GetGlyphMetrics(Vector2i& origin, Vector2i& dimensions, const FontGlyph& glyph) const;

	/// Requests the effect to generate the texture data for a single glyph's bitmap. The default implementation does nothing.
	/// @param[out] destination_data The top-left corner of the glyph's 32-bit, destination texture, RGBA-ordered with pre-multiplied alpha. Note that
	/// the glyph shares its texture with other glyphs.
	/// @param[in] destination_dimensions The dimensions of the glyph's area on its texture.
	/// @param[in] destination_stride The stride of the glyph's texture.
	/// @param[in] glyph The glyph the effect is being asked to generate an effect texture for.
	virtual void GenerateGlyphTexture(byte* destination_data, Vector2i destination_dimensions, int destination_stride, const FontGlyph& glyph) const;

	/// Sets the colour of the effect's geometry.
	void SetColour(Colourb colour);
	/// Returns the effect's colour.
	Colourb GetColour() const;

	Layer GetLayer() const;
	void SetLayer(Layer layer);

	/// Returns the font effect's fingerprint.
	/// @return A hash of the effect's type and properties used to generate the geometry and texture data.
	size_t GetFingerprint() const;
	void SetFingerprint(size_t fingerprint);

protected:
	// Helper function to copy the alpha value to the colour channels for each pixel, assuming RGBA-ordered bytes, resulting in a grayscale texture.
	static void FillColorValuesFromAlpha(byte* destination, Vector2i dimensions, int stride);

private:
	Layer layer;

	// The colour of the effect's geometry.
	Colourb colour;

	// A hash value identifying the properties that affected the generation of the effect's geometry and texture data.
	size_t fingerprint;
};

} // namespace Rml
#endif
