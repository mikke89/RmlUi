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
#include <Rocket/Core/FreeType/FontProvider.h>
#include "FontFaceHandle.h"
#include <Rocket/Core/FontDatabase.h>
#include "FontFamily.h"
#include <Rocket/Core.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace Rocket {
namespace Core {
namespace FreeType {

FontProvider* FontProvider::instance = NULL;

static FT_Library ft_library = NULL;

FontProvider::FontProvider()
{
	ROCKET_ASSERT(instance == NULL);
	instance = this;
}

FontProvider::~FontProvider()
{
	ROCKET_ASSERT(instance == this);
	instance = NULL;
}

bool FontProvider::Initialise()
{
	if (instance == NULL)
	{
		new FontProvider();

		FontDatabase::AddFontProvider(instance);

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
	if (instance != NULL)
	{
		for (FontFamilyMap::iterator i = instance->font_families.begin(); i != instance->font_families.end(); ++i)
			delete (*i).second;

		if (ft_library != NULL)
		{
			FT_Done_FreeType(ft_library);
			ft_library = NULL;
		}

		delete instance;
	}
}

// Loads a new font face.
bool FontProvider::LoadFontFace(const String& file_name)
{
	FT_Face ft_face = (FT_Face) instance->LoadFace(file_name);
	if (ft_face == NULL)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s.", file_name.CString());
		return false;
	}

	Font::Style style = ft_face->style_flags & FT_STYLE_FLAG_ITALIC ? Font::STYLE_ITALIC : Font::STYLE_NORMAL;
	Font::Weight weight = ft_face->style_flags & FT_STYLE_FLAG_BOLD ? Font::WEIGHT_BOLD : Font::WEIGHT_NORMAL;

	if (instance->AddFace(ft_face, ft_face->family_name, style, weight, true))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s %s (from %s).", ft_face->family_name, ft_face->style_name, file_name.CString());
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s %s (from %s).", ft_face->family_name, ft_face->style_name, file_name.CString());
		return false;
	}
}

// Adds a new font face to the database, ignoring any family, style and weight information stored in the face itself.
bool FontProvider::LoadFontFace(const String& file_name, const String& family, Font::Style style, Font::Weight weight)
{
	FT_Face ft_face = (FT_Face) instance->LoadFace(file_name);
	if (ft_face == NULL)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s.", file_name.CString());
		return false;
	}

	if (instance->AddFace(ft_face, family, style, weight, true))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s %s (from %s).", ft_face->family_name, ft_face->style_name, file_name.CString());
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s %s (from %s).", ft_face->family_name, ft_face->style_name, file_name.CString());
		return false;
	}
}

// Adds a new font face to the database, loading from memory.
bool FontProvider::LoadFontFace(const byte* data, int data_length)
{
	FT_Face ft_face = (FT_Face) instance->LoadFace(data, data_length, "memory", false);
	if (ft_face == NULL)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from byte stream.");
		return false;
	}

	Font::Style style = ft_face->style_flags & FT_STYLE_FLAG_ITALIC ? Font::STYLE_ITALIC : Font::STYLE_NORMAL;
	Font::Weight weight = ft_face->style_flags & FT_STYLE_FLAG_BOLD ? Font::WEIGHT_BOLD : Font::WEIGHT_NORMAL;

	if (instance->AddFace(ft_face, ft_face->family_name, style, weight, false))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s %s (from byte stream).", ft_face->family_name, ft_face->style_name);
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s %s (from byte stream).", ft_face->family_name, ft_face->style_name);
		return false;
	}
}

// Adds a new font face to the database, loading from memory, ignoring any family, style and weight information stored in the face itself.
bool FontProvider::LoadFontFace(const byte* data, int data_length, const String& family, Font::Style style, Font::Weight weight)
{
	FT_Face ft_face = (FT_Face) instance->LoadFace(data, data_length, "memory", false);
	if (ft_face == NULL)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from byte stream.");
		return false;
	}

	if (instance->AddFace(ft_face, family, style, weight, false))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s %s (from byte stream).", ft_face->family_name, ft_face->style_name);
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s %s (from byte stream).", ft_face->family_name, ft_face->style_name);
		return false;
	}
}

// Adds a loaded face to the appropriate font family.
bool FontProvider::AddFace(void* face, const String& family, Font::Style style, Font::Weight weight, bool release_stream)
{
	FontFamily* font_family = NULL;
	FontFamilyMap::iterator iterator = font_families.find(family);
	if (iterator != font_families.end())
		font_family = (FontFamily*)(*iterator).second;
	else
	{
		font_family = new FontFamily(family);
		font_families[family] = font_family;
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
		return NULL;
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
	FT_Face face = NULL;
	int error = FT_New_Memory_Face(ft_library, (const FT_Byte*) data, data_length, 0, &face);
	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "FreeType error %d while loading face from %s.", error, source.CString());
		if (local_data)
			delete[] data;

		return NULL;
	}

	// Initialise the character mapping on the face.
	if (face->charmap == NULL)
	{
		FT_Select_Charmap(face, FT_ENCODING_APPLE_ROMAN);
		if (face->charmap == NULL)
		{
			Log::Message(Log::LT_ERROR, "Font face (from %s) does not contain a Unicode or Apple Roman character map.", source.CString());
			FT_Done_Face(face);
			if (local_data)
				delete[] data;

			return NULL;
		}
	}

	return face;
}

}
}
}
