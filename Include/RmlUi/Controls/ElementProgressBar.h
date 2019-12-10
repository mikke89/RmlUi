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

#ifndef RMLUICONTROLSELEMENTPROGRESSBAR_H
#define RMLUICONTROLSELEMENTPROGRESSBAR_H

#include "Header.h"
#include "../Core/Element.h"
#include "../Core/Geometry.h"
#include "../Core/Texture.h"
#include "../Core/Spritesheet.h"

namespace Rml {
namespace Controls {

/**
	The 'progressbar' element.

	The 'value' attribute should be a number [0, 1] where 1 means completely filled.

	The 'direction' attribute should be one of:
		top | right (default) | bottom | left | clockwise | counter-clockwise

	The 'start-edge' attribute should be one of:
		top (default) | right | bottom | left
	Only applies to 'clockwise' or 'counter-clockwise' directions. Defines which edge the
	circle should start expanding from.

	The progressbar generates a non-dom 'fill' element beneath it which can be used to style
	the filled part of the bar. The 'fill' element can use the 'fill-image'-property to set
	an image which will be clipped according to the progressbar value. This property is the
	only way to style a 'clockwise' or 'counter-clockwise' progressbar.

 */

class RMLUICONTROLS_API ElementProgressBar : public Core::Element
{
public:
	RMLUI_RTTI_DefineWithParent(ElementProgressBar, Core::Element)

	/// Constructs a new ElementProgressBar. This should not be called directly; use the Factory instead.
	/// @param[in] tag The tag the element was declared as in RML.
	ElementProgressBar(const Core::String& tag);
	virtual ~ElementProgressBar();

	/// Return the value of the progress bar [0, 1]
	float GetValue() const;

	/// Set the value of the progress bar
	void SetValue(float value);

protected:
	void OnRender() override;

	void OnResize() override;

	void OnAttributeChange(const Core::ElementAttributes& changed_attributes) override;

	void OnPropertyChange(const Core::PropertyIdSet& changed_properties) override;

private:
	enum class Direction { Top, Right, Bottom, Left, Clockwise, CounterClockwise, Count };
	enum class StartEdge { Top, Right, Bottom, Left, Count };

	static constexpr Direction DefaultDirection = Direction::Right;
	static constexpr StartEdge DefaultStartEdge = StartEdge::Top;

	void GenerateGeometry();
	bool LoadTexture();

	Direction direction;
	StartEdge start_edge;

	float value;

	Core::Element* fill;

	// The size of the fill geometry as if fully filled, and the offset relative to the 'progressbar' element.
	Core::Vector2f fill_size, fill_offset;

	// The texture this element is rendering from if the 'fill-image' property is set.
	Core::Texture texture;
	bool texture_dirty;

	// The rectangle extracted from a sprite, 'rect_set' controls whether it is active.
	Core::Rectangle rect;
	bool rect_set;

	// The geometry used to render this element. Only applies if the 'fill-image' property is set.
	Core::Geometry geometry;
	bool geometry_dirty;
};

}
}

#endif
