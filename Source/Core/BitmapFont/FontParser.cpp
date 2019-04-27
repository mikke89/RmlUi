/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
#include "FontParser.h"

namespace Rocket {
namespace Core {
namespace BitmapFont {

FontParser::FontParser( BitmapFontDefinitions *face )
	: BaseXMLParser()
{
	bm_face = face;
	char_id = 0;
	kern_id = 0;
}

FontParser::~FontParser()
{
}

// Called when the parser finds the beginning of an element tag.
void FontParser::HandleElementStart(const String& name, const XMLAttributes& attributes)
{
	if ( name == "info" )
	{
		bm_face->Face.FamilyName = Get(attributes, "face", String());
		bm_face->Face.Size = Get(attributes, "size", 0);
		bm_face->Face.Weight = Get(attributes, "bold", false ) ? Font::WEIGHT_BOLD : Font::WEIGHT_NORMAL;
		bm_face->Face.Style = Get(attributes, "italic", false ) ? Font::STYLE_ITALIC : Font::STYLE_NORMAL;
		bm_face->Face.BitmapSource = Get(attributes, "src", String());
	}
	else if ( name == "common" )
	{
		bm_face->CommonCharactersInfo.LineHeight = Get(attributes, "lineHeight", 0);
		bm_face->CommonCharactersInfo.BaseLine = Get(attributes, "base", 0) * -1;
		bm_face->CommonCharactersInfo.ScaleWidth = Get(attributes, "scaleW", 0);
		bm_face->CommonCharactersInfo.ScaleHeight = Get(attributes, "scaleH", 0);
		bm_face->CommonCharactersInfo.CharacterCount = 0;
		bm_face->CommonCharactersInfo.KerningCount = 0;
	}
	else if ( name == "chars" )
	{
		bm_face->CommonCharactersInfo.CharacterCount = Get(attributes, "count", 0);
		bm_face->CharactersInfo = new CharacterInfo[ Get(attributes, "count", 0) ];
	}
	else if ( name == "char" )
	{
		bm_face->CharactersInfo[ char_id ].Id = Get(attributes, "id", 0);
		bm_face->CharactersInfo[ char_id ].X = Get(attributes, "x", 0); //The left position of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].Y = Get(attributes, "y", 0); //The top position of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].Width = Get(attributes, "width", 0); //The width of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].Height = Get(attributes, "height", 0); //The height of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].XOffset = Get(attributes, "xoffset", 0);
		bm_face->CharactersInfo[ char_id ].YOffset = Get(attributes, "yoffset", 0);
		bm_face->CharactersInfo[ char_id ].Advance = Get(attributes, "xadvance", 0);

		char_id++;
	}
	else if ( name == "kernings" )
	{
		bm_face->CommonCharactersInfo.KerningCount = Get(attributes, "count", 0);
		bm_face->KerningsInfo = new KerningInfo[ Get(attributes, "count", 0) ];
	}
	else if ( name == "kerning" )
	{
		bm_face->KerningsInfo[ kern_id ].FirstCharacterId = Get(attributes, "first", 0);
		bm_face->KerningsInfo[ kern_id ].SecondCharacterId = Get(attributes, "second", 0);
		bm_face->KerningsInfo[ kern_id ].KerningAmount = Get(attributes, "amount", 0);

		kern_id++;
	}
}

// Called when the parser finds the end of an element tag.
void FontParser::HandleElementEnd(const String& ROCKET_UNUSED_PARAMETER(name))
{
	ROCKET_UNUSED(name);
}

// Called when the parser encounters data.
void FontParser::HandleData(const String& ROCKET_UNUSED_PARAMETER(data))
{
	ROCKET_UNUSED(data);
}

}
}
}
