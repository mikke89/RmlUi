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

#include <cstdio>
#include <RmlUi/Core.h>
#include <RmlUi/Core/StreamMemory.h>
#include "FontEngineBitmap.h"

namespace FontProviderBitmap
{
	static Rml::Vector<Rml::UniquePtr<FontFaceBitmap>> fonts;


	void Initialise()
	{
	}

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
			parser.metrics.underline_position = -parser.metrics.baseline - 1.f;
			parser.metrics.underline_thickness = 1.f;
		}

		Texture texture;
		texture.Set(parser.texture_name, file_name);

		// Construct and add the font face
		fonts.push_back(
			Rml::MakeUnique<FontFaceBitmap>( 
				parser.family, parser.style, parser.weight, parser.metrics, texture, parser.texture_dimensions, std::move(parser.glyphs), std::move(parser.kerning)
		));

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

}


FontFaceBitmap::FontFaceBitmap(String family, FontStyle style, FontWeight weight, FontMetrics metrics, Texture texture, Vector2f texture_dimensions, FontGlyphs&& glyphs, FontKerning&& kerning)
	: family(family), style(style), weight(weight), metrics(metrics), texture(texture), texture_dimensions(texture_dimensions), glyphs(std::move(glyphs)), kerning(std::move(kerning)) 
{}

int FontFaceBitmap::GetStringWidth(const String& string, Character previous_character)
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

int FontFaceBitmap::GenerateString(const String& string, const Vector2f& string_position, const Colourb& colour, GeometryList& geometry_list)
{
	int width = 0;

	geometry_list.resize(1);
	Rml::Geometry& geometry = geometry_list[0];

	geometry.SetTexture(&texture);

	auto& vertices = geometry.GetVertices();
	auto& indices = geometry.GetIndices();

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

		Rml::GeometryUtilities::GenerateQuad(
			&vertices[0] + (vertices.size() - 4),
			&indices[0] + (indices.size() - 6),
			Vector2f(position + glyph.offset).Round(),
			glyph.dimension,
			colour,
			uv_top_left,
			uv_bottom_right,
			(int)vertices.size() - 4
		);

		width += glyph.advance;
		position.x += glyph.advance;

		previous_character = character;
	}

	return width;
}

int FontFaceBitmap::GetKerning(Character left, Character right) const
{
	std::uint64_t key = (((std::uint64_t)left << 32) | (std::uint64_t)right);

	auto it = kerning.find(key);
	if (it != kerning.end())
		return it->second;

	return 0;
}





FontParserBitmap::~FontParserBitmap()
{
}

// Called when the parser finds the beginning of an element tag.
void FontParserBitmap::HandleElementStart(const String& name, const Rml::XMLAttributes& attributes)
{
	if (name == "info")
	{
		family = Rml::StringUtilities::ToLower( Get(attributes, "face", String()) );
		metrics.size = Get(attributes, "size", 0);

		style = Get(attributes, "italic", 0) == 1 ? FontStyle::Italic : FontStyle::Normal;
		weight = Get(attributes, "bold", 0) == 1 ? FontWeight::Bold : FontWeight::Normal;
	}
	else if (name == "common")
	{
		metrics.line_height = Get(attributes, "lineHeight", 0);
		metrics.baseline = metrics.line_height - Get(attributes, "base", 0);

		texture_dimensions.x = Get(attributes, "scaleW", 0.f);
		texture_dimensions.y = Get(attributes, "scaleH", 0.f);
	}
	else if (name == "page")
	{
		int id = Get(attributes, "id", -1);
		if(id != 0)
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

		glyph.advance = Get(attributes, "xadvance", 0);

		// Shift y-origin from top to baseline
		float origin_shift_y = float(metrics.baseline - metrics.line_height);

		glyph.offset.x = Get(attributes, "xoffset", 0.f);
		glyph.offset.y = Get(attributes, "yoffset", 0.f) + origin_shift_y;
		
		glyph.position.x = Get(attributes, "x", 0.f);
		glyph.position.y = Get(attributes, "y", 0.f);
		glyph.dimension.x = Get(attributes, "width", 0.f);
		glyph.dimension.y = Get(attributes, "height", 0.f);

		if (character == (Character)'x')
			metrics.x_height = (int)glyph.dimension.y;
	}
	else if (name == "kerning")
	{
		std::uint64_t first = (std::uint64_t)Get(attributes, "first", 0);
		std::uint64_t second = (std::uint64_t)Get(attributes, "second", 0);
		int amount = Get(attributes, "amount", 0);

		if (first != 0 && second != 0 && amount != 0)
		{
			std::uint64_t key = ((first << 32) | second);
			kerning[key] = amount;
		}
	}
}

// Called when the parser finds the end of an element tag.
void FontParserBitmap::HandleElementEnd(const String& RMLUI_UNUSED_PARAMETER(name))
{
	RMLUI_UNUSED(name);
}

// Called when the parser encounters data.
void FontParserBitmap::HandleData(const String& RMLUI_UNUSED_PARAMETER(data), Rml::XMLDataType RMLUI_UNUSED_PARAMETER(type))
{
	RMLUI_UNUSED(data);
	RMLUI_UNUSED(type);
}
