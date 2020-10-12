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

#ifndef RMLUI_CORE_LAYOUTBLOCKBOXSPACE_H
#define RMLUI_CORE_LAYOUTBLOCKBOXSPACE_H

#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;
class LayoutBlockBox;

/**
	Each block box has a space object for managing the space occupied by its floating elements, and those of its
	ancestors as relevant.

	@author Peter Curry
 */

class LayoutBlockBoxSpace
{
public:
	LayoutBlockBoxSpace(LayoutBlockBox* parent);
	~LayoutBlockBoxSpace();

	/// Imports boxes from another block into this space.
	/// @param[in] space The space to import boxes from.
	void ImportSpace(const LayoutBlockBoxSpace& space);

	/// Generates the position for a box of a given size within our block box.
	/// @param[out] box_position The generated position for the box.
	/// @param[out] box_width The available width for the box.
	/// @param[in] cursor The ideal vertical position for the box.
	/// @param[in] dimensions The minimum available space required for the box.
	void PositionBox(Vector2f& box_position, float& box_width, float cursor, const Vector2f& dimensions) const;

	/// Generates and sets the position for a floating box of a given size within our block box. The element's box
	/// is then added into our list of floating boxes.
	/// @param[in] cursor The ideal vertical position for the box.
	/// @param[in] element The element to position.
	/// @return The offset of the bottom outer edge of the element.
	float PositionBox(float cursor, Element* element);

	/// Determines the appropriate vertical position for an object that is choosing to clear floating elements to
	/// the left or right (or both).
	/// @param[in] cursor The ideal vertical position.
	/// @param[in] clear_property The value of the clear property of the clearing object.
	/// @return The appropriate vertical position for the clearing object.
	float ClearBoxes(float cursor, Style::Clear clear_property) const;

	/// Returns the top-left corner of the boxes within the space.
	/// @return The space's offset.
	const Vector2f& GetOffset() const;
	/// Returns the dimensions of the boxes within the space.
	/// @return The space's dimensions.
	Vector2f GetDimensions() const;

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

private:
	enum AnchorEdge
	{
		LEFT = 0,
		RIGHT = 1,
		NUM_ANCHOR_EDGES = 2
	};

	/// Generates the position for an arbitrary box within our space layout, floated against either the left or
	/// right edge.
	/// @param box_position[out] The generated position for the box.
	/// @param cursor[in] The ideal vertical position for the box.
	/// @param dimensions[in] The size of the box to place.
	/// @return The maximum width at the box position.
	float PositionBox(Vector2f& box_position, float cursor, const Vector2f& dimensions, Style::Float float_property = Style::Float::None) const;

	struct SpaceBox
	{
		SpaceBox();
		SpaceBox(const Vector2f& offset, const Vector2f& dimensions);

		Vector2f offset;
		Vector2f dimensions;
	};

	using SpaceBoxList = Vector< SpaceBox >;

	// Our block-box parent.
	LayoutBlockBox* parent;

	// The boxes floating in our space.
	SpaceBoxList boxes[NUM_ANCHOR_EDGES];

	// The offset and dimensions of the boxes added specifically into this space.
	Vector2f offset;
	Vector2f dimensions;
};

} // namespace Rml
#endif
