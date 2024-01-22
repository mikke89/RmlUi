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

#ifndef FONTENGINEINTERFACEHARFBUZZ_H
#define FONTENGINEINTERFACEHARFBUZZ_H

#include "LanguageData.h"
#include <RmlUi/Core.h>

using Rml::byte;
using Rml::Character;
using Rml::Colourb;
using Rml::FontFaceHandle;
using Rml::FontEffectList;
using Rml::FontEffectsHandle;
using Rml::FontMetrics;
using Rml::GeometryList;
using Rml::String;
using Rml::TextShapingContext;
using Rml::Vector2f;
namespace Style = Rml::Style;

class FontEngineInterfaceHarfBuzz : public Rml::FontEngineInterface {
public:
	FontEngineInterfaceHarfBuzz();
	virtual ~FontEngineInterfaceHarfBuzz();

	/// Adds a new font face to the database. The face's family, style and weight will be determined from the face itself.
	bool LoadFontFace(const String& file_name, bool fallback_face, Style::FontWeight weight) override;

	/// Adds a new font face to the database using the provided family, style and weight.
	bool LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight,
		bool fallback_face) override;

	/// Returns a handle to a font face that can be used to position and render text. This will return the closest match
	/// it can find, but in the event a font family is requested that does not exist, NULL will be returned instead of a
	/// valid handle.
	FontFaceHandle GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size) override;

	/// Prepares for font effects by configuring a new, or returning an existing, layer configuration.
	FontEffectsHandle PrepareFontEffects(FontFaceHandle, const FontEffectList& font_effects) override;

	/// Returns the font metrics of the given font face.
	const FontMetrics& GetFontMetrics(FontFaceHandle handle) override;

	/// Returns the width a string will take up if rendered with this handle.
	int GetStringWidth(FontFaceHandle, const String& string, const TextShapingContext& text_shaping_context, Character prior_character) override;

	/// Generates the geometry required to render a single line of text.
	int GenerateString(FontFaceHandle, FontEffectsHandle, const String& string, const Vector2f& position, const Colourb& colour, float opacity,
		const TextShapingContext& text_shaping_context, GeometryList& geometry) override;

	/// Returns the current version of the font face.
	int GetVersion(FontFaceHandle handle) override;

	/// Releases resources owned by sized font faces, including their textures and rendered glyphs.
	void ReleaseFontResources() override;

	/// Registers a new language to assist with text shaping.
	void RegisterLanguage(const String& language_bcp47_code, const String& script_iso15924_code, const TextFlowDirection text_flow_direction);

private:
	LanguageDataMap registered_languages;
};

#endif
