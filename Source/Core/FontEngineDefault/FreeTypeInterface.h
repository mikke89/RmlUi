#pragma once

#include "../../../Include/RmlUi/Core/FontMetrics.h"
#include "FontTypes.h"

namespace Rml {

namespace FreeType {

	// Initialize FreeType library.
	bool Initialise();
	// Shutdown FreeType library.
	void Shutdown();

	// Returns a sorted list of available font variations for the font face located in memory.
	bool GetFaceVariations(Span<const byte> data, Vector<FaceVariation>& out_face_variations, int face_index);

	// Loads a FreeType face from memory, 'source' is only used for logging.
	FontFaceHandleFreetype LoadFace(Span<const byte> data, const String& source, int face_index, int named_instance_index = 0);

	// Releases the FreeType face.
	bool ReleaseFace(FontFaceHandleFreetype face);

	// Retrieves the font family, style and weight of the given font face. Use nullptr to ignore a property.
	void GetFaceStyle(FontFaceHandleFreetype face, String* font_family, Style::FontStyle* style, Style::FontWeight* weight);

	// Initializes a face for a given font size. Glyphs are filled with the ASCII subset, and the font face metrics are set.
	bool InitialiseFaceHandle(FontFaceHandleFreetype face, int font_size, FontGlyphMap& glyphs, FontMetrics& metrics, bool load_default_glyphs);

	// Build a new glyph representing the given code point and append to 'glyphs'.
	bool AppendGlyph(FontFaceHandleFreetype face, int font_size, Character character, FontGlyphMap& glyphs);

	// Returns the kerning between two characters.
	// 'font_size' value of zero assumes the font size is already set on the face, and skips this step for performance reasons.
	int GetKerning(FontFaceHandleFreetype face, int font_size, Character lhs, Character rhs);

	// Returns true if the font face has kerning.
	bool HasKerning(FontFaceHandleFreetype face);

} // namespace FreeType
} // namespace Rml
