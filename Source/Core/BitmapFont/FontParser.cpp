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

FontParser::FontParser( BM_Font *face )
    : BaseXMLParser()
{
    BM_face = face;
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
        BM_face->Face.FamilyName = "Arial";//attributes.Get( "face" )->Get< String >();
        BM_face->Face.Size = attributes.Get( "size" )->Get< int >();
        BM_face->Face.Weight = attributes.Get( "bold" )->Get< bool >() ? Font::WEIGHT_BOLD : Font::WEIGHT_NORMAL;
        BM_face->Face.Style = attributes.Get( "italic" )->Get< bool >() ? Font::STYLE_ITALIC : Font::STYLE_NORMAL;
        BM_face->Face.CharsetName = "";//attributes.Get( "charset" )->Get< String >();
        BM_face->Face.IsUnicode = attributes.Get( "unicode" )->Get< bool >();
        BM_face->Face.StretchHeight = attributes.Get( "stretchH" )->Get< int >();
        BM_face->Face.IsSmoothed = attributes.Get( "smooth" )->Get< bool >();
        BM_face->Face.SuperSamplingLevel = attributes.Get( "aa" )->Get< int >();
        //:TODO:
        //BM_face->Face.FamilyName = attributes.Get( "padding" )->Get< String >();
        //BM_face->Face.FamilyName = attributes.Get( "spacing" )->Get< String >();
        BM_face->Face.Outline = attributes.Get( "outline" )->Get< int >();
    }
    else if ( name == "common" )
    {

        BM_face->CommonCharactersInfo.LineHeight = attributes.Get( "lineHeight" )->Get< int >();
        BM_face->CommonCharactersInfo.BaseLine = attributes.Get( "base" )->Get< int >();
        BM_face->CommonCharactersInfo.ScaleWidth = attributes.Get( "scaleW" )->Get< int >();
        BM_face->CommonCharactersInfo.ScaleHeight = attributes.Get( "scaleH" )->Get< int >();
        BM_face->CommonCharactersInfo.PageCount = attributes.Get( "pages" )->Get< int >();
        BM_face->CommonCharactersInfo.IsPacked = attributes.Get( "packed" )->Get< bool >();
        BM_face->CommonCharactersInfo.AlphaChanelUsage = attributes.Get( "alphaChnl" )->Get< int >();
        BM_face->CommonCharactersInfo.RedChanelUsage = attributes.Get( "redChnl" )->Get< int >();
        BM_face->CommonCharactersInfo.GreenChanelUsage = attributes.Get( "greenChnl" )->Get< int >();
        BM_face->CommonCharactersInfo.BlueChanelUsage = attributes.Get( "blueChnl" )->Get< int >();

        BM_face->PagesInfo = new PageInfo[BM_face->CommonCharactersInfo.PageCount];
        BM_face->CommonCharactersInfo.CharacterCount = 0;
        BM_face->CommonCharactersInfo.KerningCount = 0;
    }
    else if ( name == "page" )
    {
        BM_face->PagesInfo[ attributes.Get( "id" )->Get< int >() ].Id = attributes.Get( "id" )->Get< int >();
        BM_face->PagesInfo[ attributes.Get( "id" )->Get< int >() ].FileName = "Arial_0.tga";//attributes.Get( "file" )->Get< String >();
    }
    else if ( name == "chars" )
    {
        BM_face->CommonCharactersInfo.CharacterCount = attributes.Get( "count" )->Get< int >();
        BM_face->CharactersInfo = new CharacterInfo[ attributes.Get( "count" )->Get< int >() ];
    }
    else if ( name == "char" )
    {
        BM_face->CharactersInfo[ char_id ].Id = attributes.Get( "id" )->Get< int >();
        BM_face->CharactersInfo[ char_id ].X = attributes.Get( "x" )->Get< int >(); //The left position of the character image in the texture.
        BM_face->CharactersInfo[ char_id ].Y = attributes.Get( "y" )->Get< int >(); //The top position of the character image in the texture.
        BM_face->CharactersInfo[ char_id ].Width = attributes.Get( "width" )->Get< int >(); //The width of the character image in the texture.
        BM_face->CharactersInfo[ char_id ].Height = attributes.Get( "height" )->Get< int >(); //The height of the character image in the texture.
        BM_face->CharactersInfo[ char_id ].XOffset = attributes.Get( "xoffset" )->Get< int >();
        BM_face->CharactersInfo[ char_id ].YOffset = attributes.Get( "yoffset" )->Get< int >();
        BM_face->CharactersInfo[ char_id ].Advance = attributes.Get( "xadvance" )->Get< int >();
        BM_face->CharactersInfo[ char_id ].PageId = attributes.Get( "page" )->Get< int >();
        BM_face->CharactersInfo[ char_id ].ChannelUsed = attributes.Get( "chnl" )->Get< int >();

        char_id++;
    }
    else if ( name == "kernings" )
    {
        BM_face->CommonCharactersInfo.KerningCount = attributes.Get( "count" )->Get< int >();
        BM_face->KerningsInfo = new KerningInfo[ attributes.Get( "count" )->Get< int >() ];
    }
    else if ( name == "kerning" )
    {
        BM_face->KerningsInfo[ kern_id ].FirstCharacterId = attributes.Get( "first" )->Get< int >();
        BM_face->KerningsInfo[ kern_id ].SecondCharacterId = attributes.Get( "second" )->Get< int >();
        BM_face->KerningsInfo[ kern_id ].KerningAmount = attributes.Get( "amount" )->Get< int >();

        kern_id++;
    }
}

// Called when the parser finds the end of an element tag.
void FontParser::HandleElementEnd(const String& ROCKET_UNUSED(name))
{
}

// Called when the parser encounters data.
void FontParser::HandleData(const String& ROCKET_UNUSED(data))
{
}

}
}
}
