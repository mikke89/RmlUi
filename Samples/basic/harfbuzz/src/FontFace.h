#pragma once

#include "FontEngineDefault/FontTypes.h"
#include "FontFaceHandleHarfBuzz.h"
#include <RmlUi/Core.h>

using Rml::FontFaceHandleFreetype;
using Rml::UniquePtr;
using Rml::UnorderedMap;
namespace Style = Rml::Style;

/**
    Modified to support HarfBuzz text shaping.
 */

class FontFace {
public:
	FontFace(FontFaceHandleFreetype face, Style::FontStyle style, Style::FontWeight weight);
	~FontFace();

	Style::FontStyle GetStyle() const;
	Style::FontWeight GetWeight() const;

	/// Returns a handle for positioning and rendering this face at the given size.
	/// @param[in] size The size of the desired handle, in points.
	/// @param[in] load_default_glyphs True to load the default set of glyph (ASCII range).
	/// @return The font handle.
	FontFaceHandleHarfBuzz* GetHandle(int size, bool load_default_glyphs);

	/// Releases resources owned by sized font faces, including their textures and rendered glyphs.
	void ReleaseFontResources();

private:
	Style::FontStyle style;
	Style::FontWeight weight;

	// Key is font size
	using HandleMap = UnorderedMap<int, UniquePtr<FontFaceHandleHarfBuzz>>;
	HandleMap handles;

	FontFaceHandleFreetype face;
};
