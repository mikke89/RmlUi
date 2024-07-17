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

#include "FontFamily.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "FontFace.h"
#include <limits.h>
#include "FontProvider.h"
#include <map>

namespace Rml {

FontFamily::FontFamily(const String& name) : name(name)
{}

FontFamily::~FontFamily()
{
	// Multiple face entries may share memory within a single font family, although only one of them owns it. Here we make sure that all the face
	// destructors are run before all the memory is released. This way we don't leave any hanging references to invalidated memory.
	for (FontFaceEntry& entry : font_faces)
		entry.face.reset();
}

// Returns a handle to the most appropriate font in the family, at the correct size.
FontFaceHandleDefault* FontFamily::GetFaceHandle(Style::FontStyle style, Style::FontWeight weight, int size)
{
	int best_dist = INT_MAX;
	FontFace* matching_face = nullptr;
	for (size_t i = 0; i < font_faces.size(); i++)
	{
		FontFace* face = font_faces[i].face.get();

		if (face->GetStyle() == style)
		{
			const int dist = Math::AbsoluteValue((int)face->GetWeight() - (int)weight);
			if (dist == 0)
			{
				// Direct match for weight, break the loop early.
				matching_face = face;
				break;
			}
			else if (dist < best_dist)
			{
				// Best match so far for weight, store the face and dist.
				matching_face = face;
				best_dist = dist;
			}
		}
	}

	if (!matching_face)
		return nullptr;

	return matching_face->GetHandle(size, true);
}

FontFace* FontFamily::GetFontFace(Style::FontStyle style, Style::FontWeight weight)
{
	for (auto& font_face : font_faces)
	{
		if (font_face.face->GetStyle() == style && font_face.face->GetWeight() == weight)
			return font_face.face.get();
	}

	return nullptr;
}

// Adds a new face to the family.
FontFace* FontFamily::AddFace(FontFaceHandleFreetype ft_face, Style::FontStyle style, Style::FontWeight weight, UniquePtr<byte[]> face_memory)
{
	auto face = MakeUnique<FontFace>(ft_face, style, weight);
	FontFace* result = face.get();

	font_faces.push_back(FontFaceEntry{std::move(face), std::move(face_memory)});

	return result;
}

void FontFamily::AddVirtualFace(const Vector<FontMatch>& font_matches)
{
	struct FontMatchKey 
	{
		std::string font_name;
		Style::FontStyle style;
		Style::FontWeight weight;

		bool operator<(const FontMatchKey& other) const
		{
			if (font_name < other.font_name)
				return true;
			if (font_name > other.font_name)
				return false;

			if (style < other.style)
				return true;
			if (style > other.style)
				return false;

			return weight < other.weight;
		}
	};

	// The value is a pair so we can also store the FontFace pointer, instead of querying it once again in the second loop.
	std::map<FontMatchKey, Pair<FontFace*, Vector<FontMatch>>> font_match_map;

	for(auto& font_match : font_matches)
	{
		if (font_match.character_ranges.first == -1 && font_match.character_ranges.second == -1)
		{
			Log::Message(Log::LT_WARNING, "Font match '%s' has empty character range [-1, -1]. Ignoring the font match...", font_match.font_name.c_str());
			continue;
		}

		FontFamily* fontFamily = FontProvider::GetFontFamily(font_match.font_name);
		if(!fontFamily)
		{
			Log::Message(Log::LT_ERROR, "Failed to find font family '%s'.", font_match.font_name.c_str());
			continue;
		}

		FontFace* faceHandle = fontFamily->GetFontFace(font_match.style, font_match.weight);
		if(!faceHandle)
		{
			//Log::Message(Log::LT_ERROR, "Failed to find font face with style '%s' and weight '%s'.", font_match.style, font_match.weight);
			continue;
		}
		
		FontMatchKey key = {font_match.font_name, font_match.style, font_match.weight};
		auto& font_map_value = font_match_map[key];
		font_map_value.first = faceHandle;
		font_map_value.second.push_back(font_match);
	}


	UnorderedSet<Character> character_set;
	for (auto& font_match_entry : font_match_map)
	{
		const FontMatchKey& font_match_key = font_match_entry.first;
		Pair<FontFace*, Vector<FontMatch>>& font_match_value = font_match_entry.second;
		Vector<FontMatch>& final_font_matches = font_match_value.second;

		std::sort(final_font_matches.begin(), final_font_matches.end(),[](const FontMatch& lhs, const FontMatch& rhs) { return lhs.character_ranges.first < rhs.character_ranges.first; });
		FontFaceHandleFreetype font_face_handle = font_match_value.first->GetFreeTypeHandle();

		auto face = MakeUnique<FontFace>(font_face_handle, font_match_key.style, font_match_key.weight, final_font_matches);
		font_faces.push_back(FontFaceEntry{std::move(face), nullptr});
	}
}

void FontFamily::ReleaseFontResources()
{
	for (auto& entry : font_faces)
		entry.face->ReleaseFontResources();
}

} // namespace Rml
