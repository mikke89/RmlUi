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

#ifndef RMLUICOREELEMENTIMAGE_H
#define RMLUICOREELEMENTIMAGE_H

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Texture.h"

namespace Rml {
namespace Core {

/**
	The 'img' element. The image element can have a rectangular sub-region of its source texture
	specified with the 'coords' attribute; the element will render this region rather than the
	entire image.

	The 'coords' attribute is similar to that of the HTML imagemap. It takes four comma-separated
	integer values, specifying the top-left and the bottom right of the region in
	pixel-coordinates, in that order. So for example, the attribute "coords" = "0, 10, 100, 210"
	will render a 100 x 200 region, beginning at (0, 10) and rendering through to (100, 210). No
	clamping to the dimensions of the source image will occur; rendered results in this case will
	depend on the texture addressing mode.

	The intrinsic dimensions of the image can now come from three different sources. They are
	used in the following order:

	1) 'width' / 'height' attributes if present
	2) pixel width / height given by the 'coords' attribute
	3) width / height of the source texture

	This has the result of sizing the element to the pixel-size of the rendered image, unless
	overridden by the 'width' or 'height' attributes.

	@author Peter Curry
 */

class RMLUICORE_API ElementImage : public Element
{
public:
	/// Constructs a new ElementImage. This should not be called directly; use the Factory instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementImage(const String& tag);
	virtual ~ElementImage();

	/// Returns the element's inherent size.
	/// @param[out] The element's intrinsic dimensions.
	/// @return True.
	bool GetIntrinsicDimensions(Vector2f& dimensions) override;

protected:
	/// Renders the image.
	void OnRender() override;

	/// Regenerates the element's geometry.
	void OnResize() override;

	/// Checks for changes to the image's source or dimensions.
	/// @param[in] changed_attributes A list of attributes changed on the element.
	void OnAttributeChange(const ElementAttributes& changed_attributes) override;

	/// Called when properties on the element are changed.
	/// @param[in] changed_properties The properties changed on the element.
	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

private:
	// Generates the element's geometry.
	void GenerateGeometry();
	// Loads the element's texture, as specified by the 'src' attribute.
	bool LoadTexture();
	// Resets the values of the 'coords' attribute to mark them as unused.
	void ResetCoords();

	// The texture this element is rendering from.
	Texture texture;
	// True if we need to refetch the texture's source from the element's attributes.
	bool texture_dirty;
	// The element's computed intrinsic dimensions. If either of these values are set to -1, then
	// that dimension has not been computed yet.
	Vector2f dimensions;

	// The coords extracted from the sprite or 'coords' attribute. The coords_source will be None if
	// these have not been specified or are invalid.
	Rectangle coords;
	enum class CoordsSource { None, Attribute, Sprite } coords_source;

	// The geometry used to render this element.
	Geometry geometry;
	bool geometry_dirty;
};

}
}

#endif
