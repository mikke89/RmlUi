/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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
#include "FontEngineDefault/../ComputeProperty.h"
#include "FontEngineDefault/FreeTypeInterface.h"
#include "FontFace.h"
#include "FontFaceHandleHarfBuzz.h"
#include "FontFamily.h"
#include <algorithm>

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
	if (!Rml::FreeType::Initialise())
		return false;
	g_font_provider = new FontProvider;
	return true;
}

void FontProvider::Shutdown()
{
	RMLUI_ASSERT(g_font_provider);
	delete g_font_provider;
	g_font_provider = nullptr;
	Rml::FreeType::Shutdown();
}

FontProvider& FontProvider::Get()
{
	RMLUI_ASSERT(g_font_provider);
	return *g_font_provider;
}

FontFaceHandleHarfBuzz* FontProvider::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
	RMLUI_ASSERTMSG(family == Rml::StringUtilities::ToLower(family), "Font family name must be converted to lowercase before entering here.");

	FontFamilyMap& families = Get().font_families;

	auto it = families.find(family);
	if (it == families.end())
		return nullptr;

	return it->second->GetFaceHandle(style, weight, size);
}

void FontProvider::ReleaseFontResources()
{
	RMLUI_ASSERT(g_font_provider);
	for (auto& name_family : g_font_provider->font_families)
		name_family.second->ReleaseFontResources();
}

bool FontProvider::LoadFontFace(const String& file_name, Style::FontWeight weight)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle handle = file_interface->Open(file_name);

	if (!handle)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to load font face from %s, could not open file.", file_name.c_str());
		return false;
	}

	size_t length = file_interface->Length(handle);

	auto buffer_ptr = UniquePtr<byte[]>(new byte[length]);
	byte* buffer = buffer_ptr.get();
	file_interface->Read(buffer, length, handle);
	file_interface->Close(handle);

	bool result = Get().LoadFontFace(buffer, (int)length, std::move(buffer_ptr), file_name, {}, Style::FontStyle::Normal, weight);

	return result;
}

bool FontProvider::LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight)
{
	const String source = "memory";

	bool result = Get().LoadFontFace(data, data_size, nullptr, source, font_family, style, weight);

	return result;
}

bool FontProvider::LoadFontFace(const byte* data, int data_size, UniquePtr<byte[]> face_memory, const String& source, String font_family,
	Style::FontStyle style, Style::FontWeight weight)
{
	using Style::FontWeight;

	Vector<Rml::FaceVariation> face_variations;
	if (!Rml::FreeType::GetFaceVariations(data, data_size, face_variations))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to load font face from '%s': Invalid or unsupported font face file format.", source.c_str());
		return false;
	}

	Vector<Rml::FaceVariation> load_variations;
	if (face_variations.empty())
	{
		load_variations.push_back(Rml::FaceVariation{Style::FontWeight::Auto, 0, 0});
	}
	else
	{
		// Iterate through all the face variations and pick the ones to load. The list is already sorted by (weight, width). When weight is set to
		// 'auto' we load all the weights of the face. However, we only want to load one width for each weight.
		for (auto it = face_variations.begin(); it != face_variations.end();)
		{
			if (weight != FontWeight::Auto && it->weight != weight)
			{
				++it;
				continue;
			}

			// We don't currently have any way for users to select widths, so we search for a regular (medium) value here.
			constexpr int search_width = 100;
			const FontWeight current_weight = it->weight;

			int best_width_distance = Rml::Math::Absolute((int)it->width - search_width);
			auto it_best_width = it;

			// Search forward to find the best 'width' with the same weight.
			for (++it; it != face_variations.end(); ++it)
			{
				if (it->weight != current_weight)
					break;

				const int width_distance = Rml::Math::Absolute((int)it->width - search_width);
				if (width_distance < best_width_distance)
				{
					best_width_distance = width_distance;
					it_best_width = it;
				}
			}

			load_variations.push_back(*it_best_width);
		}
	}

	if (load_variations.empty())
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to load font face from '%s': Could not locate face with weight %d.", source.c_str(),
			(int)weight);
		return false;
	}

	for (const Rml::FaceVariation& variation : load_variations)
	{
		FontFaceHandleFreetype ft_face = Rml::FreeType::LoadFace(data, data_size, source, variation.named_instance_index);
		if (!ft_face)
			return false;

		if (font_family.empty())
			Rml::FreeType::GetFaceStyle(ft_face, &font_family, &style, nullptr);
		if (weight == FontWeight::Auto)
			Rml::FreeType::GetFaceStyle(ft_face, nullptr, nullptr, &weight);

		const FontWeight variation_weight = (variation.weight == FontWeight::Auto ? weight : variation.weight);
		const String font_face_description = Rml::GetFontFaceDescription(font_family, style, variation_weight);

		if (!AddFace(ft_face, font_family, style, variation_weight, std::move(face_memory)))
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to load font face %s from '%s'.", font_face_description.c_str(), source.c_str());
			return false;
		}

		Rml::Log::Message(Rml::Log::LT_INFO, "Loaded font face %s from '%s'.", font_face_description.c_str(), source.c_str());
	}

	return true;
}

bool FontProvider::AddFace(FontFaceHandleFreetype face, const String& family, Style::FontStyle style, Style::FontWeight weight,
	UniquePtr<byte[]> face_memory)
{
	if (family.empty() || weight == Style::FontWeight::Auto)
		return false;

	String family_lower = Rml::StringUtilities::ToLower(family);
	FontFamily* font_family = nullptr;
	auto it = font_families.find(family_lower);
	if (it != font_families.end())
	{
		font_family = (FontFamily*)it->second.get();
	}
	else
	{
		auto font_family_ptr = Rml::MakeUnique<FontFamily>(family_lower);
		font_family = font_family_ptr.get();
		font_families[family_lower] = std::move(font_family_ptr);
	}

	FontFace* font_face_result = font_family->AddFace(face, style, weight, std::move(face_memory));
	return static_cast<bool>(font_face_result);
}
