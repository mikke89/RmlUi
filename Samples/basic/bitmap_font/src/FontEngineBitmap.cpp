#include "FontEngineBitmap.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/StreamMemory.h>
#include <cstdio>

namespace FontProviderBitmap {
static Rml::Vector<Rml::UniquePtr<FontFaceBitmap>> fonts;

void Initialise() {}

void Shutdown()
{
	fonts.clear();
}

bool LoadFontFace(const String& file_name)
{
	// Load the xml meta file into memory
	Rml::UniquePtr<byte[]> data;
	size_t length = 0;

	{
		auto file_interface = Rml::GetFileInterface();
		auto handle = file_interface->Open(file_name);
		if (!handle)
			return false;

		length = file_interface->Length(handle);

		data.reset(new byte[length]);

		size_t read_length = file_interface->Read(data.get(), length, handle);

		file_interface->Close(handle);

		if (read_length != length || !data)
			return false;
	}

	// Parse the xml font description
	FontParserBitmap parser;

	{
		auto stream = Rml::MakeUnique<Rml::StreamMemory>(data.get(), length);
		stream->SetSourceURL(file_name);

		parser.Parse(stream.get());

		if (parser.family.empty() || parser.glyphs.empty() || parser.texture_name.empty() || parser.metrics.size == 0)
			return false;

		// Fill the remaining metrics
		parser.metrics.underline_position = 3.f;
		parser.metrics.underline_thickness = 1.f;
	}

	// Construct and add the font face
	fonts.push_back(Rml::MakeUnique<FontFaceBitmap>(parser.family, parser.style, parser.weight, parser.metrics, parser.texture_name, file_name,
		parser.texture_dimensions, std::move(parser.glyphs), std::move(parser.kerning)));

	return true;
}

FontFaceBitmap* GetFontFaceHandle(const String& family, FontStyle style, FontWeight weight, int size)
{
	FontFaceBitmap* best_match = nullptr;
	int best_score = 0;

	// Normally, we'd want to only match the font family exactly, but for this demo we create a very lenient heuristic.
	for (const auto& font : fonts)
	{
		int score = 1;
		if (font->GetFamily() == family)
			score += 100;

		score += 10 - std::min(10, std::abs(font->GetMetrics().size - size));

		if (font->GetStyle() == style)
			score += 2;
		if (font->GetWeight() == weight)
			score += 1;

		if (score > best_score)
		{
			best_match = font.get();
			best_score = score;
		}
	}

	return best_match;
}

} // namespace FontProviderBitmap

FontFaceBitmap::FontFaceBitmap(String family, FontStyle style, FontWeight weight, FontMetrics metrics, String texture_name, String texture_path,
	Vector2f texture_dimensions, FontGlyphs&& glyphs, FontKerning&& kerning) :
	family(family), style(style), weight(weight), metrics(metrics), texture_source(texture_name, texture_path),
	texture_dimensions(texture_dimensions), glyphs(std::move(glyphs)), kerning(std::move(kerning))
{}

int FontFaceBitmap::GetStringWidth(StringView string, Character previous_character)
{
	int width = 0;

	for (auto it_char = Rml::StringIteratorU8(string); it_char; ++it_char)
	{
		Character character = *it_char;

		auto it_glyph = glyphs.find(character);
		if (it_glyph == glyphs.end())
			continue;

		const BitmapGlyph& glyph = it_glyph->second;

		int kerning = GetKerning(previous_character, character);

		width += glyph.advance + kerning;
		previous_character = character;
	}

	return width;
}

