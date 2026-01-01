#pragma once

#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"

namespace Rml {

class Box;
class BlockContainer;
class ContainerBox;
class LayoutBox;

/*
    Places boxes according to normal flow, while handling floated boxes.

    A block formatting context (BFC) starts with a root BlockContainer, where child block-level boxes are placed
    vertically. Descendant elements take part in the same BFC, unless the element has properties causing it to establish
    an independent formatting context.

    Floated boxes are taken out-of-flow and placed inside the current BFC. Floats are only affected by, and affect only,
    other boxes within the same BFC.
*/
class BlockFormattingContext final : public FormattingContext {
public:
	static UniquePtr<LayoutBox> Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box);

private:
	// Format the element as a block box, including its children.
	// @return False if the box caused an automatic vertical scrollbar to appear in the block formatting context root, forcing it to be reformatted.
	static bool FormatBlockBox(BlockContainer* parent_container, Element* element);

	// Format the element as an inline box, including its children.
	// @return False if the box caused an automatic vertical scrollbar to appear in the block formatting context root, forcing it to be reformatted.
	static bool FormatInlineBox(BlockContainer* parent_container, Element* element);

	// Determine how to format a child element of a block container, and format it accordingly, possibly including any children.
	// @return False if the box caused an automatic vertical scrollbar to appear in the block formatting context root, forcing it to be reformatted.
	static bool FormatBlockContainerChild(BlockContainer* parent_container, Element* element);
};

} // namespace Rml
