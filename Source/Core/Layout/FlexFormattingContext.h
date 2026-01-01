#pragma once

#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"

namespace Rml {

class LayoutBox;
class ContainerBox;
class FlexContainer;

/*
    Formats a flex container element and its flex items according to flexible box (flexbox) layout rules.
*/
class FlexFormattingContext final : public FormattingContext {
public:
	/// Formats a flex container element and its flex items according to flexbox layout rules.
	static UniquePtr<LayoutBox> Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box);

	/// Computes max-content size for a flex container.
	static Vector2f GetMaxContentSize(Element* element);

private:
	FlexFormattingContext() = default;

	/// Format the flexbox and its children.
	/// @param[out] flex_resulting_content_size The final content size of the flex container.
	/// @param[out] flex_content_overflow_size Overflow size in case flex items or their contents overflow the container.
	/// @param[out] flex_baseline The baseline of the flex container, in terms of the vertical distance from its top-left border corner.
	void Format(Vector2f& flex_resulting_content_size, Vector2f& flex_content_overflow_size, float& flex_baseline) const;

	Vector2f flex_available_content_size;
	Vector2f flex_content_containing_block;
	Vector2f flex_content_offset;
	Vector2f flex_min_size;
	Vector2f flex_max_size;

	Element* element_flex = nullptr;
	FlexContainer* flex_container_box = nullptr;
};

} // namespace Rml