int FontFaceBitmap::GenerateString(RenderManager& render_manager, StringView string, Vector2f string_position, ColourbPremultiplied colour,
	TexturedMeshList& mesh_list)
{
	int width = 0;

	mesh_list.resize(1);

	mesh_list[0].texture = texture_source.GetTexture(render_manager);

	Rml::Mesh& mesh = mesh_list[0].mesh;
	auto& vertices = mesh.vertices;
	auto& indices = mesh.indices;

	vertices.reserve(string.size() * 4);
	indices.reserve(string.size() * 6);

	Vector2f position = string_position.Round();
	Character previous_character = Character::Null;

	for (auto it_char = Rml::StringIteratorU8(string); it_char; ++it_char)
	{
		Character character = *it_char;

		auto it_glyph = glyphs.find(character);
		if (it_glyph == glyphs.end())
			continue;

		int kerning = GetKerning(previous_character, character);

		width += kerning;
		position.x += kerning;

		const BitmapGlyph& glyph = it_glyph->second;

		// Generate the geometry for the character.
		vertices.resize(vertices.size() + 4);
		indices.resize(indices.size() + 6);

		Vector2f uv_top_left = glyph.position / texture_dimensions;
		Vector2f uv_bottom_right = (glyph.position + glyph.dimension) / texture_dimensions;

		Rml::MeshUtilities::GenerateQuad(mesh, Vector2f(position + glyph.offset).Round(), glyph.dimension, colour, uv_top_left, uv_bottom_right);

		width += glyph.advance;
		position.x += glyph.advance;

		previous_character = character;
	}

	return width;
}

int FontFaceBitmap::GetKerning(Character left, Character right) const
{
	uint64_t key = (((uint64_t)left << 32) | (uint64_t)right);

	auto it = kerning.find(key);
	if (it != kerning.end())
		return it->second;

	return 0;
}

FontParserBitmap::~FontParserBitmap() {}

void FontParserBitmap::HandleElementStart(const String& name, const Rml::XMLAttributes& attributes)
{
	if (name == "info")
	{
		family = Rml::StringUtilities::ToLower(Get(attributes, "face", String()));
		metrics.size = Get(attributes, "size", 0);

		style = Get(attributes, "italic", 0) == 1 ? FontStyle::Italic : FontStyle::Normal;
		weight = Get(attributes, "bold", 0) == 1 ? FontWeight::Bold : FontWeight::Normal;
	}
	else if (name == "common")
	{
		metrics.line_spacing = Get(attributes, "lineHeight", 0.f);
		metrics.ascent = Get(attributes, "base", 0.f);
		metrics.descent = metrics.line_spacing - metrics.ascent;

		texture_dimensions.x = Get(attributes, "scaleW", 0.f);
		texture_dimensions.y = Get(attributes, "scaleH", 0.f);
	}
	else if (name == "page")
	{
		int id = Get(attributes, "id", -1);
		if (id != 0)
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Only single font textures are supported in Bitmap Font Engine");
			return;
		}
		texture_name = Get(attributes, "file", String());
	}
	else if (name == "char")
	{
		Character character = (Character)Get(attributes, "id", 0);
		if (character == Character::Null)
			return;

		BitmapGlyph& glyph = glyphs[character];

		glyph.offset.x = Get(attributes, "xoffset", 0.f);
		glyph.offset.y = Get(attributes, "yoffset", 0.f) - metrics.ascent; // Shift y-origin from top to baseline

		glyph.advance = Get(attributes, "xadvance", 0);
		glyph.position.x = Get(attributes, "x", 0.f);
		glyph.position.y = Get(attributes, "y", 0.f);
		glyph.dimension.x = Get(attributes, "width", 0.f);
		glyph.dimension.y = Get(attributes, "height", 0.f);

		if (character == (Character)'x')
			metrics.x_height = glyph.dimension.y;
		if (character == (Character)0x2026)
			metrics.has_ellipsis = true;
	}
	else if (name == "kerning")
	{
		uint64_t first = (uint64_t)Get(attributes, "first", 0);
		uint64_t second = (uint64_t)Get(attributes, "second", 0);
		int amount = Get(attributes, "amount", 0);

		if (first != 0 && second != 0 && amount != 0)
		{
			uint64_t key = ((first << 32) | second);
			kerning[key] = amount;
		}
	}
}

void FontParserBitmap::HandleElementEnd(const String& /*name*/) {}

void FontParserBitmap::HandleData(const String& /*data*/, Rml::XMLDataType /*type*/) {}
