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

#include "precompiled.h"
#include "FontFaceHandle.h"
#include <algorithm>
#include "../../../Include/RmlUi/Core.h"
#include "../FontFaceLayer.h"
#include "../TextureLayout.h"

namespace Rml {
namespace Core {

static void BuildGlyph(FontGlyph& glyph, FT_GlyphSlot ft_glyph);
static void BuildGlyphMap(FT_Face ft_face, FontGlyphMap& glyphs);
static void GenerateMetrics(FT_Face ft_face, const FontGlyphMap& glyphs, FontMetrics& metrics);


FontFaceHandle_FreeType::FontFaceHandle_FreeType()
{
	ft_face = nullptr;
}

FontFaceHandle_FreeType::~FontFaceHandle_FreeType()
{
}


// Initialises the handle so it is able to render text.
bool FontFaceHandle_FreeType::Initialise(FT_Face ft_face, int size)
{
	this->ft_face = ft_face;
	GetMetrics().size = size;

	// Set the character size on the font face.
	FT_Error error = FT_Set_Char_Size(ft_face, 0, size << 6, 0, 0);
	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "Unable to set the character size '%d' on the font face '%s %s'.", size, ft_face->family_name, ft_face->style_name);
		return false;
	}

	// Construct the list of the characters specified by the charset.
	BuildGlyphMap(ft_face, GetGlyphs());

	// Generate the metrics for the handle.
	GenerateMetrics(ft_face, GetGlyphs(), GetMetrics());

	// Generate the default layer and layer configuration.
	GenerateBaseLayer();

	return true;
}

static void BuildGlyphMap(FT_Face ft_face, FontGlyphMap& glyphs)
{
	// TODO: ASCII range for now
	FT_ULong code_min = 32;
	FT_ULong code_max = 126;

	for (FT_ULong character_code = code_min; character_code <= code_max; ++character_code)
	{
		int index = FT_Get_Char_Index(ft_face, character_code);
		if (index != 0)
		{
			FT_Error error = FT_Load_Glyph(ft_face, index, 0);
			if (error != 0)
			{
				Log::Message(Log::LT_WARNING, "Unable to load glyph for character '%u' on the font face '%s %s'; error code: %d.", character_code, ft_face->family_name, ft_face->style_name, error);
				continue;
			}

			error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
			if (error != 0)
			{
				Log::Message(Log::LT_WARNING, "Unable to render glyph for character '%u' on the font face '%s %s'; error code: %d.", character_code, ft_face->family_name, ft_face->style_name, error);
				continue;
			}

			FontGlyph glyph;
			BuildGlyph(glyph, ft_face->glyph);
			glyphs[(CodePoint)character_code] = glyph;
		}
	}
}

static void BuildGlyph(FontGlyph& glyph, FT_GlyphSlot ft_glyph)
{
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
			glyph.bitmap_data = nullptr;
			Log::Message(Log::LT_WARNING, "Unable to render glyph on the font face '%s %s'; unsupported pixel mode (%d).", ft_glyph->face->family_name, ft_glyph->face->style_name, ft_glyph->bitmap.pixel_mode);
		}
		else
		{
			glyph.bitmap_data = new byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y];

			byte* source_bitmap = ft_glyph->bitmap.buffer;
			byte* destination_bitmap = glyph.bitmap_data;

			// Copy the bitmap data into the newly-allocated space on our glyph.
			switch (ft_glyph->bitmap.pixel_mode)
			{
				// Unpack 1-bit data into 8-bit.
			case FT_PIXEL_MODE_MONO:
			{
				for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
				{
					int mask = 0x80;
					byte* source_byte = source_bitmap;
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
		glyph.bitmap_data = nullptr;
}


static void GenerateMetrics(FT_Face ft_face, const FontGlyphMap& glyphs, FontMetrics& metrics)
{
	metrics.line_height = ft_face->size->metrics.height >> 6;
	metrics.baseline = metrics.line_height - (ft_face->size->metrics.ascender >> 6);

	metrics.underline_position = FT_MulFix(ft_face->underline_position, ft_face->size->metrics.y_scale) / float(1 << 6);
	metrics.underline_thickness = FT_MulFix(ft_face->underline_thickness, ft_face->size->metrics.y_scale) / float(1 << 6);
	metrics.underline_thickness = Math::Max(metrics.underline_thickness, 1.0f);

	metrics.average_advance = 0;
	unsigned int num_visible_glyphs = 0;
	for (auto it = glyphs.begin(); it != glyphs.end(); ++it)
	{
		const FontGlyph& glyph = it->second;
		if (glyph.advance)
		{
			metrics.average_advance += glyph.advance;
			num_visible_glyphs++;
		}
	}

	// Bring the total advance down to the average advance, but scaled up 10%, just to be on the safe side.
	if (num_visible_glyphs)
		metrics.average_advance = Math::RealToInteger((float)metrics.average_advance / (num_visible_glyphs * 0.9f));

	// Determine the x-height of this font face.
	int index = FT_Get_Char_Index(ft_face, 'x');
	if (FT_Load_Glyph(ft_face, index, 0) == 0)
		metrics.x_height = ft_face->glyph->metrics.height >> 6;
	else
		metrics.x_height = 0;
}


int FontFaceHandle_FreeType::GetKerning(CodePoint lhs, CodePoint rhs) const
{
	if (!FT_HAS_KERNING(ft_face))
		return 0;

	FT_Vector ft_kerning;

	FT_Error ft_error = FT_Get_Kerning(
		ft_face,
		FT_Get_Char_Index(ft_face, (FT_ULong)lhs), 
		FT_Get_Char_Index(ft_face, (FT_ULong)rhs), 
		FT_KERNING_DEFAULT, 
		&ft_kerning
	);

	if (ft_error != 0)
		return 0;

	int kerning = ft_kerning.x >> 6;
	return kerning;
}


}
}
