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
#include <Rocket/Core/BitmapFont/FontProvider.h>
#include "../FontFaceHandle.h"
#include <Rocket/Core/FontDatabase.h>
#include <Rocket/Core/StreamMemory.h>
#include "FontFamily.h"
#include <Rocket/Core.h>
#include "BitmapFontDefinitions.h"
#include "FontParser.h"

namespace Rocket {
namespace Core {
namespace BitmapFont {


FontProvider* FontProvider::instance = NULL;

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
	}

	return true;
}

void FontProvider::Shutdown()
{
	if (instance != NULL)
	{
		FontDatabase::RemoveFontProvider(instance);
		delete instance;
		instance = NULL;
	}
}

// Adds a new font face to the database, ignoring any family, style and weight information stored in the face itself.
bool FontProvider::LoadFontFace(const String& file_name)
{
	BitmapFontDefinitions *bm_font = (BitmapFontDefinitions*) instance->LoadFace(file_name);

	if (bm_font == NULL)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s.", file_name.CString());
		return false;
	}

	Font::Style style = bm_font->Face.Style;
	Font::Weight weight = bm_font->Face.Weight;

	if (instance->AddFace(bm_font, bm_font->Face.FamilyName, style, weight, true))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s (from %s).", bm_font->Face.FamilyName.CString(), file_name.CString());
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s (from %s).", bm_font->Face.FamilyName.CString(), file_name.CString());
		return false;
	}

	return true;
}

// Loads a new font face.
bool FontProvider::LoadFontFace(const String& file_name, const String& family, Font::Style style, Font::Weight weight)
{
	BitmapFontDefinitions *bm_font = (BitmapFontDefinitions*) instance->LoadFace(file_name);
	if (bm_font == NULL)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s.", file_name.CString());
		return false;
	}

	if (instance->AddFace(bm_font, family, style, weight, true))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s (from %s).", bm_font->Face.FamilyName.CString(), file_name.CString());
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s (from %s).", bm_font->Face.FamilyName.CString(), file_name.CString());
		return false;
	}

	return true;
}

bool FontProvider::LoadFontFace(const byte* data, int data_length)
{
	// TODO: Loading from memory
	return false;
}

// Adds a new font face to the database, loading from memory.
bool FontProvider::LoadFontFace(const byte* data, int data_length, const String& family, Font::Style style, Font::Weight weight)
{
	// TODO Loading from memory
	return false;
}

// Adds a loaded face to the appropriate font family.
bool FontProvider::AddFace(void* face, const String& family, Font::Style style, Font::Weight weight, bool release_stream)
{
	Rocket::Core::FontFamily* font_family = NULL;
	FontFamilyMap::iterator iterator = instance->font_families.find(family);
	if (iterator != instance->font_families.end())
		font_family = (*iterator).second;
	else
	{
		font_family = new FontFamily(family);
		instance->font_families[family] = font_family;
	}

	return font_family->AddFace((BitmapFontDefinitions *) face, style, weight, release_stream);
	return true;
}

// Loads a FreeType face.
void* FontProvider::LoadFace(const String& file_name)
{
	BitmapFontDefinitions *bm_face = new BitmapFontDefinitions();
	FontParser parser( bm_face );

	FileInterface* file_interface = GetFileInterface();
	FileHandle handle = file_interface->Open(file_name);

	if (!handle)
	{
		return NULL;
	}

	size_t length = file_interface->Length(handle);

	byte* buffer = new byte[length];
	file_interface->Read(buffer, length, handle);
	file_interface->Close(handle);

	StreamMemory* stream = new StreamMemory( buffer, length );
	stream->SetSourceURL( file_name );

	parser.Parse( stream );

	bm_face->Face.Source = file_name;
	return bm_face;
}

// Loads a FreeType face from memory.
void* FontProvider::LoadFace(const byte* data, int data_length, const String& source, bool local_data)
{
	URL file_url = source + ".fnt";

	BitmapFontDefinitions *bm_face = new BitmapFontDefinitions();
	FontParser parser( bm_face );
	StreamMemory* stream = new StreamMemory( data, data_length );
	stream->SetSourceURL( file_url );

	parser.Parse( stream );

	bm_face->Face.Source = file_url.GetPathedFileName();
	return bm_face;
}

}
}
}
