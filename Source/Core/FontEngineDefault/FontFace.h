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

#ifndef RMLUI_CORE_FONTENGINEDEFAULT_FONTFACE_H
#define RMLUI_CORE_FONTENGINEDEFAULT_FONTFACE_H

#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "FontTypes.h"

namespace Rml {

class FontFaceHandleDefault;

/**
	@author Peter Curry
 */

class FontFace
{
public:
	FontFace(FontFaceHandleFreetype face, Style::FontStyle style, Style::FontWeight weight, UniquePtr<byte[]> face_memory);
	~FontFace();

	Style::FontStyle GetStyle() const;
	Style::FontWeight GetWeight() const;

	/// Returns a handle for positioning and rendering this face at the given size.
	/// @param[in] size The size of the desired handle, in points.
	/// @param[in] load_default_glyphs True to load the default set of glyph (ASCII range).
	/// @return The font handle.
	FontFaceHandleDefault* GetHandle(int size, bool load_default_glyphs);

private:
	Style::FontStyle style;
	Style::FontWeight weight;

	// Only filled if we own the memory used by the FreeType face handle.
	UniquePtr<byte[]> face_memory;

	// Key is font size
	using HandleMap = UnorderedMap< int, UniquePtr<FontFaceHandleDefault> >;
	HandleMap handles;

	FontFaceHandleFreetype face;
};

} // namespace Rml
#endif
