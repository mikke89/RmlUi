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

#include "FreeTypeInterface.h"
#include <ft2build.h>
#include <limits.h>
#include <string.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_TRUETYPE_TABLES_H

namespace FreeType
{
static bool BuildGlyph(FT_Face ft_face, const FontGlyphIndex glyph_index, FontGlyphMap& glyphs, const float bitmap_scaling_factor);
static void BuildGlyphMap(FT_Face ft_face, int size, FontGlyphMap& glyphs, const float bitmap_scaling_factor, const bool load_default_glyphs);
static void GenerateMetrics(FT_Face ft_face, FontMetrics& metrics, float bitmap_scaling_factor);
static bool SetFontSize(FT_Face ft_face, int font_size, float& out_bitmap_scaling_factor);
static void BitmapDownscale(Rml::byte* bitmap_new, const int new_width, const int new_height, const Rml::byte* bitmap_source, const int width,
	const int height,
	const int pitch, const Rml::ColorFormat color_format);

bool InitialiseFaceHandle(FontFaceHandleFreetype face, int font_size, FontGlyphMap& glyphs, FontMetrics& metrics, bool load_default_glyphs)
{
	FT_Face ft_face = (FT_Face)face;

	metrics.size = font_size;

	float bitmap_scaling_factor = 1.0f;
	if (!SetFontSize(ft_face, font_size, bitmap_scaling_factor))
		return false;

	// Construct the initial list of glyphs.
	BuildGlyphMap(ft_face, font_size, glyphs, bitmap_scaling_factor, load_default_glyphs);

	// Generate the metrics for the handle.
	GenerateMetrics(ft_face, metrics, bitmap_scaling_factor);

	return true;
}

bool AppendGlyph(FontFaceHandleFreetype face, int font_size, FontGlyphIndex glyph_index, FontGlyphMap& glyphs)
{
	FT_Face ft_face = (FT_Face)face;

	RMLUI_ASSERT(glyphs.find(glyph_index) == glyphs.end());
	RMLUI_ASSERT(ft_face);

	// Set face size again in case it was used at another size in another font face handle.
	float bitmap_scaling_factor = 1.0f;
	if (!SetFontSize(ft_face, font_size, bitmap_scaling_factor))
		return false;

	if (!BuildGlyph(ft_face, glyph_index, glyphs, bitmap_scaling_factor))
		return false;

	return true;
}

int GetKerning(FontFaceHandleFreetype face, int font_size, FontGlyphIndex lhs, FontGlyphIndex rhs)
{
	FT_Face ft_face = (FT_Face)face;

	RMLUI_ASSERT(FT_HAS_KERNING(ft_face));

	// Set face size again in case it was used at another size in another font face handle.
	// Font size value of zero assumes it is already set.
	if (font_size > 0)
	{
		float bitmap_scaling_factor = 1.0f;
		if (!SetFontSize(ft_face, font_size, bitmap_scaling_factor) || bitmap_scaling_factor != 1.0f)
			return 0;
	}

	FT_Vector ft_kerning;

	FT_Error ft_error = FT_Get_Kerning(ft_face, lhs, rhs,FT_KERNING_DEFAULT, &ft_kerning);

	if (ft_error)
		return 0;

	int kerning = ft_kerning.x >> 6;
	return kerning;
}

FontGlyphIndex GetGlyphIndexFromCharacter(FontFaceHandleFreetype face, Character character)
{
	return FT_Get_Char_Index((FT_Face)face, (FT_ULong)character);
}

static bool BuildGlyph(FT_Face ft_face, const FontGlyphIndex glyph_index, FontGlyphMap& glyphs,
	const float bitmap_scaling_factor)
{
	if (glyph_index == 0)
		return false;

	FT_Error error = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_COLOR);
	if (error != 0)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Unable to load glyph at index '%u' on the font face '%s %s'; error code: %d.",
			(unsigned int)glyph_index,
			ft_face->family_name, ft_face->style_name, error);
		return false;
	}

	error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
	if (error != 0)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Unable to render glyph at index '%u' on the font face '%s %s'; error code: %d.",
			(unsigned int)glyph_index,
			ft_face->family_name, ft_face->style_name, error);
		return false;
	}

	auto result = glyphs.emplace(glyph_index, Rml::FontGlyph{});
	if (!result.second)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Glyph index '%u' is already loaded in the font face '%s %s'.", (unsigned int)glyph_index,
			ft_face->family_name, ft_face->style_name);
		return false;
	}

	Rml::FontGlyph& glyph = result.first->second;

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

	// Determine new metrics if we need to scale the bitmap received from FreeType. Only allow bitmap downscaling.
	const bool scale_bitmap = (bitmap_scaling_factor < 1.f);
	if (scale_bitmap)
	{
		glyph.dimensions = Rml::Vector2i(Rml::Vector2f(glyph.dimensions) * bitmap_scaling_factor);
		glyph.bearing = Rml::Vector2i(Rml::Vector2f(glyph.bearing) * bitmap_scaling_factor);
		glyph.advance = int(float(glyph.advance) * bitmap_scaling_factor);
		glyph.bitmap_dimensions = Rml::Vector2i(Rml::Vector2f(glyph.bitmap_dimensions) * bitmap_scaling_factor);
	}

	// Copy the glyph's bitmap data from the FreeType glyph handle to our glyph handle.
	if (glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y != 0)
	{
		// Check if the pixel mode is supported.
		if (ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO && ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY &&
			ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA)
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Unable to render glyph on the font face '%s %s': unsupported pixel mode (%d).",
				ft_glyph->face->family_name, ft_glyph->face->style_name, ft_glyph->bitmap.pixel_mode);
		}
		else if (ft_glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO && scale_bitmap)
		{
			Rml::Log::Message(Rml::Log::LT_WARNING, "Unable to render glyph on the font face '%s %s': bitmap scaling unsupported in mono pixel mode.",
				ft_glyph->face->family_name, ft_glyph->face->style_name);
		}
		else
		{
			const int num_bytes_per_pixel = (ft_glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? 4 : 1);
			glyph.color_format = (ft_glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? Rml::ColorFormat::RGBA8 : Rml::ColorFormat::A8);

			glyph.bitmap_owned_data.reset(new Rml::byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y * num_bytes_per_pixel]);
			glyph.bitmap_data = glyph.bitmap_owned_data.get();
			Rml::byte* destination_bitmap = glyph.bitmap_owned_data.get();
			const Rml::byte* source_bitmap = ft_glyph->bitmap.buffer;

			// Copy the bitmap data into the newly-allocated space on our glyph.
			switch (ft_glyph->bitmap.pixel_mode)
			{
			case FT_PIXEL_MODE_MONO:
			{
				// Unpack 1-bit data into 8-bit.
				for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
				{
					int mask = 0x80;
					const Rml::byte* source_byte = source_bitmap;
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
			case FT_PIXEL_MODE_GRAY:
			case FT_PIXEL_MODE_BGRA:
			{
				if (scale_bitmap)
				{
					// Resize the glyph data to the new dimensions.
					BitmapDownscale(destination_bitmap, glyph.bitmap_dimensions.x, glyph.bitmap_dimensions.y, source_bitmap,
						(int)ft_glyph->bitmap.width, (int)ft_glyph->bitmap.rows, ft_glyph->bitmap.pitch, glyph.color_format);
				}
				else
				{
					// Copy the glyph data directly.
					const int num_bytes_per_line = glyph.bitmap_dimensions.x * num_bytes_per_pixel;
					for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
					{
						memcpy(destination_bitmap, source_bitmap, num_bytes_per_line);
						destination_bitmap += num_bytes_per_line;
						source_bitmap += ft_glyph->bitmap.pitch;
					}
				}

				if (glyph.color_format == Rml::ColorFormat::RGBA8)
				{
					// Swizzle channels (BGRA -> RGBA) and un-premultiply alpha.
					destination_bitmap = glyph.bitmap_owned_data.get();

					for (int k = 0; k < glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y * num_bytes_per_pixel; k += 4)
					{
						Rml::byte b = destination_bitmap[k];
						Rml::byte g = destination_bitmap[k + 1];
						Rml::byte r = destination_bitmap[k + 2];
						const Rml::byte alpha = destination_bitmap[k + 3];
						RMLUI_ASSERTMSG(b <= alpha && g <= alpha && r <= alpha, "Assumption of glyph data being premultiplied is broken.");

						if (alpha > 0 && alpha < 255)
						{
							b = Rml::byte((b * 255) / alpha);
							g = Rml::byte((g * 255) / alpha);
							r = Rml::byte((r * 255) / alpha);
						}

						destination_bitmap[k] = r;
						destination_bitmap[k + 1] = g;
						destination_bitmap[k + 2] = b;
						destination_bitmap[k + 3] = alpha;
					}
				}
			}
			break;
			}
		}
	}

	return true;
}

