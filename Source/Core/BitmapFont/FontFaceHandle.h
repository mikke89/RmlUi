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

#ifndef RMLUICOREBITMAPFONTFONTFACEHANDLE_H
#define RMLUICOREBITMAPFONTFONTFACEHANDLE_H

#include "../UnicodeRange.h"
#include "../../../Include/RmlUi/Core/Font.h"
#include "../../../Include/RmlUi/Core/FontEffect.h"
#include "../../../Include/RmlUi/Core/FontGlyph.h"
#include "../../../Include/RmlUi/Core/Geometry.h"
#include "../../../Include/RmlUi/Core/String.h"
#include "../../../Include/RmlUi/Core/Texture.h"
#include "../FontFaceHandle.h"
#include "BitmapFontDefinitions.h"

namespace Rml {
namespace Core {
namespace BitmapFont {

/**
	@author Peter Curry
 */

class FontFaceHandle : public Rml::Core::FontFaceHandle
{
public:
	FontFaceHandle();
	virtual ~FontFaceHandle();

	/// Initialises the handle so it is able to render text.
	bool Initialise(BitmapFontDefinitions *bm_face, const String& charset, int size);

	const String & GetTextureSource() const
	{
		return texture_source;
	}

	unsigned int GetTextureWidth() const
	{
		return texture_width;
	}

	unsigned int GetTextureHeight() const
	{
		return texture_height;
	}

protected:
	Rml::Core::FontFaceLayer* CreateNewLayer() override;

private:
	void GenerateMetrics(BitmapFontDefinitions *bm_face);

	void BuildGlyphMap(BitmapFontDefinitions *bm_face, const UnicodeRange& unicode_range);
	void BuildGlyph(FontGlyph& glyph, CharacterInfo *ft_glyph);
	int GetKerning(word lhs, word rhs) const;

	BitmapFontDefinitions * bm_face;
	String texture_source;
	String texture_directory;
	unsigned int texture_width;
	unsigned int texture_height;
};

}
}
}

#endif
