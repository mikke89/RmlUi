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

#ifndef FONTGLYPH_H
#define FONTGLYPH_H

#include <RmlUi/Core.h>

using FontGlyphIndex = uint32_t;

struct FontGlyphData
{
	Rml::FontGlyph bitmap;
	Rml::Character character;
};

struct FontGlyphReference
{
	const Rml::FontGlyph* bitmap;
	Rml::Character character;
};

struct FontClusterGlyphData
{
	FontGlyphIndex glyph_index;
	FontGlyphData glyph_data;
};

using FontGlyphMap = Rml::UnorderedMap<FontGlyphIndex, FontGlyphData>;
using FallbackFontGlyphMap = Rml::UnorderedMap<Rml::Character, Rml::FontGlyph>;
using FallbackFontClusterGlyphMap = Rml::UnorderedMap<Rml::String, Rml::Vector<FontClusterGlyphData>>;
using FallbackFontClusterGlyphLookupMap = Rml::UnorderedMap<uint64_t, const Rml::FontGlyph*>;

struct FontGlyphMaps {
	const FontGlyphMap* glyphs;
	const FallbackFontGlyphMap* fallback_glyphs;
	const FallbackFontClusterGlyphLookupMap* fallback_cluster_glyphs;
};

inline uint64_t GetFallbackFontClusterGlyphLookupID(FontGlyphIndex glyph_index, Rml::Character character)
{
	// Combine 32-bit glyph index and 32-bit character into a single 64-bit integer.
	return (static_cast<uint64_t>(glyph_index) << (sizeof(Rml::Character) * 8)) | static_cast<uint64_t>(character);
}

#endif
