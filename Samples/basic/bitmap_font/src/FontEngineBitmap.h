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

#ifndef FONTENGINEBITMAP_H
#define FONTENGINEBITMAP_H

#include "FontEngineInterfaceBitmap.h"
#include <RmlUi/Core/BaseXMLParser.h>
#include <RmlUi/Core/Texture.h>
#include <RmlUi/Core/Types.h>

class FontFaceBitmap;
using Rml::TextureSource;

namespace FontProviderBitmap {
void Initialise();
void Shutdown();
bool LoadFontFace(const String& file_name);
FontFaceBitmap* GetFontFaceHandle(const String& family, FontStyle style, FontWeight weight, int size);
} // namespace FontProviderBitmap

struct BitmapGlyph {
	int advance = 0;
	Vector2f offset = {0, 0};
	Vector2f position = {0, 0};
	Vector2f dimension = {0, 0};
};

// A mapping of characters to their glyphs.
using FontGlyphs = Rml::UnorderedMap<Character, BitmapGlyph>;

// Mapping of combined (left, right) character to kerning in pixels.
using FontKerning = Rml::UnorderedMap<uint64_t, int>;

class FontFaceBitmap {
public:
	FontFaceBitmap(String family, FontStyle style, FontWeight weight, FontMetrics metrics, String texture_name, String texture_path,
		Vector2f texture_dimensions, FontGlyphs&& glyphs, FontKerning&& kerning);

	// Get width of string.
	int GetStringWidth(StringView string, Character prior_character);

	// Generate the string geometry, returning its width.
	int GenerateString(RenderManager& render_manager, StringView string, Vector2f position, ColourbPremultiplied colour, TexturedMeshList& mesh_list);

	const FontMetrics& GetMetrics() const { return metrics; }

	const String& GetFamily() const { return family; }
	FontStyle GetStyle() const { return style; }
	FontWeight GetWeight() const { return weight; }

private:
	int GetKerning(Character left, Character right) const;

	String family;
	FontStyle style;
	FontWeight weight;

	FontMetrics metrics;

	TextureSource texture_source;
	Vector2f texture_dimensions;

	FontGlyphs glyphs;
	FontKerning kerning;
};

/*
    Parses the font meta data from an xml file.
*/

class FontParserBitmap : public Rml::BaseXMLParser {
public:
	FontParserBitmap() {}
	virtual ~FontParserBitmap();

	/// Called when the parser finds the beginning of an element tag.
	void HandleElementStart(const String& name, const Rml::XMLAttributes& attributes) override;
	/// Called when the parser finds the end of an element tag.
	void HandleElementEnd(const String& name) override;
	/// Called when the parser encounters data.
	void HandleData(const String& data, Rml::XMLDataType type) override;

	String family;
	FontStyle style = FontStyle::Normal;
	FontWeight weight = FontWeight::Normal;

	String texture_name;
	Vector2f texture_dimensions = {0, 0};

	FontMetrics metrics = {};
	FontGlyphs glyphs;
	FontKerning kerning;
};

#endif
