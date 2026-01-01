#pragma once

#include "../../../Include/RmlUi/Core/Types.h"
#include "FormattingContext.h"
#include "LayoutBox.h"

namespace Rml {

/*
    A formatting context that handles replaced elements.

    Replaced elements normally take care of their own layouting, so this is only responsible for setting thei box
    dimensions and notifying the element.
*/
class ReplacedFormattingContext final : public FormattingContext {
public:
	static UniquePtr<LayoutBox> Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box);
};

class ReplacedBox : public LayoutBox {
public:
	ReplacedBox(Element* element) : LayoutBox(Type::Replaced), element(element) {}

	void Close();
	Box& GetBox() { return box; }

	const Box* GetIfBox() const override { return &box; }
	String DebugDumpTree(int depth) const override;

private:
	Element* element;
	Box box;
};

} // namespace Rml
