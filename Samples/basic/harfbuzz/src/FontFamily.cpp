#include "FontFamily.h"
#include "FontFace.h"
#include "FontFaceHandleHarfBuzz.h"
#include <limits.h>

FontFamily::FontFamily(const String& name) : name(name) {}

FontFamily::~FontFamily()
{
	// Multiple face entries may share memory within a single font family, although only one of them owns it. Here we make sure that all the face
	// destructors are run before all the memory is released. This way we don't leave any hanging references to invalidated memory.
	for (FontFaceEntry& entry : font_faces)
		entry.face.reset();
}

FontFaceHandleHarfBuzz* FontFamily::GetFaceHandle(Style::FontStyle style, Style::FontWeight weight, int size)
{
	int best_dist = INT_MAX;
	FontFace* matching_face = nullptr;
	for (size_t i = 0; i < font_faces.size(); i++)
	{
		FontFace* face = font_faces[i].face.get();

		if (face->GetStyle() == style)
		{
			const int dist = Rml::Math::Absolute((int)face->GetWeight() - (int)weight);
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

FontFace* FontFamily::AddFace(FontFaceHandleFreetype ft_face, Style::FontStyle style, Style::FontWeight weight, UniquePtr<byte[]> face_memory)
{
	auto face = Rml::MakeUnique<FontFace>(ft_face, style, weight);
	FontFace* result = face.get();

	font_faces.push_back(FontFaceEntry{std::move(face), std::move(face_memory)});

	return result;
}

void FontFamily::ReleaseFontResources()
{
	for (auto& entry : font_faces)
		entry.face->ReleaseFontResources();
}
