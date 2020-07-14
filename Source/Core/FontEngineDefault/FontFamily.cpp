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
#include "FontFace.h"

namespace Rml {

FontFamily::FontFamily(const String& name) : name(name)
{
}

// Returns a handle to the most appropriate font in the family, at the correct size.
FontFaceHandleDefault* FontFamily::GetFaceHandle(Style::FontStyle style, Style::FontWeight weight, int size)
{
	// Search for a face of the same style, and match the weight as closely as we can.
	FontFace* matching_face = nullptr;
	for (size_t i = 0; i < font_faces.size(); i++)
	{
		// If we've found a face matching the style, then ... great! We'll match it regardless of the weight. However,
		// if it's a perfect match, then we'll stop looking altogether.
		if (font_faces[i]->GetStyle() == style)
		{
			matching_face = font_faces[i].get();

			if (font_faces[i]->GetWeight() == weight)
				break;
		}
	}

	if (matching_face == nullptr)
		return nullptr;

	return matching_face->GetHandle(size);
}


// Adds a new face to the family.
FontFace* FontFamily::AddFace(FontFaceHandleFreetype ft_face, Style::FontStyle style, Style::FontWeight weight, bool release_stream)
{
	auto face = MakeUnique<FontFace>(ft_face, style, weight, release_stream);
	FontFace* result = face.get();

	font_faces.push_back(std::move(face));

	return result;
}

} // namespace Rml
