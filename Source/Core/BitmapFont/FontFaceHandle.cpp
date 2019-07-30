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
#include "FontFaceLayer.h"
#include <algorithm>
#include "../TextureLayout.h"

namespace Rml {
namespace Core {

BitmapFont::FontFaceHandle::FontFaceHandle()
{
	bm_face = nullptr;
	texture_width = 0;
	texture_height = 0;
}

BitmapFont::FontFaceHandle::~FontFaceHandle()
{
}

// Initialises the handle so it is able to render text.
bool BitmapFont::FontFaceHandle::Initialise(BitmapFontDefinitions *_bm_face, const String& _charset, int _size)
{
	bm_face = _bm_face;
	size = _size;
	line_height = _size;
	texture_width = bm_face->CommonCharactersInfo.ScaleWidth;
	texture_height = bm_face->CommonCharactersInfo.ScaleHeight;
	raw_charset = _charset;

	// Construct proper path to texture
	URL fnt_source = bm_face->Face.Source;
	URL bitmap_source = bm_face->Face.BitmapSource;
	if(bitmap_source.GetPath().empty())
	{
		texture_source = fnt_source.GetPath() + bitmap_source.GetFileName();
		if(!bitmap_source.GetExtension().empty())
		{
			texture_source += "." + bitmap_source.GetExtension();
		}
	}
	else
	{
		texture_source = bitmap_source.GetPathedFileName();
	}

	if (!UnicodeRange::BuildList(charset, raw_charset))
	{
		Log::Message(Log::LT_ERROR, "Invalid font charset '%s'.", raw_charset.c_str());
		return false;
	}

	// Construct the list of the characters specified by the charset.
	for (size_t i = 0; i < charset.size(); ++i)
		BuildGlyphMap(bm_face, charset[i]);

	// Generate the metrics for the handle.
	GenerateMetrics(bm_face);

	// Generate the default layer and layer configuration.
	base_layer = GenerateLayer(nullptr);
	layer_configurations.push_back(LayerConfiguration());
	layer_configurations.back().push_back(base_layer);

	return true;
}

void BitmapFont::FontFaceHandle::GenerateMetrics(BitmapFontDefinitions *bm_face)
{
	line_height = bm_face->CommonCharactersInfo.LineHeight;
	baseline = bm_face->CommonCharactersInfo.BaseLine;

	underline_position = (float)line_height - bm_face->CommonCharactersInfo.BaseLine;
	baseline += int( underline_position / 1.6f );
	underline_thickness = 1.0f;

	average_advance = 0;
	for (FontGlyphList::iterator i = glyphs.begin(); i != glyphs.end(); ++i)
		average_advance += i->advance;

	// Bring the total advance down to the average advance, but scaled up 10%, just to be on the safe side.
	average_advance = Math::RealToInteger((float) average_advance / (glyphs.size() * 0.9f));

	// Determine the x-height of this font face.
	word x = (word) 'x';
	int index = bm_face->BM_Helper_GetCharacterTableIndex( x );// FT_Get_Char_Index(ft_face, x);

	if ( index >= 0)
		x_height = bm_face->CharactersInfo[ index ].Height;
	else
		x_height = 0;
}

void BitmapFont::FontFaceHandle::BuildGlyphMap(BitmapFontDefinitions *bm_face, const UnicodeRange& unicode_range)
{
	glyphs.resize(unicode_range.max_codepoint + 1);

	for (word character_code = (word) (Math::Max< unsigned int >(unicode_range.min_codepoint, 32)); character_code <= unicode_range.max_codepoint; ++character_code)
	{
		int index = bm_face->BM_Helper_GetCharacterTableIndex( character_code );

		if ( index < 0 )
		{
			continue;
		}

		FontGlyph glyph;
		glyph.character = character_code;
		BuildGlyph(glyph, &bm_face->CharactersInfo[ index ] );
		glyphs[character_code] = glyph;
	}
}

void BitmapFont::FontFaceHandle::BuildGlyph(FontGlyph& glyph, CharacterInfo *bm_glyph)
{
	// Set the glyph's dimensions.
	glyph.dimensions.x = bm_glyph->Width;
	glyph.dimensions.y = bm_glyph->Height;

	// Set the glyph's bearing.
	glyph.bearing.x = bm_glyph->XOffset;
	glyph.bearing.y = bm_glyph->YOffset;

	// Set the glyph's advance.
	glyph.advance = bm_glyph->Advance;

	// Set the glyph's bitmap position.
	glyph.bitmap_dimensions.x = bm_glyph->X;
	glyph.bitmap_dimensions.y = bm_glyph->Y;

	glyph.bitmap_data = nullptr;
}

int BitmapFont::FontFaceHandle::GetKerning(word lhs, word rhs) const
{
	if( bm_face != nullptr)
	{
		return bm_face->BM_Helper_GetXKerning(lhs, rhs);
	}

	return 0;
}

Rml::Core::FontFaceLayer* BitmapFont::FontFaceHandle::CreateNewLayer()
{
	return new BitmapFont::FontFaceLayer();
}


}
}
