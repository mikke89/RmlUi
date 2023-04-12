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

#ifndef RMLUI_CORE_FONTENGINEINTERFACE_H
#define RMLUI_CORE_FONTENGINEINTERFACE_H

#include "FontMetrics.h"
#include "Geometry.h"
#include "Header.h"
#include "StyleTypes.h"
#include "Types.h"

namespace Rml {

/**
    The abstract base class for an application-specific font engine implementation.

    By default, RmlUi will use its own font engine with characters rendered through FreeType. To use your own engine,
    provide a concrete implementation of this class and install it through Rml::SetFontEngineInterface().
 */

class RMLUICORE_API FontEngineInterface {
public:
	FontEngineInterface();
	virtual ~FontEngineInterface();

	/// Called by RmlUi when it wants to load a font face from file.
	/// @param[in] file_name The file to load the face from.
	/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
	/// @param[in] weight The weight to load when the font face contains multiple weights, otherwise the weight to register the font as.
	/// @return True if the face was loaded successfully, false otherwise.
	virtual bool LoadFontFace(const String& file_name, bool fallback_face, Style::FontWeight weight);

	/// Called by RmlUi when it wants to load a font face from memory, registered using the provided family, style, and weight.
	/// @param[in] data A pointer to the data.
	/// @param[in] data_size Size of the data in bytes.
	/// @param[in] family The family to register the font as.
	/// @param[in] style The style to register the font as.
	/// @param[in] weight The weight to load when the font face contains multiple weights, otherwise the weight to register the font as.
	/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
	/// @return True if the face was loaded successfully, false otherwise.
	/// Note: The debugger plugin will load its embedded font faces through this method using the family name 'rmlui-debugger-font'.
	virtual bool LoadFontFace(const byte* data, int data_size, const String& family, Style::FontStyle style, Style::FontWeight weight,
		bool fallback_face);

	/// Called by RmlUi when a font configuration is resolved for an element. Should return a handle that
	/// can later be used to resolve properties of the face, and generate string geometry to be rendered.
	/// @param[in] family The family of the desired font handle.
	/// @param[in] style The style of the desired font handle.
	/// @param[in] weight The weight of the desired font handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, NULL otherwise.
	virtual FontFaceHandle GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size);

	/// Called by RmlUi when a list of font effects is resolved for an element with a given font face.
	/// @param[in] handle The font handle.
	/// @param[in] font_effects The list of font effects to generate the configuration for.
	/// @return A handle to the prepared font effects which will be used when generating geometry for a string.
	virtual FontEffectsHandle PrepareFontEffects(FontFaceHandle handle, const FontEffectList& font_effects);

	/// Should return the font metrics of the given font face.
	/// @param[in] handle The font handle.
	/// @return The face's metrics.
	virtual const FontMetrics& GetFontMetrics(FontFaceHandle handle);

	/// Called by RmlUi when it wants to retrieve the width of a string when rendered with this handle.
	/// @param[in] handle The font handle.
	/// @param[in] string The string to measure.
	/// @param[in] letter_spacing The letter spacing size in pixels.
	/// @param[in] prior_character The optionally-specified character that immediately precedes the string. This may have an impact on the string
	/// width due to kerning.
	/// @return The width, in pixels, this string will occupy if rendered with this handle.
	virtual int GetStringWidth(FontFaceHandle handle, const String& string, float letter_spacing, Character prior_character = Character::Null);

	/// Called by RmlUi when it wants to retrieve the geometry required to render a single line of text.
	/// @param[in] face_handle The font handle.
	/// @param[in] font_effects_handle The handle to the prepared font effects for which the geometry should be generated.
	/// @param[in] string The string to render.
	/// @param[in] position The position of the baseline of the first character to render.
	/// @param[in] colour The colour to render the text. Colour alpha is premultiplied with opacity.
	/// @param[in] opacity The opacity of the text, should be applied to font effects.
	/// @param[in] letter_spacing The letter spacing size in pixels.
	/// @param[out] geometry An array of geometries to generate the geometry into.
	/// @return The width, in pixels, of the string geometry.
	virtual int GenerateString(FontFaceHandle face_handle, FontEffectsHandle font_effects_handle, const String& string, const Vector2f& position,
		const Colourb& colour, float opacity, float letter_spacing, GeometryList& geometry);

	/// Called by RmlUi to determine if the text geometry is required to be re-generated. Whenever the returned version
	/// is changed, all geometry belonging to the given face handle will be re-generated.
	/// @param[in] face_handle The font handle.
	/// @return The version required for using any geometry generated with the face handle.
	virtual int GetVersion(FontFaceHandle handle);

	/// Called by RmlUi when it wants to garbage collect memory used by fonts.
	/// @note All existing FontFaceHandles and FontEffectsHandles are considered invalid after this call.
	virtual void ReleaseFontResources();
};

} // namespace Rml
#endif
