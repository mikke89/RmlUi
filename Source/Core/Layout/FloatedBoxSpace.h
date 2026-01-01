#pragma once

#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;
class BlockContainer;

enum class FloatedBoxEdge {
	Margin,   // The box's margin area, used for layout sizing.
	Overflow, // Includes the box's border box plus any visible overflow, used to determine overflow and scrolling area.
};

/**
    Each block box has a space object for managing the space occupied by its floating elements, and those of its
    ancestors as relevant.
 */

class FloatedBoxSpace {
public:
	FloatedBoxSpace();
	~FloatedBoxSpace();

	/// Generates the position for a box of a given size within our block box.
	/// @param[out] box_width The available width for the box.
	/// @param[in] cursor The ideal vertical position for the box.
	/// @param[in] dimensions The minimum available space required for the box.
	/// @param[in] nowrap Restrict from wrapping down, returned vertical position always placed at ideal cursor.
	/// @return The generated position for the box.
	Vector2f NextBoxPosition(const BlockContainer* parent, float& box_width, float cursor, Vector2f dimensions, bool nowrap) const;

	/// Determines the position of a floated element within our block box.
	/// @param[out] box_width The available width for the box.
	/// @param[in] cursor The ideal vertical position for the box.
	/// @param[in] dimensions The floated element's margin size.
	/// @param[in] float_property The element's computed float property.
	/// @param[in] clear_property The element's computed clear property.
	/// @return The next placement position for the float at its top-left margin position.
	Vector2f NextFloatPosition(const BlockContainer* parent, float& box_width, float cursor, Vector2f dimensions, Style::Float float_property,
		Style::Clear clear_property) const;

	/// Add a new entry into our list of floated boxes.
	void PlaceFloat(Style::Float float_property, Vector2f margin_position, Vector2f margin_size, Vector2f overflow_position, Vector2f overflow_size);

	/// Determines the appropriate vertical position for an object that is choosing to clear floating elements to
	/// the left or right (or both).
	/// @param[in] cursor The ideal vertical position.
	/// @param[in] clear_property The value of the clear property of the clearing object.
	/// @return The appropriate vertical position for the clearing object.
	float DetermineClearPosition(float cursor, Style::Clear clear_property) const;

	/// Returns the size of the rectangle encompassing all boxes within the space, relative to the block formatting context space.
	/// @param[in] edges Which edge of the boxes to encompass.
	Vector2f GetDimensions(FloatedBoxEdge edge) const;

	/// Get the width of the floated boxes for calculating the shrink-to-fit width.
	float GetShrinkToFitWidth(float edge_left, float edge_right) const;

	/// Clear all floating boxes placed in this space.
	void Reset()
	{
		for (auto& box_list : boxes)
			box_list.clear();
		extent_bottom_right_overflow = {};
		extent_bottom_right_overflow = {};
		extent_bottom_right_margin = {};
	}

	void* operator new(size_t size);
	void operator delete(void* chunk, size_t size);

private:
	enum AnchorEdge { LEFT = 0, RIGHT = 1, NUM_ANCHOR_EDGES = 2 };

	// Generates the position for an arbitrary box within our space layout, floated against either the left or right edge.
	Vector2f NextBoxPosition(const BlockContainer* parent, float& maximum_box_width, float cursor, Vector2f dimensions, bool nowrap,
		Style::Float float_property) const;

	struct FloatedBox {
		Vector2f offset;
		Vector2f dimensions;
	};

	using FloatedBoxList = Vector<FloatedBox>;

	// The boxes floating in our space.
	FloatedBoxList boxes[NUM_ANCHOR_EDGES];

	// The rectangle encompassing all boxes added specifically into this space, relative to our block formatting context space.
	Vector2f extent_top_left_overflow;
	Vector2f extent_bottom_right_overflow;
	Vector2f extent_bottom_right_margin;
};

} // namespace Rml
