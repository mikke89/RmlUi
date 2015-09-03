/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
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

#ifndef ROCKETCOREBITMAPFONTFACELAYER_H
#define ROCKETCOREBITMAPFONTFACELAYER_H

#include <Rocket/Core/Header.h>
#include <Rocket/Core/FontGlyph.h>
#include <Rocket/Core/Geometry.h>
#include <Rocket/Core/GeometryUtilities.h>
#include <Rocket/Core/String.h>
#include "../FontFaceLayer.h"

namespace Rocket {
namespace Core {

	class TextureLayout;

namespace BitmapFont {

/**
	A textured layer stored as part of a font face handle. Each handle will have at least a base
	layer for the standard font. Further layers can be added to allow to rendering of text effects.
	@author Peter Curry
 */

class FontFaceLayer : public Rocket::Core::FontFaceLayer
{
public:
	FontFaceLayer();
	virtual ~FontFaceLayer();

	/// Generates the character and texture data for the layer.
	/// @param[in] handle The handle generating this layer.
	/// @param[in] effect The effect to initialise the layer with.
	/// @param[in] clone The layer to optionally clone geometry and texture data from.
	/// @param[in] deep_clone If true, the clones geometry will be completely cloned and the effect will have no option to affect even the glyph origins.
	/// @return True if the layer was generated successfully, false if not.
	virtual bool Initialise(const FontFaceHandle* handle, FontEffect* effect = NULL, const Rocket::Core::FontFaceLayer* clone = NULL, bool deep_clone = false);

	/// Generates the texture data for a layer (for the texture database).
	/// @param[out] texture_data The pointer to be set to the generated texture data.
	/// @param[out] texture_dimensions The dimensions of the texture.
	/// @param[in] glyphs The glyphs required by the font face handle.
	/// @param[in] texture_id The index of the texture within the layer to generate.
	virtual bool GenerateTexture(const byte*& texture_data, Vector2i& texture_dimensions, int texture_id);

};

}
}
}

#endif
