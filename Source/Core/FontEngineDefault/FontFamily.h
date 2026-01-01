#pragma once

#include "FontTypes.h"

namespace Rml {

class FontFace;
class FontFaceHandleDefault;

class FontFamily {
public:
	FontFamily(const String& name);
	~FontFamily();

	/// Returns a handle to the most appropriate font in the family, at the correct size.
	/// @param[in] style The style of the desired handle.
	/// @param[in] weight The weight of the desired handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, nullptr otherwise.
	FontFaceHandleDefault* GetFaceHandle(Style::FontStyle style, Style::FontWeight weight, int size);

	/// Adds a new face to the family.
	/// @param[in] ft_face The previously loaded FreeType face.
	/// @param[in] style The style of the new face.
	/// @param[in] weight The weight of the new face.
	/// @param[in] face_memory Optionally pass ownership of the face's memory to the face itself, automatically releasing it on destruction.
	/// @return True if the face was loaded successfully, false otherwise.
	FontFace* AddFace(FontFaceHandleFreetype ft_face, Style::FontStyle style, Style::FontWeight weight, UniquePtr<byte[]> face_memory);

	/// Releases resources owned by sized font faces, including their textures and rendered glyphs.
	void ReleaseFontResources();

protected:
	String name;

	struct FontFaceEntry {
		UniquePtr<FontFace> face;
		// Only filled if we own the memory used by the face's FreeType handle. May be shared with other faces in this family.
		UniquePtr<byte[]> face_memory;
	};

	using FontFaceList = Vector<FontFaceEntry>;
	FontFaceList font_faces;
};

} // namespace Rml