static void BuildGlyphMap(FT_Face ft_face, int size, FontGlyphMap& glyphs, const float bitmap_scaling_factor, const bool load_default_glyphs)
{
	if (load_default_glyphs)
	{
		glyphs.reserve(128);

		// Add the ASCII characters now. Other characters are added later as needed.
		FT_ULong code_min = 32;
		FT_ULong code_max = 126;

		for (FT_ULong character_code = code_min; character_code <= code_max; ++character_code)
		{
			FT_UInt index = FT_Get_Char_Index(ft_face, character_code);
			BuildGlyph(ft_face, index, glyphs, bitmap_scaling_factor);
		}
	}

	// Add a replacement character for rendering unknown characters.
	FontGlyphIndex replacement_glyph_index = FT_Get_Char_Index(ft_face, (FT_ULong)Rml::Character::Replacement);
	auto it = glyphs.find(replacement_glyph_index);
	if (it == glyphs.end())
	{
		Rml::FontGlyph glyph;
		glyph.dimensions = {size / 3, (size * 2) / 3};
		glyph.bitmap_dimensions = glyph.dimensions;
		glyph.advance = glyph.dimensions.x + 2;
		glyph.bearing = {1, glyph.dimensions.y};

		glyph.bitmap_owned_data.reset(new Rml::byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y]);
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

		glyphs[replacement_glyph_index] = std::move(glyph);
	}
}

