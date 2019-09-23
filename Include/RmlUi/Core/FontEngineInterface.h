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

#ifndef RMLUICOREFONTENGINEINTERFACE_H
#define RMLUICOREFONTENGINEINTERFACE_H

#include "Header.h"
#include "Types.h"
#include "Geometry.h"

namespace Rml {
namespace Core {

class RMLUICORE_API FontEngineInterface
{
public:
	FontEngineInterface();
	virtual ~FontEngineInterface();

	/// Called by the RmlUi when it wants to load a font face from file.
	/// @param[in] file_name The file to load the face from.
	/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
	/// @return True if the face was loaded successfully, false otherwise.
	virtual bool LoadFontFace(const String& file_name, bool fallback_face);

	/// Called by the RmlUi when it wants to load a font face from memory, registered using the provided family, style, and weight.
	/// @param[in] data A pointer to the data.
	/// @param[in] data_size Size of the data in bytes.
	/// @param[in] family The family to register the font as.
	/// @param[in] style The style to register the font as.
	/// @param[in] weight The weight to register the font as.
	/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
	/// @return True if the face was loaded successfully, false otherwise.
	/// Note: The debugger plugin will load its embedded font faces through this method using the family name 'rmlui-debugger-font'.
	virtual bool LoadFontFace(const byte* data, int data_size, const String& family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face);

	/// Called by the RmlUi when a font configuration is resolved for an element. Should return a handle that 
	/// can later be used to resolve properties of the face, and generate strings which can later be rendered.
	/// @param[in] family The family of the desired font handle.
	/// @param[in] style The style of the desired font handle.
	/// @param[in] weight The weight of the desired font handle.
	/// @param[in] size The size of desired handle, in points. // TODO: Pixels?
	/// @return A valid handle if a matching (or closely matching) font face was found, NULL otherwise.
	virtual FontFaceHandle GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size);

	/// Called by RmlUi when it wants to configure the layers such that font effects can be applied.
	/// If font effects are not needed, it should return zero.
	/// @param[in] handle The font handle.
	/// @param[in] font_effects The list of font effects to generate the configuration for.
	/// @return The index to use when generating geometry using this configuration.
	// TODO: Should return a handle.
	virtual int GenerateLayerConfiguration(FontFaceHandle handle, const FontEffectList &font_effects);

	/// Should return the average advance of all glyphs in this font face.
	/// @param[in] handle The font handle.
	/// @return An approximate width of the characters in this font face.
	// TODO: Not used, remove?
	virtual int GetCharacterWidth(FontFaceHandle handle);

	/// Should returns the point size of this font face.
	/// @param[in] handle The font handle.
	/// @return The face's point size.
	virtual int GetSize(FontFaceHandle handle);
	/// Should return the pixel height of a lower-case x in this font face.
	/// @param[in] handle The font handle.
	/// @return The height of a lower-case x.
	virtual int GetXHeight(FontFaceHandle handle);
	/// Should return the default height between this font face's baselines.
	/// @param[in] handle The font handle.
	/// @return The default line height.
	virtual int GetLineHeight(FontFaceHandle handle);

	/// Should return the font's baseline, as a pixel offset from the bottom of the font.
	/// @param[in] handle The font handle.
	/// @return The font's baseline.
	virtual int GetBaseline(FontFaceHandle handle);

	/// Should return the font's underline, as a pixel offset from the bottom of the font.
	/// @param[in] handle The font handle.
	/// @return The font's underline thickness.
	// TODO: Thickness vs Return value? Pointer?
	virtual float GetUnderline(FontFaceHandle handle, float *thickness);

	/// Called by RmlUi when it wants to retrieve the width of a string when rendered with this handle.
	/// @param[in] handle The font handle.
	/// @param[in] string The string to measure.
	/// @param[in] prior_character The optionally-specified character that immediately precedes the string. This may have an impact on the string width due to kerning.
	/// @return The width, in pixels, this string will occupy if rendered with this handle.
	virtual int GetStringWidth(FontFaceHandle handle, const String& string, CodePoint prior_character = CodePoint::Null);

	/// Called by RmlUi when it wants to retrieve the geometry required to render a single line of text.
	/// @param[in] handle The font handle.
	/// @param[out] geometry An array of geometries to generate the geometry into.
	/// @param[in] handle The font handle.
	/// @param[in] string The string to render.
	/// @param[in] position The position of the baseline of the first character to render.
	/// @param[in] colour The colour to render the text.
	/// @param[in] layer_configuration The layer for which the geometry should be generated.
	/// @return The width, in pixels, of the string geometry.
	// TODO: Layer configuration should be a handle. Reorder arguments to take the handles first, geometry last/first.
	virtual int GenerateString(FontFaceHandle handle, GeometryList& geometry, const String& string, const Vector2f& position, const Colourb& colour, int layer_configuration);

	/// Should return a new value whenever the geometry need to be re-generated.
	virtual int GetVersion(FontFaceHandle handle);

};

}
}

#endif
