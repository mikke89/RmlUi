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

#ifndef RMLUICOREFONTDATABASE_H
#define RMLUICOREFONTDATABASE_H

#include "StringUtilities.h"
#include "Header.h"
#include "FontProvider.h"

namespace Rml {
namespace Core {

class FontEffect;
class FontFamily;
class FontFaceHandle;
class PropertyDictionary;

/**
	The font database contains all font families currently in use by RmlUi.

	@author Peter Curry
 */

class RMLUICORE_API FontDatabase
{
public:

    enum FontProviderType
    {
        FreeType = 0,
        BitmapFont
    };

	static bool Initialise();
	static void Shutdown();

	/// Adds a new font face to the database. The face's family, style and weight will be determined from the face itself.
	/// @param[in] file_name The file to load the face from.
	/// @return True if the face was loaded successfully, false otherwise.
	static bool LoadFontFace(const String& file_name);
	/// Adds a new font face to the database, ignoring any family, style and weight information stored in the face itself.
	/// @param[in] file_name The file to load the face from.
	/// @param[in] family The family to add the face to.
	/// @param[in] style The style of the face (normal or italic).
	/// @param[in] weight The weight of the face (normal or bold).
	/// @return True if the face was loaded successfully, false otherwise.
	static bool LoadFontFace(const String& file_name, const String& family, Style::FontStyle style, Style::FontWeight weight);
	/// Adds a new font face to the database, loading from memory. The face's family, style and weight will be determined from the face itself.
	/// @param[in] data The font data.
	/// @param[in] data_length Length of the data.
	/// @return True if the face was loaded successfully, false otherwise.
    static bool LoadFontFace(FontProviderType font_provider_type, const byte* data, int data_length);
	/// Adds a new font face to the database, loading from memory.
	/// @param[in] data The font data.
	/// @param[in] data_length Length of the data.
	/// @param[in] family The family to add the face to.
	/// @param[in] style The style of the face (normal or italic).
	/// @param[in] weight The weight of the face (normal or bold).
	/// @return True if the face was loaded successfully, false otherwise.
    static bool LoadFontFace(FontProviderType font_provider_type, const byte* data, int data_length, const String& family, Style::FontStyle style, Style::FontWeight weight);

	/// Returns a handle to a font face that can be used to position and render text. This will return the closest match
	/// it can find, but in the event a font family is requested that does not exist, nullptr will be returned instead of a
	/// valid handle.
	/// @param[in] family The family of the desired font handle.
	/// @param[in] charset The set of characters required in the font face, as a comma-separated list of unicode ranges.
	/// @param[in] style The style of the desired font handle.
	/// @param[in] weight The weight of the desired font handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, nullptr otherwise.
	static SharedPtr<FontFaceHandle> GetFontFaceHandle(const String& family, const String& charset, Style::FontStyle style, Style::FontWeight weight, int size);

    static void AddFontProvider(FontProvider * provider);

    static void RemoveFontProvider(FontProvider * provider);

private:
	FontDatabase(void);
	~FontDatabase(void);

    static FontProviderType GetFontProviderType(const String& file_name);

    typedef std::vector< FontProvider *> FontProviderTable;

    static FontProviderTable font_provider_table;
	static FontDatabase* instance;
};

}
}

#endif
