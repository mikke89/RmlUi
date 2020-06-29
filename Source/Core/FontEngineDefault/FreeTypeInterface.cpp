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

#include "FreeTypeInterface.h"
#include "../../../Include/RmlUi/Core/Log.h"

#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace Rml {

static FT_Library ft_library = nullptr;


static bool BuildGlyph(FT_Face ft_face, Character character, FontGlyphMap& glyphs);
static void BuildGlyphMap(FT_Face ft_face, int size, FontGlyphMap& glyphs);
static void GenerateMetrics(FT_Face ft_face, FontMetrics& metrics);


bool FreeType::Initialise()
{
	RMLUI_ASSERT(!ft_library);

	FT_Error result = FT_Init_FreeType(&ft_library);
	if (result != 0)
	{
		Log::Message(Log::LT_ERROR, "Failed to initialise FreeType, error %d.", result);
		Shutdown();
		return false;
	}

	return true;
}

void FreeType::Shutdown()
{
	if (ft_library != nullptr)
	{
		FT_Done_FreeType(ft_library);
		ft_library = nullptr;
	}
}

// Loads a FreeType face from memory.
FontFaceHandleFreetype FreeType::LoadFace(const byte* data, int data_length, const String& source)
{
	RMLUI_ASSERT(ft_library);

	FT_Face face = nullptr;
	int error = FT_New_Memory_Face(ft_library, (const FT_Byte*)data, data_length, 0, &face);
	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "FreeType error %d while loading face from %s.", error, source.c_str());
		return 0;
	}

	// Initialise the character mapping on the face.
	if (face->charmap == nullptr)
	{
		FT_Select_Charmap(face, FT_ENCODING_APPLE_ROMAN);
		if (face->charmap == nullptr)
		{
			Log::Message(Log::LT_ERROR, "Font face (from %s) does not contain a Unicode or Apple Roman character map.", source.c_str());
			FT_Done_Face(face);
			return 0;
		}
	}

	return (FontFaceHandleFreetype)face;
}

bool FreeType::ReleaseFace(FontFaceHandleFreetype in_face, bool release_stream)
{
	FT_Face face = (FT_Face)in_face;

	FT_Byte* face_memory = face->stream->base;
	FT_Error error = FT_Done_Face(face);

	if (release_stream)
		delete[] face_memory;

	return (error == 0);
}

void FreeType::GetFaceStyle(FontFaceHandleFreetype in_face, String& font_family, Style::FontStyle& style, Style::FontWeight& weight)
{
	FT_Face face = (FT_Face)in_face;

	font_family = face->family_name;
	style = face->style_flags & FT_STYLE_FLAG_ITALIC ? Style::FontStyle::Italic : Style::FontStyle::Normal;
	weight = face->style_flags & FT_STYLE_FLAG_BOLD ? Style::FontWeight::Bold : Style::FontWeight::Normal;
}



// Initialises the handle so it is able to render text.
bool FreeType::InitialiseFaceHandle(FontFaceHandleFreetype face, int font_size, FontGlyphMap& glyphs, FontMetrics& metrics)
{
	FT_Face ft_face = (FT_Face)face;

	metrics.size = font_size;

	// Set the character size on the font face.
	FT_Error error = FT_Set_Char_Size(ft_face, 0, font_size << 6, 0, 0);
	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "Unable to set the character size '%d' on the font face '%s %s'.", font_size, ft_face->family_name, ft_face->style_name);
		return false;
	}

	// Construct the initial list of glyphs.
	BuildGlyphMap(ft_face, font_size, glyphs);

	// Generate the metrics for the handle.
	GenerateMetrics(ft_face, metrics);

	return true;
}