static void GenerateMetrics(FT_Face ft_face, FontMetrics& metrics, float bitmap_scaling_factor)
{
	metrics.ascent = ft_face->size->metrics.ascender * bitmap_scaling_factor / float(1 << 6);
	metrics.descent = -ft_face->size->metrics.descender * bitmap_scaling_factor / float(1 << 6);
	metrics.line_spacing = ft_face->size->metrics.height * bitmap_scaling_factor / float(1 << 6);

	metrics.underline_position = FT_MulFix(-ft_face->underline_position, ft_face->size->metrics.y_scale) * bitmap_scaling_factor / float(1 << 6);
	metrics.underline_thickness = FT_MulFix(ft_face->underline_thickness, ft_face->size->metrics.y_scale) * bitmap_scaling_factor / float(1 << 6);
	metrics.underline_thickness = Rml::Math::Max(metrics.underline_thickness, 1.0f);

	// Determine the x-height of this font face.
	FT_UInt index = FT_Get_Char_Index(ft_face, 'x');
	if (index != 0 && FT_Load_Glyph(ft_face, index, 0) == 0)
		metrics.x_height = ft_face->glyph->metrics.height * bitmap_scaling_factor / float(1 << 6);
	else
		metrics.x_height = 0.5f * metrics.line_spacing;
}

static bool SetFontSize(FT_Face ft_face, int font_size, float& out_bitmap_scaling_factor)
{
	RMLUI_ASSERT(out_bitmap_scaling_factor == 1.f);

	FT_Error error = 0;

	// Set the character size on the font face.
	error = FT_Set_Char_Size(ft_face, 0, font_size << 6, 0, 0);

	// If setting char size fails, try to select a bitmap strike instead when available.
	if (error != 0 && FT_HAS_FIXED_SIZES(ft_face))
	{
		constexpr int a_big_number = INT_MAX / 2;
		int heuristic_min = INT_MAX;
		int index_min = -1;

		// Select the bitmap strike with the smallest size *above* font_size, or else the largest size.
		for (int i = 0; i < ft_face->num_fixed_sizes; i++)
		{
			const int size_diff = ft_face->available_sizes[i].height - font_size;
			const int heuristic = (size_diff < 0 ? a_big_number - size_diff : size_diff);

			if (heuristic < heuristic_min)
			{
				index_min = i;
				heuristic_min = heuristic;
			}
		}

		if (index_min >= 0)
		{
			out_bitmap_scaling_factor = float(font_size) / ft_face->available_sizes[index_min].height;

			// Add some tolerance to the scaling factor to avoid unnecessary scaling. Only allow downscaling.
			constexpr float bitmap_scaling_factor_threshold = 0.95f;
			if (out_bitmap_scaling_factor >= bitmap_scaling_factor_threshold)
				out_bitmap_scaling_factor = 1.f;

			error = FT_Select_Size(ft_face, index_min);
		}
	}

	if (error != 0)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Unable to set the character size '%d' on the font face '%s %s'.", font_size, ft_face->family_name,
			ft_face->style_name);
		return false;
	}

	return true;
}

static void BitmapDownscale(Rml::byte* bitmap_new, const int new_width, const int new_height, const Rml::byte* bitmap_source, const int width,
	const int height, const int pitch, const Rml::ColorFormat color_format)
{
	// Average filter for downscaling bitmap images, based on https://stackoverflow.com/a/9571580
	constexpr int max_num_channels = 4;
	const int num_channels = (color_format == Rml::ColorFormat::RGBA8 ? 4 : 1);

	const float xscale = float(new_width) / width;
	const float yscale = float(new_height) / height;
	const float sumscale = xscale * yscale;

	float yend = 0.0f;
	for (int f = 0; f < new_height; f++) // y on output
	{
		const float ystart = yend;
		yend = (f + 1) * (1.f / yscale);
		if (yend >= height)
			yend = height - 0.001f;

		float xend = 0.0;
		for (int g = 0; g < new_width; g++) // x on output
		{
			float xstart = xend;
			xend = (g + 1) * (1.f / xscale);
			if (xend >= width)
				xend = width - 0.001f;

			float sum[max_num_channels] = {};
			for (int y = (int)ystart; y <= (int)yend; ++y)
			{
				float yportion = 1.0f;
				if (y == (int)ystart)
					yportion -= ystart - y;
				if (y == (int)yend)
					yportion -= y + 1 - yend;

				for (int x = (int)xstart; x <= (int)xend; ++x)
				{
					float xportion = 1.0f;
					if (x == (int)xstart)
						xportion -= xstart - x;
					if (x == (int)xend)
						xportion -= x + 1 - xend;

					for (int i = 0; i < num_channels; i++)
						sum[i] += bitmap_source[y * pitch + x * num_channels + i] * yportion * xportion;
				}
			}

			for (int i = 0; i < num_channels; i++)
				bitmap_new[(f * new_width + g) * num_channels + i] = (Rml::byte)Rml::Math::Min(sum[i] * sumscale, 255.f);
		}
	}
}
} // namespace FreeType
