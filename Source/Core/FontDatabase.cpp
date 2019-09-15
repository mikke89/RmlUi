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

namespace Rml {
namespace Core {

FontDatabase* FontDatabase::instance = nullptr;
FontDatabase::FontProviderTable FontDatabase::font_provider_table;

FontDatabase::FontDatabase()
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;
}

FontDatabase::~FontDatabase()
{
	RMLUI_ASSERT(instance == this);
	instance = nullptr;
}

bool FontDatabase::Initialise()
{
	if (instance == nullptr)
	{
		new FontDatabase();

        if(!FontProvider_FreeType::Initialise())
            return false;
	}

	return true;
}

void FontDatabase::Shutdown()
{
	if (instance != nullptr)
	{
        FontProvider_FreeType::Shutdown();

		delete instance;
	}
}

// Loads a new font face.
bool FontDatabase::LoadFontFace(const String& file_name)
{
	return FontProvider_FreeType::LoadFontFace(file_name);
}

// Adds a new font face to the database, ignoring any family, style and weight information stored in the face itself.
bool FontDatabase::LoadFontFace(const String& file_name, const String& family, Style::FontStyle style, Style::FontWeight weight)
{
	return FontProvider_FreeType::LoadFontFace(file_name, family, style, weight);
}

// Adds a new font face to the database, loading from memory.
bool FontDatabase::LoadFontFace(const byte* data, int data_length)
{
	return FontProvider_FreeType::LoadFontFace(data, data_length);
}

// Adds a new font face to the database, loading from memory, ignoring any family, style and weight information stored in the face itself.
bool FontDatabase::LoadFontFace(const byte* data, int data_length, const String& family, Style::FontStyle style, Style::FontWeight weight)
{
	return FontProvider_FreeType::LoadFontFace(data, data_length, family, style, weight);
}

// Returns a handle to a font face that can be used to position and render text.
SharedPtr<FontFaceHandle> FontDatabase::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
    size_t provider_count = font_provider_table.size();

    for(size_t provider_index = 0; provider_index < provider_count; ++provider_index)
    {
		SharedPtr<FontFaceHandle> face_handle = font_provider_table[ provider_index ]->GetFontFaceHandle(family, style, weight, size);

        if(face_handle)
            return face_handle;
    }

    return nullptr;
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
