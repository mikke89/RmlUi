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

#include "../precompiled.h"
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
		bm_face->Face.FamilyName = attributes.Get( "face" )->Get< String >();
		bm_face->Face.Size = attributes.Get( "size" )->Get< int >();
		bm_face->Face.Weight = attributes.Get( "bold" )->Get< bool >() ? Font::WEIGHT_BOLD : Font::WEIGHT_NORMAL;
		bm_face->Face.Style = attributes.Get( "italic" )->Get< bool >() ? Font::STYLE_ITALIC : Font::STYLE_NORMAL;
		bm_face->Face.BitmapSource = attributes.Get( "src" )->Get< String >();
	}
	else if ( name == "common" )
	{
		bm_face->CommonCharactersInfo.LineHeight = attributes.Get( "lineHeight" )->Get< int >();
		bm_face->CommonCharactersInfo.BaseLine = attributes.Get( "base" )->Get< int >() * -1;
		bm_face->CommonCharactersInfo.ScaleWidth = attributes.Get( "scaleW" )->Get< int >();
		bm_face->CommonCharactersInfo.ScaleHeight = attributes.Get( "scaleH" )->Get< int >();
		bm_face->CommonCharactersInfo.CharacterCount = 0;
		bm_face->CommonCharactersInfo.KerningCount = 0;
	}
	else if ( name == "chars" )
	{
		bm_face->CommonCharactersInfo.CharacterCount = attributes.Get( "count" )->Get< int >();
		bm_face->CharactersInfo = new CharacterInfo[ attributes.Get( "count" )->Get< int >() ];
	}
	else if ( name == "char" )
	{
		bm_face->CharactersInfo[ char_id ].Id = attributes.Get( "id" )->Get< int >();
		bm_face->CharactersInfo[ char_id ].X = attributes.Get( "x" )->Get< int >(); //The left position of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].Y = attributes.Get( "y" )->Get< int >(); //The top position of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].Width = attributes.Get( "width" )->Get< int >(); //The width of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].Height = attributes.Get( "height" )->Get< int >(); //The height of the character image in the texture.
		bm_face->CharactersInfo[ char_id ].XOffset = attributes.Get( "xoffset" )->Get< int >();
		bm_face->CharactersInfo[ char_id ].YOffset = attributes.Get( "yoffset" )->Get< int >();
		bm_face->CharactersInfo[ char_id ].Advance = attributes.Get( "xadvance" )->Get< int >();

		char_id++;
	}
	else if ( name == "kernings" )
	{
		bm_face->CommonCharactersInfo.KerningCount = attributes.Get( "count" )->Get< int >();
		bm_face->KerningsInfo = new KerningInfo[ attributes.Get( "count" )->Get< int >() ];
	}
	else if ( name == "kerning" )
	{
		bm_face->KerningsInfo[ kern_id ].FirstCharacterId = attributes.Get( "first" )->Get< int >();
		bm_face->KerningsInfo[ kern_id ].SecondCharacterId = attributes.Get( "second" )->Get< int >();
		bm_face->KerningsInfo[ kern_id ].KerningAmount = attributes.Get( "amount" )->Get< int >();

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
