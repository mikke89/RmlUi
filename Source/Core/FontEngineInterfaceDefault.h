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

#include "../../Include/RmlUi/Core/FontEngineInterface.h"

namespace Rml {
namespace Core {

class RMLUICORE_API FontEngineInterfaceDefault: public FontEngineInterface
{
public:
	FontEngineInterfaceDefault();
	virtual ~FontEngineInterfaceDefault();

	/// Adds a new font face to the database. The face's family, style and weight will be determined from the face itself.
	/// @param[in] file_name The file to load the face from.
	/// @return True if the face was loaded successfully, false otherwise.
	virtual bool LoadFontFace(const String& file_name) override;

	/// Returns a handle to a font face that can be used to position and render text. This will return the closest match
	/// it can find, but in the event a font family is requested that does not exist, NULL will be returned instead of a
	/// valid handle.
	/// @param[in] family The family of the desired font handle.
	/// @param[in] charset The set of characters required in the font face, as a comma-separated list of unicode ranges.
	/// @param[in] style The style of the desired font handle.
	/// @param[in] weight The weight of the desired font handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, NULL otherwise.
	virtual FontFaceHandle GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size) override;

	/// Generates, if required, the layer configuration for a given array of font effects.
    /// @param[in] FontHandle
	/// @param[in] font_effects The list of font effects to generate the configuration for.
	/// @return The index to use when generating geometry using this configuration.
	virtual int GenerateLayerConfiguration(FontFaceHandle, const FontEffectList& font_effects) const override;

	/// Returns the average advance of all glyphs in this font face.
	/// @param[in] FontHandle
	/// @return An approximate width of the characters in this font face.
	virtual int GetCharacterWidth(FontFaceHandle) const override;

	/// Returns the point size of this font face.
	/// @param[in] FontHandle
	/// @return The face's point size.
	virtual int GetSize(FontFaceHandle) const override;
	/// Returns the pixel height of a lower-case x in this font face.
	/// @param[in] FontHandle
	/// @return The height of a lower-case x.
	virtual int GetXHeight(FontFaceHandle) const override;
	/// Returns the default height between this font face's baselines.
	/// @param[in] FontHandle
	/// @return The default line height.
	virtual int GetLineHeight(FontFaceHandle) const override;

	/// Returns the font's baseline, as a pixel offset from the bottom of the font.
	/// @param[in] FontHandle
	/// @return The font's baseline.
	virtual int GetBaseline(FontFaceHandle) const override;

	/// Returns the font's underline, as a pixel offset from the bottom of the font.
	/// @param[in] FontHandle
	/// @return The font's underline thickness.
	virtual float GetUnderline(FontFaceHandle, float *thickness) const override;

	/// Returns the width a string will take up if rendered with this handle.
	/// @param[in] FontHandle
	/// @param[in] string The string to measure.
	/// @param[in] prior_character The optionally-specified character that immediately precedes the string. This may have an impact on the string width due to kerning.
	/// @return The width, in pixels, this string will occupy if rendered with this handle.
	virtual int GetStringWidth(FontFaceHandle, const String& string, CodePoint prior_character) override;

	/// Generates the geometry required to render a single line of text.
	/// @param[out] geometry An array of geometries to generate the geometry into.
	/// @param[in] FontHandle
	/// @param[in] string The string to render.
	/// @param[in] position The position of the baseline of the first character to render.
	/// @param[in] colour The colour to render the text.
	/// @return The width, in pixels, of the string geometry.
	virtual int GenerateString(FontFaceHandle, GeometryList& geometry, const String& string, const Vector2f& position, const Colourb& colour, int layer_configuration) const override;
};

}
}

#endif
