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
#include <RmlUi/Core/StreamMemory.h>
#include "FontFamily.h"
#include <RmlUi/Core.h>
#include "BitmapFontDefinitions.h"
#include "FontParser.h"

namespace Rml {
namespace Core {

BitmapFont::FontProvider* BitmapFont::FontProvider::instance = nullptr;

BitmapFont::FontProvider::FontProvider()
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;
}

BitmapFont::FontProvider::~FontProvider()
{
	RMLUI_ASSERT(instance == this);
	instance = nullptr;
}

bool BitmapFont::FontProvider::Initialise()
{
	if (instance == nullptr)
	{
		new FontProvider();

		FontDatabaseDefault::AddFontProvider(instance);
	}

	return true;
}

void BitmapFont::FontProvider::Shutdown()
{
	if (instance != nullptr)
	{
		FontDatabaseDefault::RemoveFontProvider(instance);
		delete instance;
		instance = nullptr;
	}
}

// Adds a new font face to the database, ignoring any family, style and weight information stored in the face itself.
bool BitmapFont::FontProvider::LoadFontFace(const String& file_name)
{
	BitmapFont::BitmapFontDefinitions *bm_font = (BitmapFont::BitmapFontDefinitions*) instance->LoadFace(file_name);

	if (bm_font == nullptr)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s.", file_name.c_str());
		return false;
	}

	Style::FontStyle style = bm_font->Face.Style;
	Style::FontWeight weight = bm_font->Face.Weight;

	if (instance->AddFace(bm_font, bm_font->Face.FamilyName, style, weight, true))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s (from %s).", bm_font->Face.FamilyName.c_str(), file_name.c_str());
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s (from %s).", bm_font->Face.FamilyName.c_str(), file_name.c_str());
		return false;
	}

	return true;
}

// Loads a new font face.
bool BitmapFont::FontProvider::LoadFontFace(const String& file_name, const String& family, Style::FontStyle style, Style::FontWeight weight)
{
	BitmapFont::BitmapFontDefinitions *bm_font = (BitmapFont::BitmapFontDefinitions*) instance->LoadFace(file_name);
	if (bm_font == nullptr)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s.", file_name.c_str());
		return false;
	}

	if (instance->AddFace(bm_font, family, style, weight, true))
	{
		Log::Message(Log::LT_INFO, "Loaded font face %s (from %s).", bm_font->Face.FamilyName.c_str(), file_name.c_str());
		return true;
	}
	else
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s (from %s).", bm_font->Face.FamilyName.c_str(), file_name.c_str());
		return false;
	}

	return true;
}

bool BitmapFont::FontProvider::LoadFontFace(const byte* data, int data_length)
{
	// TODO: Loading from memory
	return false;
}

// Adds a new font face to the database, loading from memory.
bool BitmapFont::FontProvider::LoadFontFace(const byte* data, int data_length, const String& family, Style::FontStyle style, Style::FontWeight weight)
{
	// TODO Loading from memory
	return false;
}

// Adds a loaded face to the appropriate font family.
bool BitmapFont::FontProvider::AddFace(void* face, const String& family, Style::FontStyle style, Style::FontWeight weight, bool release_stream)
{
	String family_lower = StringUtilities::ToLower(family);
	Rml::Core::FontFamily* font_family = nullptr;
	FontFamilyMap::iterator iterator = instance->font_families.find(family_lower);
	if (iterator != instance->font_families.end())
		font_family = (*iterator).second;
	else
	{
		font_family = new FontFamily(family_lower);
		instance->font_families[family_lower] = font_family;
	}

	return font_family->AddFace((BitmapFontDefinitions *) face, style, weight, release_stream);
}

// Loads a FreeType face.
void* BitmapFont::FontProvider::LoadFace(const String& file_name)
{
	BitmapFont::BitmapFontDefinitions *bm_face = new BitmapFont::BitmapFontDefinitions();
	BitmapFont::FontParser parser( bm_face );

	FileInterface* file_interface = GetFileInterface();
	FileHandle handle = file_interface->Open(file_name);

	if (!handle)
	{
		return nullptr;
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
void* BitmapFont::FontProvider::LoadFace(const byte* data, int data_length, const String& source, bool local_data)
{
	URL file_url = source + ".fnt";

	BitmapFont::BitmapFontDefinitions *bm_face = new BitmapFont::BitmapFontDefinitions();
	BitmapFont::FontParser parser( bm_face );
	StreamMemory* stream = new StreamMemory( data, data_length );
	stream->SetSourceURL( file_url );

	parser.Parse( stream );

	bm_face->Face.Source = file_url.GetPathedFileName();
	return bm_face;
}

}
}

#endif
