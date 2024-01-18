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

#ifndef FONTFACEHANDLEHARFBUZZ_H
#define FONTFACEHANDLEHARFBUZZ_H

#include "FontEngineDefault/FontTypes.h"
#include "FontFaceLayer.h"
#include "FontGlyph.h"
#include "LanguageData.h"
#include <RmlUi/Core.h>

using Rml::byte;
using Rml::Character;
using Rml::Colourb;
using Rml::FontEffect;
using Rml::FontEffectList;
using Rml::FontFaceHandleFreetype;
using Rml::FontGlyph;
using Rml::FontMetrics;
using Rml::GeometryList;
using Rml::SharedPtr;
using Rml::SmallUnorderedMap;
using Rml::String;
using Rml::TextShapingContext;
using Rml::UniquePtr;
using Rml::UnorderedMap;
using Rml::Vector;
using Rml::Vector2f;
using Rml::Vector2i;

/**
    Original author: Peter Curry
    Modified to support HarfBuzz text shaping.
 */
class FontFaceHandleHarfBuzz : public Rml::NonCopyMoveable {
public:
	FontFaceHandleHarfBuzz();
	~FontFaceHandleHarfBuzz();

	bool Initialize(FontFaceHandleFreetype face, int font_size, bool load_default_glyphs);

	const FontMetrics& GetFontMetrics() const;

	const FontGlyphMap& GetGlyphs() const;

	/// Returns the width a string will take up if rendered with this handle.
	/// @param[in] string The string to measure.
	/// @param[in] text_shaping_context Extra parameters that provide context for text shaping.
	/// @param[in] registered_languages A list of languages registered in the font engine interface.
	/// @param[in] prior_character The optionally-specified character that immediately precedes the string. This may have an impact on the string
	/// width due to kerning.
	/// @return The width, in pixels, this string will occupy if rendered with this handle.
	int GetStringWidth(const String& string, const TextShapingContext& text_shaping_context, const LanguageDataMap& registered_languages,
		Character prior_character = Character::Null);

	/// Generates, if required, the layer configuration for a given list of font effects.
	/// @param[in] font_effects The list of font effects to generate the configuration for.
	/// @return The index to use when generating geometry using this configuration.
	int GenerateLayerConfiguration(const FontEffectList& font_effects);
	/// Generates the texture data for a layer (for the texture database).
	/// @param[out] texture_data The pointer to be set to the generated texture data.
	/// @param[out] texture_dimensions The dimensions of the texture.
	/// @param[in] font_effect The font effect used for the layer.
	/// @param[in] texture_id The index of the texture within the layer to generate.
	/// @param[in] handle_version The version of the handle data. Function returns false if out of date.
	bool GenerateLayerTexture(UniquePtr<const byte[]>& texture_data, Vector2i& texture_dimensions, const FontEffect* font_effect, int texture_id,
		int handle_version) const;

	/// Generates the geometry required to render a single line of text.
	/// @param[out] geometry An array of geometries to generate the geometry into.
	/// @param[in] string The string to render.
	/// @param[in] position The position of the baseline of the first character to render.
	/// @param[in] colour The colour to render the text.
	/// @param[in] opacity The opacity of the text, should be applied to font effects.
	/// @param[in] text_shaping_context Extra parameters that provide context for text shaping.
	/// @param[in] registered_languages A list of languages registered in the font engine interface.
	/// @param[in] layer_configuration Face configuration index to use for generating string.
	/// @return The width, in pixels, of the string geometry.
	int GenerateString(GeometryList& geometry, const String& string, Vector2f position, Colourb colour, float opacity,
		const TextShapingContext& text_shaping_context, const LanguageDataMap& registered_languages, int layer_configuration = 0);

	/// Version is changed whenever the layers are dirtied, requiring regeneration of string geometry.
	int GetVersion() const;

private:
	// Build and append glyph to 'glyphs'
	bool AppendGlyph(FontGlyphIndex glyph_index);

	// Build a kerning cache for common characters.
	void FillKerningPairCache();

	// Return the kerning for a codepoint pair.
	int GetKerning(FontGlyphIndex lhs, FontGlyphIndex rhs) const;

	/// Retrieve a glyph from the given code index, building and appending a new glyph if not already built.
	/// @param[in-out] glyph_index  The glyph index, can be changed e.g. to the replacement character if no glyph is found.
	/// @return The font glyph for the returned glyph index.
	const FontGlyph* GetOrAppendGlyph(FontGlyphIndex& glyph_index);

	// Regenerate layers if dirty, such as after adding new glyphs.
	bool UpdateLayersOnDirty();

	// Create a new layer from the given font effect if it does not already exist.
	FontFaceLayer* GetOrCreateLayer(const SharedPtr<const FontEffect>& font_effect);

	// (Re-)generate a layer in this font face handle.
	bool GenerateLayer(FontFaceLayer* layer);

	// Configure internal text shaping buffer values with context.
	void ConfigureTextShapingBuffer(struct hb_buffer_t* shaping_buffer, const String& string, const TextShapingContext& text_shaping_context,
		const LanguageDataMap& registered_languages);

	FontGlyphMap glyphs;

	struct EffectLayerPair {
		const FontEffect* font_effect;
		UniquePtr<FontFaceLayer> layer;
	};
	using FontLayerMap = Vector<EffectLayerPair>;
	using FontLayerCache = SmallUnorderedMap<size_t, FontFaceLayer*>;
	using LayerConfiguration = Vector<FontFaceLayer*>;
	using LayerConfigurationList = Vector<LayerConfiguration>;

	// The list of all font layers, index by the effect that instanced them.
	FontFaceLayer* base_layer;
	FontLayerMap layers;
	// Each font layer that generated geometry or textures, indexed by the font-effect's fingerprint key.
	FontLayerCache layer_cache;

	// Pre-cache kerning pairs for some ascii subset of all characters.
	using AsciiPair = uint16_t;
	using KerningIntType = int16_t;
	using KerningPairs = UnorderedMap<AsciiPair, KerningIntType>;
	KerningPairs kerning_pair_cache;

	bool has_kerning = false;
	bool is_layers_dirty = false;
	int version = 0;

	// All configurations currently in use on this handle. New configurations will be generated as required.
	LayerConfigurationList layer_configurations;

	FontMetrics metrics;

	FontFaceHandleFreetype ft_face;
	struct hb_font_t* hb_font;
};

#endif
