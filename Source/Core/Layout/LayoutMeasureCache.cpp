#include "LayoutMeasureCache.h"
#include <algorithm>

namespace Rml {

static LayoutMeasureCache* active_measure_cache = nullptr;

LayoutMeasureCache::LayoutMeasureCache() : previous_active_cache(active_measure_cache)
{
	active_measure_cache = this;
}

LayoutMeasureCache::~LayoutMeasureCache()
{
	active_measure_cache = previous_active_cache;
}

LayoutMeasureCache* LayoutMeasureCache::GetActiveCache()
{
	return active_measure_cache;
}

bool LayoutMeasureCache::FindShrinkToFitWidth(Element* element, Vector2f containing_block, float& shrink_to_fit_width) const
{
	auto it = shrink_to_fit_widths.find(element);
	if (it == shrink_to_fit_widths.end())
		return false;

	auto it_entry = std::find_if(it->second.begin(), it->second.end(),
		[containing_block](const ShrinkToFitWidthEntry& entry) { return entry.containing_block == containing_block; });
	if (it_entry == it->second.end())
		return false;

	shrink_to_fit_width = it_entry->shrink_to_fit_width;
	return true;
}

void LayoutMeasureCache::StoreShrinkToFitWidth(Element* element, Vector2f containing_block, float shrink_to_fit_width)
{
	shrink_to_fit_widths[element].push_back(ShrinkToFitWidthEntry{containing_block, shrink_to_fit_width});
}

bool LayoutMeasureCache::FindMaxContentSize(Element* element, Vector2f& max_content_size) const
{
	auto it = max_content_sizes.find(element);
	if (it == max_content_sizes.end())
		return false;

	max_content_size = it->second;
	return true;
}

void LayoutMeasureCache::StoreMaxContentSize(Element* element, Vector2f max_content_size)
{
	max_content_sizes[element] = max_content_size;
}

bool LayoutMeasureCache::FindFormattedContentHeight(Element* element, float content_width, float& formatted_content_height) const
{
	auto it = formatted_content_heights.find(element);
	if (it == formatted_content_heights.end())
		return false;

	auto it_entry = std::find_if(it->second.begin(), it->second.end(),
		[content_width](const FormattedContentHeightEntry& entry) { return entry.content_width == content_width; });
	if (it_entry == it->second.end())
		return false;

	formatted_content_height = it_entry->formatted_content_height;
	return true;
}

void LayoutMeasureCache::StoreFormattedContentHeight(Element* element, float content_width, float formatted_content_height)
{
	formatted_content_heights[element].push_back(FormattedContentHeightEntry{content_width, formatted_content_height});
}

} // namespace Rml
