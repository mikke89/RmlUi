#pragma once

#include "FontEngineDefault/FontTypes.h"
#include <RmlUi/Core.h>

using Rml::byte;
using Rml::FontFaceHandleFreetype;
using Rml::Span;
using Rml::String;
using Rml::UniquePtr;
using Rml::UnorderedMap;
using Rml::Vector;
namespace Style = Rml::Style;

class FontFace;
class FontFamily;
class FontFaceHandleHarfBuzz;

/**
    The font provider contains all font families currently in use by RmlUi.
    Modified to support HarfBuzz text shaping.
 */

class FontProvider {
public:
	static bool Initialise();
	static void Shutdown();

	/// Returns a handle to a font face that can be used to position and render text. This will return the closest match
	/// it can find, but in the event a font family is requested that does not exist, nullptr will be returned instead of a
	/// valid handle.
	/// @param[in] family The family of the desired font handle.
	/// @param[in] style The style of the desired font handle.
	/// @param[in] weight The weight of the desired font handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, nullptr otherwise.
	static FontFaceHandleHarfBuzz* GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size);

	/// Adds a new font face to the database. The face's family, style and weight will be determined from the face itself.
	static bool LoadFontFace(const String& file_name, int face_index, bool fallback_face, Style::FontWeight weight = Style::FontWeight::Auto);

	/// Adds a new font face from memory.
	static bool LoadFontFace(Span<const byte> data, int face_index, const String& font_family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face);

	/// Return the number of fallback font faces.
	static int CountFallbackFontFaces();

	/// Return a font face handle with the given index, at the given font size.
	static FontFaceHandleHarfBuzz* GetFallbackFontFace(int index, int font_size);

	/// Releases resources owned by sized font faces, including their textures and rendered glyphs.
	static void ReleaseFontResources();

private:
	FontProvider();
	~FontProvider();

	static FontProvider& Get();

	bool LoadFontFace(Span<const byte> data, int face_index, bool fallback_face, UniquePtr<byte[]> face_memory, const String& source, String font_family,
		Style::FontStyle style,
		Style::FontWeight weight);

	bool AddFace(FontFaceHandleFreetype face, const String& family, Style::FontStyle style, Style::FontWeight weight,
		bool fallback_face, UniquePtr<byte[]> face_memory);

	using FontFaceList = Vector<FontFace*>;
	using FontFamilyMap = UnorderedMap<String, UniquePtr<FontFamily>>;

	FontFamilyMap font_families;
	FontFaceList fallback_font_faces;

	static const String debugger_font_family_name;
};
