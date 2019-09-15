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

#ifndef RMLUICOREFREETYPEFONTFACEHANDLE_H
#define RMLUICOREFREETYPEFONTFACEHANDLE_H

#include "../FontFaceHandle.h"
#include "../../../Include/RmlUi/Core/FontEffect.h"
#include "../../../Include/RmlUi/Core/FontGlyph.h"
#include "../../../Include/RmlUi/Core/Geometry.h"
#include "../../../Include/RmlUi/Core/Texture.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace Rml {
namespace Core {

/**
	@author Peter Curry
 */

class FontFaceHandle_FreeType : public Rml::Core::FontFaceHandle
{
public:
	FontFaceHandle_FreeType();
	virtual ~FontFaceHandle_FreeType();

	/// Initialises the handle so it is able to render text.
	/// @param[in] ft_face The FreeType face that this handle is rendering.
	/// @param[in] size The size, in points, of the face this handle should render at.
	/// @return True if the handle initialised successfully and is ready for rendering, false if an error occured.
	bool Initialise(FT_Face ft_face, int size);

private:
	int GetKerning(CodePoint lhs, CodePoint rhs) const override;

	FT_Face ft_face;
};

}
}

#endif
