/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef RMLUICOREFONTENGINEINTERFACEDEFAULT_H
#define RMLUICOREFONTENGINEINTERFACEDEFAULT_H

#include "../../../Include/RmlUi/Core/FontEngineInterface.h"

namespace Rml {
namespace Core {

class RMLUICORE_API FontEngineInterfaceDefault : public FontEngineInterface
{
public:
	FontEngineInterfaceDefault();
	virtual ~FontEngineInterfaceDefault();

	/// Adds a new font face to the database. The face's family, style and weight will be determined from the face itself.
	bool LoadFontFace(const String& file_name, bool fallback_face) override;

	/// Adds a new font face to the database using the provided family, style and weight.
	bool LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face) override;

	/// Returns a handle to a font face that can be used to position and render text. This will return the closest match
	/// it can find, but in the event a font family is requested that does not exist, NULL will be returned instead of a
	/// valid handle.
	FontFaceHandle GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size) override;

	/// Prepares for font effects by configuring a new, or returning an existing, layer configuration.
	FontEffectsHandle PrepareFontEffects(FontFaceHandle, const FontEffectList& font_effects) override;

	/// Returns the point size of this font face.
	int GetSize(FontFaceHandle) override;
	/// Returns the pixel height of a lower-case x in this font face.
	int GetXHeight(FontFaceHandle) override;
	/// Returns the default height between this font face's baselines.
	int GetLineHeight(FontFaceHandle) override;

	/// Returns the font's baseline, as a pixel offset from the bottom of the font.
	int GetBaseline(FontFaceHandle) override;

	/// Returns the font's underline, as a pixel offset from the bottom of the font.
	float GetUnderline(FontFaceHandle, float& thickness) override;

	/// Returns the width a string will take up if rendered with this handle.
	int GetStringWidth(FontFaceHandle, const String& string, Character prior_character) override;

	/// Generates the geometry required to render a single line of text.
	int GenerateString(FontFaceHandle, FontEffectsHandle, const String& string, const Vector2f& position, const Colourb& colour, GeometryList& geometry) override;

	/// Returns the current version of the font face.
	int GetVersion(FontFaceHandle handle) override;
};

}
}

#endif
