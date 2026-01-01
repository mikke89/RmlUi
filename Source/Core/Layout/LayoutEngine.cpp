#include "LayoutEngine.h"
#include "../../../Include/RmlUi/Core/Element.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/Profiling.h"
#include "ContainerBox.h"
#include "FormattingContext.h"

namespace Rml {

void LayoutEngine::FormatElement(Element* element, Vector2f containing_block)
{
	RMLUI_ASSERT(element && containing_block.x >= 0 && containing_block.y >= 0);

	RootBox root(containing_block);

	auto layout_box = FormattingContext::FormatIndependent(&root, element, nullptr, FormattingContextType::Block);
	if (!layout_box)
	{
		Log::Message(Log::LT_ERROR, "Error while formatting element: %s", element->GetAddress().c_str());
	}

	{
		RMLUI_ZoneScopedN("ClampScrollOffsetRecursive");
		// The size of the scrollable area might have changed, so clamp the scroll offset to avoid scrolling outside the
		// scrollable area. During layouting, we might be changing the scrollable overflow area of the element several
		// times, such as after enabling scrollbars. For this reason, we don't clamp the scroll offset during layouting,
		// as that could inadvertently clamp it to a temporary size. Now that we know the final layout, including the
		// size of each element's scrollable area, we can finally clamp the scroll offset.
		element->ClampScrollOffsetRecursive();
	}
}

} // namespace Rml
