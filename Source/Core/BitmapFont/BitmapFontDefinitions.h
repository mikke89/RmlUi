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

#ifndef BITMAPFONTDEFINITIONS_H
#define BITMAPFONTDEFINITIONS_H

#include <Rocket/Core/Header.h>
#include <Rocket/Core/Types.h>
#include <Rocket/Core/Dictionary.h>
#include <set>

namespace Rocket {
namespace Core {
namespace BitmapFont {

	struct FontInfo
	{
		String FamilyName;
		String Source;
		String BitmapSource;
		int Size;
		Font::Style Style;
		Font::Weight Weight;
	};

	struct CharacterCommonInfo
	{
		int LineHeight;
		int BaseLine;
		int ScaleWidth;
		int ScaleHeight;
		int CharacterCount;
		int KerningCount;
	};

	struct CharacterInfo
	{
		int Id;
		int X;
		int Y;
		int Width;
		int Height;
		int XOffset;
		int YOffset;
		int Advance;
	};

	struct KerningInfo
	{
		int FirstCharacterId;
		int SecondCharacterId;
		int KerningAmount;
	};

	class BitmapFontDefinitions
	{
	public:
		FontInfo Face;
		CharacterCommonInfo CommonCharactersInfo;
		CharacterInfo *CharactersInfo;
		KerningInfo *KerningsInfo;

		int BM_Helper_GetCharacterTableIndex( int unicode_code )
		{
			return BinarySearch( unicode_code, 0, CommonCharactersInfo.CharacterCount );
		}

		int BM_Helper_GetXKerning( int left_uni_id, int right_uni_id )
		{
			for ( int i = 0; i < this->CommonCharactersInfo.KerningCount; i++ )
			{
				if ( this->KerningsInfo[i].FirstCharacterId == left_uni_id && this->KerningsInfo[i].SecondCharacterId == right_uni_id )
				{
					return this->KerningsInfo[i].KerningAmount;
				}
			}

			return 0;
		}

	private:

		int BinarySearch( int unicode_code, int min_index, int max_index )
		{
			if ( abs( max_index - min_index ) <= 1 )
			{
				if ( this->CharactersInfo[ min_index ].Id == unicode_code )
				{
					return min_index;
				}
				else if ( this->CharactersInfo[ max_index ].Id == unicode_code )
				{
					return max_index;
				}
				else
				{
					return -1;
				}
			}
			else
			{
				int mid_index = ( min_index + max_index ) / 2;

				if ( this->CharactersInfo[ mid_index ].Id == unicode_code )
				{
					return mid_index;
				}
				else if ( this->CharactersInfo[ mid_index ].Id > unicode_code )
				{
					return BinarySearch( unicode_code, min_index, mid_index );
				}
				else
				{
					return BinarySearch( unicode_code, mid_index, max_index );
				}
			}
		}
	};

}
}
}
#endif
