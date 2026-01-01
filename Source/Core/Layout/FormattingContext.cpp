#include "FormattingContext.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "BlockFormattingContext.h"
#include "FlexFormattingContext.h"
#include "LayoutBox.h"
#include "ReplacedFormattingContext.h"
#include "TableFormattingContext.h"

namespace Rml {

UniquePtr<LayoutBox> FormattingContext::FormatIndependent(ContainerBox* parent_container, Element* element, const Box* override_initial_box,
	FormattingContextType backup_context)
{
	RMLUI_ZoneScopedC(0xAFAFAF);
	using namespace Style;

	if (element->IsReplaced())
		return ReplacedFormattingContext::Format(parent_container, element, override_initial_box);

	FormattingContextType type = backup_context;

	auto& computed = element->GetComputedValues();
	const Display display = computed.display();
	if (display == Display::Flex || display == Display::InlineFlex)
	{
		type = FormattingContextType::Flex;
	}
	else if (display == Display::Table || display == Display::InlineTable)
	{
		type = FormattingContextType::Table;
	}
	else if (display == Display::InlineBlock || display == Display::FlowRoot || display == Display::TableCell || computed.float_() != Float::None ||
		computed.position() == Position::Absolute || computed.position() == Position::Fixed || computed.overflow_x() != Overflow::Visible ||
		computed.overflow_y() != Overflow::Visible || !element->GetParentNode() || element->GetParentNode()->GetDisplay() == Display::Flex)
	{
		type = FormattingContextType::Block;
	}

	switch (type)
	{
	case FormattingContextType::Block: return BlockFormattingContext::Format(parent_container, element, override_initial_box);
	case FormattingContextType::Table: return TableFormattingContext::Format(parent_container, element, override_initial_box);
	case FormattingContextType::Flex: return FlexFormattingContext::Format(parent_container, element, override_initial_box);
	case FormattingContextType::None: break;
	}

	return nullptr;
}

} // namespace Rml
