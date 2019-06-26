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
#include <RmlUi/Core/FontDatabase.h>
#include <RmlUi/Core/FontFamily.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/FreeType/FontProvider.h>
#include <RmlUi/Core/BitmapFont/FontProvider.h>

namespace Rml {
namespace Core {

FontDatabase* FontDatabase::instance = NULL;
FontDatabase::FontProviderTable FontDatabase::font_provider_table;

FontDatabase::FontDatabase()
{
	RMLUI_ASSERT(instance == NULL);
	instance = this;
}

FontDatabase::~FontDatabase()
{
	RMLUI_ASSERT(instance == this);
	instance = NULL;
}

bool FontDatabase::Initialise()
{
	if (instance == NULL)
	{
		new FontDatabase();

        if(!FreeType::FontProvider::Initialise())
            return false;

        if(!BitmapFont::FontProvider::Initialise())
            return false;
	}

	return true;
}

void FontDatabase::Shutdown()
{
	if (instance != NULL)
	{
        FreeType::FontProvider::Shutdown();
        BitmapFont::FontProvider::Shutdown();

		delete instance;
	}
}

// Loads a new font face.
bool FontDatabase::LoadFontFace(const String& file_name)
{
    FontProviderType font_provider_type = GetFontProviderType(file_name);

    switch(font_provider_type)
    {
        case FreeType:
            return FreeType::FontProvider::LoadFontFace(file_name);

        case BitmapFont:
            return BitmapFont::FontProvider::LoadFontFace(file_name);

        default:
            return false;
    }
}

// Adds a new font face to the database, ignoring any family, style and weight information stored in the face itself.
bool FontDatabase::LoadFontFace(const String& file_name, const String& family, Font::Style style, Font::Weight weight)
{
    FontProviderType font_provider_type = GetFontProviderType(file_name);

    switch(font_provider_type)
    {
        case FreeType:
            return FreeType::FontProvider::LoadFontFace(file_name, family, style, weight);

        case BitmapFont:
            return BitmapFont::FontProvider::LoadFontFace(file_name, family, style, weight);

        default:
            return false;
    }
}

// Adds a new font face to the database, loading from memory.
bool FontDatabase::LoadFontFace(FontProviderType font_provider_type, const byte* data, int data_length)
{
    switch(font_provider_type)
    {
        case FreeType:
            return FreeType::FontProvider::LoadFontFace(data, data_length);

        case BitmapFont:
            return BitmapFont::FontProvider::LoadFontFace(data, data_length);

        default:
            return false;
    }
}

// Adds a new font face to the database, loading from memory, ignoring any family, style and weight information stored in the face itself.
bool FontDatabase::LoadFontFace(FontProviderType font_provider_type, const byte* data, int data_length, const String& family, Font::Style style, Font::Weight weight)
{
    switch(font_provider_type)
    {
        case FreeType:
            return FreeType::FontProvider::LoadFontFace(data, data_length, family, style, weight);

        case BitmapFont:
            return BitmapFont::FontProvider::LoadFontFace(data, data_length, family, style, weight);

        default:
            return false;
    }
}

FontDatabase::FontProviderType FontDatabase::GetFontProviderType(const String& file_name)
{
    if(file_name.find(".fnt") != String::npos)
    {
        return BitmapFont;
    }
    else
    {
        return FreeType;
    }
}

// Returns a handle to a font face that can be used to position and render text.
FontFaceHandle* FontDatabase::GetFontFaceHandle(const String& family, const String& charset, Font::Style style, Font::Weight weight, int size)
{
    size_t provider_index, provider_count;

    provider_count = font_provider_table.size();

    for(provider_index = 0; provider_index < provider_count; ++provider_index)
    {
        FontFaceHandle * face_handle = font_provider_table[ provider_index ]->GetFontFaceHandle(family, charset, style, weight, size);

        if(face_handle)
        {
            return face_handle;
        }
    }

    return NULL;
}

void FontDatabase::AddFontProvider(FontProvider * provider)
{
    instance->font_provider_table.push_back(provider);
}

void FontDatabase::RemoveFontProvider(FontProvider * provider)
{
    for(FontProviderTable::iterator i = instance->font_provider_table.begin(); i != instance->font_provider_table.end(); ++i)
    {
        if(*i == provider)
        {
            instance->font_provider_table.erase(i);
            return;
        }
    }
}

}
}
