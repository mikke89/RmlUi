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

#ifndef RMLUI_NO_FONT_INTERFACE_DEFAULT

#include "FontProvider.h"
#include "FontFaceHandle.h"
#include "../FontDatabaseDefault.h"
#include "FontFamily.h"
#include <RmlUi/Core.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace Rml {
namespace Core {
namespace FreeType {

FontProvider* FontProvider::instance = nullptr;

static FT_Library ft_library = nullptr;

FontProvider::FontProvider()
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;
}

FontProvider::~FontProvider()
{
	RMLUI_ASSERT(instance == this);
	instance = nullptr;
}

bool FontProvider::Initialise()
{
	if (instance == nullptr)
	{
		new FontProvider();

		FontDatabaseDefault::AddFontProvider(instance);

		FT_Error result = FT_Init_FreeType(&ft_library);
		if (result != 0)
		{
			Log::Message(Log::LT_ERROR, "Failed to initialise FreeType, error %d.", result);
			Shutdown();
			return false;
		}
	}

	return true;
}

void FontProvider::Shutdown()
{
	if (instance != nullptr)
	{
		for (FontFamilyMap::iterator i = instance->font_families.begin(); i != instance->font_families.end(); ++i)
			delete (*i).second;

		if (ft_library != nullptr)
		{
			FT_Done_FreeType(ft_library);
			ft_library = nullptr;
		}

		FontDatabaseDefault::RemoveFontProvider(instance);
		delete instance;
		instance = nullptr;
	}
}

// Loads a new font face.
bool FontProvider::LoadFontFace(const String& file_name)
{
	FT_Face ft_face = (FT_Face) instance->LoadFace(file_name);
	if (ft_face == nullptr)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s.", file_name.c_str());
		return false;
	}

	Style::FontStyle style = ft_face->style_flags & FT_STYLE_FLAG_ITALIC ? Style::FontStyle::Italic : Style::FontStyle::Normal;
	Style::FontWeight weight = ft_face->style_flags & FT_STYLE_FLAG_BOLD ? Style::FontWeight::Bold : Style::FontWeight::Normal;

	if (instance->AddFace(ft_face, ft_face->family_name, style, weight, true))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s %s (from %s).", ft_face->family_name, ft_face->style_name, file_name.c_str());
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s %s (from %s).", ft_face->family_name, ft_face->style_name, file_name.c_str());
		return false;
	}
}

// Adds a loaded face to the appropriate font family.
bool FontProvider::AddFace(void* face, const String& family, Style::FontStyle style, Style::FontWeight weight, bool release_stream)
{
	String family_lower = StringUtilities::ToLower(family);
	FontFamily* font_family = nullptr;
	FontFamilyMap::iterator iterator = font_families.find(family_lower);
	if (iterator != font_families.end())
		font_family = (FontFamily*)(*iterator).second;
	else
	{
		font_family = new FontFamily(family_lower);
		font_families[family_lower] = font_family;
	}

	return font_family->AddFace((FT_Face) face, style, weight, release_stream);
}

// Loads a FreeType face.
void* FontProvider::LoadFace(const String& file_name)
{
	FileInterface* file_interface = GetFileInterface();
	FileHandle handle = file_interface->Open(file_name);

	if (!handle)
	{
		return nullptr;
	}

	size_t length = file_interface->Length(handle);

	FT_Byte* buffer = new FT_Byte[length];
	file_interface->Read(buffer, length, handle);
	file_interface->Close(handle);

	return LoadFace(buffer, (int)length, file_name, true);
}

// Loads a FreeType face from memory.
void* FontProvider::LoadFace(const byte* data, int data_length, const String& source, bool local_data)
{
	FT_Face face = nullptr;
	int error = FT_New_Memory_Face(ft_library, (const FT_Byte*) data, data_length, 0, &face);
	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "FreeType error %d while loading face from %s.", error, source.c_str());
		if (local_data)
			delete[] data;

		return nullptr;
	}

	// Initialise the character mapping on the face.
	if (face->charmap == nullptr)
	{
		FT_Select_Charmap(face, FT_ENCODING_APPLE_ROMAN);
		if (face->charmap == nullptr)
		{
			Log::Message(Log::LT_ERROR, "Font face (from %s) does not contain a Unicode or Apple Roman character map.", source.c_str());
			FT_Done_Face(face);
			if (local_data)
				delete[] data;

			return nullptr;
		}
	}

	return face;
}

}
}
}

#endif