bool FreeType::AppendGlyph(FontFaceHandleFreetype face, int font_size, Character character, FontGlyphMap& glyphs)
{
	FT_Face ft_face = (FT_Face)face;

	RMLUI_ASSERT(glyphs.find(character) == glyphs.end());
	RMLUI_ASSERT(ft_face);

	// Set face size again in case it was used at another size in another font face handle.
	FT_Error error = FT_Set_Char_Size(ft_face, 0, font_size << 6, 0, 0);
	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "Unable to set the character size '%d' on the font face '%s %s'.", font_size, ft_face->family_name, ft_face->style_name);
		return false;
	}

	if (!BuildGlyph(ft_face, character, glyphs))
		return false;

	return true;
}


int FreeType::GetKerning(FontFaceHandleFreetype face, int font_size, Character lhs, Character rhs)
{
	FT_Face ft_face = (FT_Face)face;

	if (!FT_HAS_KERNING(ft_face))
		return 0;

	// Set face size again in case it was used at another size in another font face handle.
	FT_Error ft_error = FT_Set_Char_Size(ft_face, 0, font_size << 6, 0, 0);
	if (ft_error)
		return 0;

	FT_Vector ft_kerning;

	ft_error = FT_Get_Kerning(
		ft_face,
		FT_Get_Char_Index(ft_face, (FT_ULong)lhs),
		FT_Get_Char_Index(ft_face, (FT_ULong)rhs),
		FT_KERNING_DEFAULT,
		&ft_kerning
	);

	if (ft_error)
		return 0;

	int kerning = ft_kerning.x >> 6;
	return kerning;
}



static void BuildGlyphMap(FT_Face ft_face, int size, FontGlyphMap& glyphs)
{
	glyphs.reserve(128);

	// Add the ASCII characters now. Other characters are added later as needed.
	FT_ULong code_min = 32;
	FT_ULong code_max = 126;

	for (FT_ULong character_code = code_min; character_code <= code_max; ++character_code)
		BuildGlyph(ft_face, (Character)character_code, glyphs);

	// Add a replacement character for rendering unknown characters.
	Character replacement_character = Character::Replacement;
	auto it = glyphs.find(replacement_character);
	if (it == glyphs.end())
	{
		FontGlyph glyph;
		glyph.dimensions = { size / 3, (size * 2) / 3 };
		glyph.bitmap_dimensions = glyph.dimensions;
		glyph.advance = glyph.dimensions.x + 2;
		glyph.bearing = { 1, glyph.dimensions.y };

		glyph.bitmap_owned_data.reset(new byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y]);
		glyph.bitmap_data = glyph.bitmap_owned_data.get();

		for (int y = 0; y < glyph.bitmap_dimensions.y; y++)
		{
			for (int x = 0; x < glyph.bitmap_dimensions.x; x++)
			{
				constexpr int stroke = 1;
				int i = y * glyph.bitmap_dimensions.x + x;
				bool near_edge = (x < stroke || x >= glyph.bitmap_dimensions.x - stroke || y < stroke || y >= glyph.bitmap_dimensions.y - stroke);
				glyph.bitmap_owned_data[i] = (near_edge ? 0xdd : 0);
			}
		}

		glyphs[replacement_character] = std::move(glyph);
	}
}

