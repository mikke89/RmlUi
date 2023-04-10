/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#ifndef RMLUI_CORE_ELEMENTS_ELEMENTIMAGE_H
#define RMLUI_CORE_ELEMENTS_ELEMENTIMAGE_H

#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/Geometry.h"
#include "../../../Include/RmlUi/Core/Header.h"
#include "../../../Include/RmlUi/Core/Spritesheet.h"
#include "../../../Include/RmlUi/Core/Texture.h"

namespace Rml {

/**
    The 'img' element can render images and sprites.

    The 'src' attribute is used to specify an image url. If instead the `sprite` attribute is set,
    it will load a sprite and ignore the `src` and `rect` attributes.

    The 'rect' attribute takes four space-separated	integer values, specifying a rectangle
    using 'x y width height' in pixel coordinates inside the image. No clamping to the
    dimensions of the source image will occur; rendered results in this case will
    depend on the user's texture addressing mode.

    The intrinsic dimensions of the image can now come from three different sources. They are
    used in the following order:

    1) 'width' / 'height' attributes if present
    2) pixel width / height of the sprite
    3) pixel width / height given by the 'rect' attribute
    4) width / height of the image texture

    This has the result of sizing the element to the pixel-size of the rendered image, unless
    overridden by the 'width' or 'height' attributes.

    @author Peter Curry
 */

class RMLUICORE_API ElementImage : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementImage, Element)

	/// Constructs a new ElementImage. This should not be called directly; use the Factory instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementImage(const String& tag);
	virtual ~ElementImage();

	/// Returns the element's inherent size.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

protected:
	/// Renders the image.
	void OnRender() override;

	/// Regenerates the element's geometry.
	void OnResize() override;

	/// Our intrinsic dimensions may change with the dp-ratio.
	void OnDpRatioChange() override;

	/// The sprite may have changed when the style sheet is recompiled.
	void OnStyleSheetChange() override;

	/// Checks for changes to the image's source or dimensions.
	/// @param[in] changed_attributes A list of attributes changed on the element.
	void OnAttributeChange(const ElementAttributes& changed_attributes) override;

	/// Called when properties on the element are changed.
	/// @param[in] changed_properties The properties changed on the element.
	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

	/// Detect when we have been added to the document.
	void OnChildAdd(Element* child) override;

private:
	// Generates the element's geometry.
	void GenerateGeometry();
	// Loads the element's texture, as specified by the 'src' attribute.
	bool LoadTexture();
	// Loads the rect value from the element's attribute, but only if we're not a sprite.
	void UpdateRect();

	// The texture this element is rendering from.
	Texture texture;
	// True if we need to refetch the texture's source from the element's attributes.
	bool texture_dirty;
	// A factor which scales the intrinsic dimensions based on the dp-ratio and image scale.
	float dimensions_scale;
	// The element's computed intrinsic dimensions. If either of these values are set to -1, then
	// that dimension has not been computed yet.
	Vector2f dimensions;

	// The rectangle extracted from the sprite or 'rect' attribute. The rect_source will be None if
	// these have not been specified or are invalid.
	Rectanglef rect;
	enum class RectSource { None, Attribute, Sprite } rect_source;

	// The geometry used to render this element.
	Geometry geometry;
	bool geometry_dirty;
};

} // namespace Rml
#endif
