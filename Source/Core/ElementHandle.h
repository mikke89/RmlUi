#pragma once

#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Header.h"

namespace Rml {

/**
    A derivation of an element for use as a mouse drag handle. It responds to drag events, and can be configured to move
    or resize specified target elements.
 */

class RMLUICORE_API ElementHandle : public Element {
public:
	RMLUI_RTTI_DefineWithParent(ElementHandle, Element)

	ElementHandle(const String& tag);
	virtual ~ElementHandle();

	struct MoveData {
		Vector2f original_position_top_left;
		Vector2f original_position_bottom_right;
		Vector2<bool> top_left;
		Vector2<bool> bottom_right;
	};

	struct SizeData {
		Vector2f original_size;
		Vector2f original_position_bottom_right;
		Vector2<bool> width_height;
		Vector2<bool> bottom_right;
	};

protected:
	void OnAttributeChange(const ElementAttributes& changed_attributes) override;
	void ProcessDefaultAction(Event& event) override;

	bool initialised;
	Element* move_target;
	Element* size_target;
	Array<NumericValue, 4> edge_margin = {};

	Vector2f drag_start;
	Vector2f drag_delta_min;
	Vector2f drag_delta_max;

	MoveData move_data;
	SizeData size_data;
};

} // namespace Rml
