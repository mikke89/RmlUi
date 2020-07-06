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

#ifndef RMLUI_CORE_FONTENGINEDEFAULT_FREETYPEINTERFACE_H
#define RMLUI_CORE_FONTENGINEDEFAULT_FREETYPEINTERFACE_H

#include "FontTypes.h"

namespace Rml {

namespace FreeType {

// Initialize FreeType library.
bool Initialise();
// Shutdown FreeType library.
void Shutdown();

// Loads a FreeType face from memory, 'source' is only used for logging.
FontFaceHandleFreetype LoadFace(const byte* data, int data_length, const String& source);

// Releases the FreeType face.
bool ReleaseFace(FontFaceHandleFreetype face, bool release_stream);

// Retrieves the font family, style and weight of the given font face.
void GetFaceStyle(FontFaceHandleFreetype face, String& font_family, Style::FontStyle& style, Style::FontWeight& weight);

// Initializes a face for a given font size. Glyphs are filled with the ASCII subset, and the font face metrics are set.
bool InitialiseFaceHandle(FontFaceHandleFreetype face, int font_size, FontGlyphMap& glyphs, FontMetrics& metrics);

// Build a new glyph representing the given code point and append to 'glyphs'.
bool AppendGlyph(FontFaceHandleFreetype face, int font_size, Character character, FontGlyphMap& glyphs);

// Returns the kerning between two characters.
int GetKerning(FontFaceHandleFreetype face, int font_size, Character lhs, Character rhs);

}
} // namespace Rml
#endif
