#include "ReplacedFormattingContext.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "BlockFormattingContext.h"
#include "ContainerBox.h"
#include "LayoutDetails.h"

namespace Rml {

UniquePtr<LayoutBox> ReplacedFormattingContext::Format(ContainerBox* parent_container, Element* element, const Box* override_initial_box)
{
	RMLUI_ASSERT(element->IsReplaced());

	// Replaced elements provide their own rendering, we just set their box here and notify them that the element has been sized.
	auto replaced_box = MakeUnique<ReplacedBox>(element);
	Box& box = replaced_box->GetBox();
	if (override_initial_box)
		box = *override_initial_box;
	else
	{
		const Vector2f containing_block = LayoutDetails::GetContainingBlock(parent_container, element->GetPosition()).size;
		LayoutDetails::BuildBox(box, containing_block, element);
	}

	// Submit the box and notify the element.
	replaced_box->Close();

	// Usually, replaced elements add children to the hidden DOM. If we happen to have any normal DOM children, e.g.
	// added by the user, we format them using normal block formatting rules. Since replaced elements provide their
	// own rendering, this could cause conflicting or strange layout results, and is done at the user's own risk.
	if (element->HasChildNodes())
	{
		RootBox root(box);
		BlockFormattingContext::Format(&root, element, &box);
	}

	return replaced_box;
}

void ReplacedBox::Close()
{
	element->SetBox(box);
	element->OnLayout();
}

String ReplacedBox::DebugDumpTree(int depth) const
{
	return String(depth * 2, ' ') + "ReplacedBox";
}

} // namespace Rml
