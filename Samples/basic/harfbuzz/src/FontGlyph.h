#pragma once

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
using FallbackFontClusterGlyphsMap = Rml::UnorderedMap<Rml::String, Rml::Vector<FontClusterGlyphData>>;
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
