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

#ifndef RMLUI_CORE_FONTGLYPH_H
#define RMLUI_CORE_FONTGLYPH_H

#include "Types.h"

namespace Rml {

/**
    Metrics and bitmap data for a single glyph within a font face.

    @author Peter Curry
 */

class RMLUICORE_API FontGlyph {
public:
	/// The distance from the cursor (positioned vertically on the baseline) to the top-left corner of this glyph's bitmap.
	Vector2i bearing;
	/// The glyph's advance; this is how far the cursor will be moved along after rendering this character.
	int advance = 0;

	/// Bitmap data defining this glyph. The dimensions and format of the data is given below. This will be nullptr if the glyph has no bitmap data.
	const byte* bitmap_data = nullptr;

	Vector2i bitmap_dimensions;
	ColorFormat color_format = ColorFormat::A8;

	// bitmap_data may point to this member or another font glyph data.
	UniquePtr<byte[]> bitmap_owned_data;

	// Create a copy with its bitmap data owned by another glyph.
	FontGlyph WeakCopy() const
	{
		FontGlyph glyph;
		glyph.bearing = bearing;
		glyph.advance = advance;
		glyph.bitmap_data = bitmap_data;
		glyph.bitmap_dimensions = bitmap_dimensions;
		glyph.color_format = color_format;
		return glyph;
	}
};

using FontGlyphMap = UnorderedMap<Character, FontGlyph>;

} // namespace Rml
#endif
