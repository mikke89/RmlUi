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

#include "FontProvider.h"
#include "FontFace.h"
#include "FontFamily.h"
#include "FreeTypeInterface.h"
#include "../../../Include/RmlUi/Core/Core.h"
#include "../../../Include/RmlUi/Core/FileInterface.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/StringUtilities.h"
#include <algorithm>

namespace Rml {
namespace Core {

static FontProvider* g_font_provider = nullptr;

FontProvider::FontProvider()
{
	RMLUI_ASSERT(!g_font_provider);
}

FontProvider::~FontProvider()
{
	RMLUI_ASSERT(g_font_provider == this);
}

bool FontProvider::Initialise()
{
	RMLUI_ASSERT(!g_font_provider);
	if (!FreeType::Initialise())
		return false;
	g_font_provider = new FontProvider;
	return true;
}

void FontProvider::Shutdown()
{
	RMLUI_ASSERT(g_font_provider);
	delete g_font_provider;
	g_font_provider = nullptr;
	FreeType::Shutdown();
}

FontProvider& FontProvider::Get()
{
	RMLUI_ASSERT(g_font_provider);
	return *g_font_provider;
}

FontFaceHandleDefault* FontProvider::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
	RMLUI_ASSERTMSG(family == StringUtilities::ToLower(family), "Font family name must be converted to lowercase before entering here.");

	FontFamilyMap& families = Get().font_families;

	auto it = families.find(family);
	if (it == families.end())
		return nullptr;

	return it->second->GetFaceHandle(style, weight, size);
}

int FontProvider::CountFallbackFontFaces()
{
	return (int)Get().fallback_font_faces.size();
}

FontFaceHandleDefault* FontProvider::GetFallbackFontFace(int index, int font_size)
{
	auto& faces = FontProvider::Get().fallback_font_faces;

	if (index >= 0 && index < (int)faces.size())
		return faces[index]->GetHandle(font_size);

	return nullptr;
}


bool FontProvider::LoadFontFace(const String& file_name, bool fallback_face)
{
	FileInterface* file_interface = GetFileInterface();
	FileHandle handle = file_interface->Open(file_name);

	if (!handle)
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face from %s, could not open file.", file_name.c_str());
		return false;
	}

	size_t length = file_interface->Length(handle);

	byte* buffer = new byte[length];
	file_interface->Read(buffer, length, handle);
	file_interface->Close(handle);

	bool result = Get().LoadFontFace(buffer, (int)length, fallback_face, true, file_name);

	return result;
}


bool FontProvider::LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face)
{
	const String source = "memory";
	
	bool result = Get().LoadFontFace(data, data_size, fallback_face, false, source, font_family, style, weight);
	
	return result;
}

bool FontProvider::LoadFontFace(const byte* data, int data_size, bool fallback_face, bool local_data, const String& source,
	String font_family, Style::FontStyle style, Style::FontWeight weight)
{
	FontFaceHandleFreetype ft_face = FreeType::LoadFace(data, data_size, source);
	
	if (!ft_face)
	{
		if (local_data)
			delete[] data;

		Log::Message(Log::LT_ERROR, "Failed to load font face %s (from %s).", font_family.c_str(), source.c_str());
		return false;
	}

	if (font_family.empty())
	{
		FreeType::GetFaceStyle(ft_face, font_family, style, weight);
	}

	if (!AddFace(ft_face, font_family, style, weight, fallback_face, local_data))
	{
		Log::Message(Log::LT_ERROR, "Failed to load font face %s (from %s).", font_family.c_str(), source.c_str());
		return false;
	}

	Log::Message(Log::LT_INFO, "Loaded font face %s (from %s).", font_family.c_str(), source.c_str());
	return true;
}

bool FontProvider::AddFace(FontFaceHandleFreetype face, const String& family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face, bool release_stream)
{
	String family_lower = StringUtilities::ToLower(family);
	FontFamily* font_family = nullptr;
	auto it = font_families.find(family_lower);
	if (it != font_families.end())
	{
		font_family = (FontFamily*)it->second.get();
	}
	else
	{
		auto font_family_ptr = std::make_unique<FontFamily>(family_lower);
		font_family = font_family_ptr.get();
		font_families[family_lower] = std::move(font_family_ptr);
	}

	FontFace* font_face_result = font_family->AddFace(face, style, weight, release_stream);

	if (font_face_result && fallback_face)
	{
		auto it_fallback_face = std::find(fallback_font_faces.begin(), fallback_font_faces.end(), font_face_result);
		if (it_fallback_face == fallback_font_faces.end())
		{
			fallback_font_faces.push_back(font_face_result);
		}
	}

	return static_cast<bool>(font_face_result);
}


}
}
