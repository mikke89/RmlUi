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

#ifndef RMLUICOREFREETYPEFONTPROVIDER_H
#define RMLUICOREFREETYPEFONTPROVIDER_H

#ifndef RMLUI_NO_FONT_INTERFACE_DEFAULT

#include <RmlUi/Core/StringUtilities.h>
#include "../FontProvider.h"

namespace Rml {
namespace Core {

/**
    The font database contains all font families currently in use by RmlUi.
    @author Peter Curry
 */

class RMLUICORE_API FontProvider_FreeType : public Rml::Core::FontProvider
{
public:
	static bool Initialise();
	static void Shutdown();

	/// Adds a new font face to the database. The face's family, style and weight will be determined from the face itself.
	/// @param[in] file_name The file to load the face from.
	/// @return True if the face was loaded successfully, false otherwise.
	static bool LoadFontFace(const String& file_name, bool fallback_face);

private:
	FontProvider_FreeType(void);
	~FontProvider_FreeType(void);

	// Adds a loaded face to the appropriate font family.
	bool AddFace(void* face, const String& family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face, bool release_stream);
	// Loads a FreeType face.
	void* LoadFace(const String& file_name);
	// Loads a FreeType face from memory.
	void* LoadFace(const byte* data, int data_length, const String& source, bool local_data);

	static FontProvider_FreeType* instance;
};

}
}

#endif

#endif
