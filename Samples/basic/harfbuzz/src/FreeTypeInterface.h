#include "FontEngineDefault/FontTypes.h"
#include "FontGlyph.h"
#include <RmlUi/Core.h>

using Rml::Character;
using Rml::FontFaceHandleFreetype;
using Rml::FontMetrics;

namespace FreeType {

// Initializes a face for a given font size. Glyphs are filled with the ASCII subset, and the font face metrics are set.
bool InitialiseFaceHandle(FontFaceHandleFreetype face, int font_size, FontGlyphMap& glyphs, FontMetrics& metrics, bool load_default_glyphs);

// Build a new glyph representing the given glyph index and append to 'glyphs'.
bool AppendGlyph(FontFaceHandleFreetype face, int font_size, FontGlyphIndex glyph_index, Character character, FontGlyphMap& glyphs);

// Returns the corresponding glyph index from a character code.
FontGlyphIndex GetGlyphIndexFromCharacter(FontFaceHandleFreetype face, Character character);

} // namespace FreeType
