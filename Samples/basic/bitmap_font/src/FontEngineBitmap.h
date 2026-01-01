#pragma once

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