static bool BuildGlyph(FT_Face ft_face, Character character, FontGlyphMap& glyphs)
{
	int index = FT_Get_Char_Index(ft_face, (FT_ULong)character);
	if (index == 0)
		return false;

	FT_Error error = FT_Load_Glyph(ft_face, index, 0);
	if (error != 0)
	{
		Log::Message(Log::LT_WARNING, "Unable to load glyph for character '%u' on the font face '%s %s'; error code: %d.", character, ft_face->family_name, ft_face->style_name, error);
		return false;
	}

	error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
	if (error != 0)
	{
		Log::Message(Log::LT_WARNING, "Unable to render glyph for character '%u' on the font face '%s %s'; error code: %d.", character, ft_face->family_name, ft_face->style_name, error);
		return false;
	}

	auto result = glyphs.emplace(character, FontGlyph{});
	if (!result.second)
	{
		Log::Message(Log::LT_WARNING, "Glyph character '%u' is already loaded in the font face '%s %s'.", character, ft_face->family_name, ft_face->style_name);
		return false;
	}

	FontGlyph& glyph = result.first->second;

	FT_GlyphSlot ft_glyph = ft_face->glyph;

	// Set the glyph's dimensions.
	glyph.dimensions.x = ft_glyph->metrics.width >> 6;
	glyph.dimensions.y = ft_glyph->metrics.height >> 6;

	// Set the glyph's bearing.
	glyph.bearing.x = ft_glyph->metrics.horiBearingX >> 6;
	glyph.bearing.y = ft_glyph->metrics.horiBearingY >> 6;

	// Set the glyph's advance.
	glyph.advance = ft_glyph->metrics.horiAdvance >> 6;

	// Set the glyph's bitmap dimensions.
	glyph.bitmap_dimensions.x = ft_glyph->bitmap.width;
	glyph.bitmap_dimensions.y = ft_glyph->bitmap.rows;

	// Copy the glyph's bitmap data from the FreeType glyph handle to our glyph handle.
	if (glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y != 0)
	{
		// Check the pixel mode is supported.
		if (ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO &&
			ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
		{
			glyph.bitmap_owned_data.reset();
			glyph.bitmap_data = nullptr;
			Log::Message(Log::LT_WARNING, "Unable to render glyph on the font face '%s %s'; unsupported pixel mode (%d).", ft_glyph->face->family_name, ft_glyph->face->style_name, ft_glyph->bitmap.pixel_mode);
		}
		else
		{
			glyph.bitmap_owned_data.reset(new byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y]);
			glyph.bitmap_data = glyph.bitmap_owned_data.get();

			const byte* source_bitmap = ft_glyph->bitmap.buffer;
			byte* destination_bitmap = glyph.bitmap_owned_data.get();

			// Copy the bitmap data into the newly-allocated space on our glyph.
			switch (ft_glyph->bitmap.pixel_mode)
			{
				// Unpack 1-bit data into 8-bit.
			case FT_PIXEL_MODE_MONO:
			{
				for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
				{
					int mask = 0x80;
					const byte* source_byte = source_bitmap;
					for (int j = 0; j < glyph.bitmap_dimensions.x; ++j)
					{
						if ((*source_byte & mask) == mask)
							destination_bitmap[j] = 255;
						else
							destination_bitmap[j] = 0;

						mask >>= 1;
						if (mask <= 0)
						{
							mask = 0x80;
							++source_byte;
						}
					}

					destination_bitmap += glyph.bitmap_dimensions.x;
					source_bitmap += ft_glyph->bitmap.pitch;
				}
			}
			break;

			// Directly copy 8-bit data.
			case FT_PIXEL_MODE_GRAY:
			{
				for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
				{
					memcpy(destination_bitmap, source_bitmap, glyph.bitmap_dimensions.x);
					destination_bitmap += glyph.bitmap_dimensions.x;
					source_bitmap += ft_glyph->bitmap.pitch;
				}
			}
			break;
			}
		}
	}
	else
	{
		glyph.bitmap_owned_data.reset();
		glyph.bitmap_data = nullptr;
	}

	return true;
}

static void GenerateMetrics(FT_Face ft_face, FontMetrics& metrics)
{
	metrics.line_height = ft_face->size->metrics.height >> 6;
	metrics.baseline = metrics.line_height - (ft_face->size->metrics.ascender >> 6);

	metrics.underline_position = FT_MulFix(ft_face->underline_position, ft_face->size->metrics.y_scale) / float(1 << 6);
	metrics.underline_thickness = FT_MulFix(ft_face->underline_thickness, ft_face->size->metrics.y_scale) / float(1 << 6);
	metrics.underline_thickness = Math::Max(metrics.underline_thickness, 1.0f);

	// Determine the x-height of this font face.
	int index = FT_Get_Char_Index(ft_face, 'x');
	if (FT_Load_Glyph(ft_face, index, 0) == 0)
		metrics.x_height = ft_face->glyph->metrics.height >> 6;
	else
		metrics.x_height = 0;
}


} // namespace Rml
