#pragma once

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

	/// Loads the current source file if needed. This normally happens automatically during layouting.
	void EnsureSourceLoaded();

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
