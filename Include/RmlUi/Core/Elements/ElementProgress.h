#pragma once

#include "../Element.h"
#include "../Geometry.h"
#include "../Header.h"
#include "../Spritesheet.h"
#include "../Texture.h"

namespace Rml {

/**
    The 'progress' element.

    The 'value' attribute should be a number [0, 1] where 1 means completely filled.

    The 'direction' attribute should be one of:
        top | right (default) | bottom | left | clockwise | counter-clockwise

    The 'start-edge' attribute should be one of:
        top (default) | right | bottom | left
    Only applies to 'clockwise' or 'counter-clockwise' directions. Defines which edge the
    circle should start expanding from.

    The progress element generates a non-dom 'fill' element beneath it which can be used to
    style the filled part of the bar. The 'fill' element can use the 'fill-image'-property
    to set an image which will be clipped according to the progress value. This property is
    the only way to style a 'clockwise' or 'counter-clockwise' progress element.

 */

class RMLUICORE_API ElementProgress : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementProgress, Element)

	/// Constructs a new ElementProgress. This should not be called directly; use the Factory instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementProgress(const String& tag);
	virtual ~ElementProgress();

	/// Returns the value of the progress bar.
	float GetValue() const;
	/// Sets the value of the progress bar.
	void SetValue(float value);

	/// Returns the maximum value of the progress bar.
	float GetMax() const;
	/// Sets the maximum value of the progress bar.
	void SetMax(float max_value);

	/// Returns the element's inherent size.
	bool GetIntrinsicDimensions(Vector2f& dimensions, float& ratio) override;

protected:
	void OnRender() override;

	void OnResize() override;

	void OnAttributeChange(const ElementAttributes& changed_attributes) override;

	void OnPropertyChange(const PropertyIdSet& changed_properties) override;

private:
	enum class Direction { Top, Right, Bottom, Left, Clockwise, CounterClockwise, Count };
	enum class StartEdge { Top, Right, Bottom, Left, Count };

	static constexpr Direction DefaultDirection = Direction::Right;
	static constexpr StartEdge DefaultStartEdge = StartEdge::Top;

	void GenerateGeometry();
	bool LoadTexture();

	Direction direction;
	StartEdge start_edge;

	Element* fill;

	// The size of the fill geometry as if fully filled, and the offset relative to the 'progress' element.
	Vector2f fill_size, fill_offset;

	// The texture this element is rendering from if the 'fill-image' property is set.
	Texture texture;

	// The rectangle extracted from a sprite, 'rect_set' controls whether it is active.
	Rectanglef rect;
	bool rect_set;

	// The geometry used to render this element. Only applies if the 'fill-image' property is set.
	Geometry geometry;
	bool geometry_dirty;
};

} // namespace Rml
