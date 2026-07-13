#pragma once

#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

class Element;

/*
    Memoizes content-based size measurements for the duration of a single layout pass.

    Nested flex containers repeat the measurements of all their descendants, which makes the number of formatting
    calls grow exponentially with the nesting depth. The measured values only depend on the element's subtree and the
    given size constraints, both of which are fixed during a layout pass, so they can safely be reused until the pass
    is completed. The cache registers itself as active during its lifetime and is instantiated at the root of each
    layout pass.
*/
class LayoutMeasureCache final {
public:
	LayoutMeasureCache();
	~LayoutMeasureCache();

	/// Returns the currently active cache, or nullptr if no layout pass is in progress.
	static LayoutMeasureCache* GetActiveCache();

	/// Retrieves the shrink-to-fit width of an element, as previously measured under the same containing block.
	bool FindShrinkToFitWidth(Element* element, Vector2f containing_block, float& shrink_to_fit_width) const;
	void StoreShrinkToFitWidth(Element* element, Vector2f containing_block, float shrink_to_fit_width);

	/// Retrieves the max-content size of a flex container, as previously measured.
	bool FindMaxContentSize(Element* element, Vector2f& max_content_size) const;
	void StoreMaxContentSize(Element* element, Vector2f max_content_size);

	/// Retrieves the content height of an element, as previously formatted under the same content width.
	bool FindFormattedContentHeight(Element* element, float content_width, float& formatted_content_height) const;
	void StoreFormattedContentHeight(Element* element, float content_width, float formatted_content_height);

private:
	struct ShrinkToFitWidthEntry {
		Vector2f containing_block;
		float shrink_to_fit_width;
	};
	struct FormattedContentHeightEntry {
		float content_width;
		float formatted_content_height;
	};

	UnorderedMap<Element*, Vector<ShrinkToFitWidthEntry>> shrink_to_fit_widths;
	UnorderedMap<Element*, Vector2f> max_content_sizes;
	UnorderedMap<Element*, Vector<FormattedContentHeightEntry>> formatted_content_heights;

	LayoutMeasureCache* previous_active_cache;
};

} // namespace Rml
