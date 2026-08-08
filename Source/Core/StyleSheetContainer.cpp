#include "../../Include/RmlUi/Core/StyleSheetContainer.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/Utilities.h"
#include "ComputeProperty.h"
#include "StyleSheetParser.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "CalculationResolver.h"
#endif

namespace Rml {

StyleSheetContainer::StyleSheetContainer() {}

StyleSheetContainer::~StyleSheetContainer() {}

bool StyleSheetContainer::LoadStyleSheetContainer(Stream* stream, int begin_line_number)
{
	StyleSheetParser parser;
	bool result = parser.Parse(media_blocks, stream, begin_line_number);
	return result;
}

bool StyleSheetContainer::UpdateCompiledStyleSheet(const Context* context)
{
	RMLUI_ZoneScoped;

	const float dp_ratio = context->GetDensityIndependentPixelRatio();
	const Vector2i vp_dimensions_i(context->GetDimensions());
	const Vector2f vp_dimensions(vp_dimensions_i);

	Vector<int> new_active_media_block_indices;

	const float font_size = DefaultComputedValues().font_size();

#ifdef RMLUI_MATH_EXPRESSIONS
	auto resolve_media_value = [&](const Property& property, float& result) {
		if (property.unit != Unit::CALCULATION)
			return false;
		const CalculationPtr calculation = property.value.Get<CalculationPtr>();
		if (!calculation)
			return false;
		CalculationResolverContext resolver_context;
		resolver_context.font_size = font_size;
		resolver_context.parent_font_size = font_size;
		resolver_context.document_font_size = font_size;
		resolver_context.viewport_dimensions = vp_dimensions;
		resolver_context.dp_ratio = dp_ratio;
		ResolvedCalculation resolved;
		if (!ResolveCalculation(*calculation, resolver_context, resolved) || !resolved.is_constant)
			return false;
		result = resolved.value;
		return resolved.unit == Unit::PX || resolved.unit == Unit::X || resolved.unit == Unit::NUMBER;
	};
#endif

	for (int media_block_index = 0; media_block_index < (int)media_blocks.size(); media_block_index++)
	{
		const MediaBlock& media_block = media_blocks[media_block_index];
		bool all_match = true;
		bool expected_match_value = media_block.modifier == MediaQueryModifier::Not ? false : true;

		for (const auto& property : media_block.properties.GetProperties())
		{
			const MediaQueryId id = static_cast<MediaQueryId>(property.first);
			Vector2i ratio;
			float media_value = 0.f;
#ifdef RMLUI_MATH_EXPRESSIONS
			const bool is_calculated_media_value = property.second.unit == Unit::CALCULATION;
			const bool has_calculated_media_value = is_calculated_media_value && resolve_media_value(property.second, media_value);
			if (is_calculated_media_value && !has_calculated_media_value)
			{
				all_match = false;
				break;
			}
#else
			const bool has_calculated_media_value = false;
#endif
			auto values_equal = [](float lhs, float rhs, bool approximate) { return approximate ? Math::Absolute(lhs - rhs) < 0.0001f : lhs == rhs; };
			auto value_less = [](float lhs, float rhs, bool approximate) { return approximate ? lhs < rhs - 0.0001f : lhs < rhs; };
			auto value_greater = [](float lhs, float rhs, bool approximate) { return approximate ? lhs > rhs + 0.0001f : lhs > rhs; };

			switch (id)
			{
			case MediaQueryId::Width:
				if (!values_equal(float(vp_dimensions.x),
						has_calculated_media_value ? media_value
												   : ComputeLength(property.second.GetNumericValue(), font_size, font_size, dp_ratio, vp_dimensions),
						has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::MinWidth:
				if (value_less(float(vp_dimensions.x),
						has_calculated_media_value ? media_value
												   : ComputeLength(property.second.GetNumericValue(), font_size, font_size, dp_ratio, vp_dimensions),
						has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::MaxWidth:
				if (value_greater(float(vp_dimensions.x),
						has_calculated_media_value ? media_value
												   : ComputeLength(property.second.GetNumericValue(), font_size, font_size, dp_ratio, vp_dimensions),
						has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::Height:
				if (!values_equal(float(vp_dimensions.y),
						has_calculated_media_value ? media_value
												   : ComputeLength(property.second.GetNumericValue(), font_size, font_size, dp_ratio, vp_dimensions),
						has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::MinHeight:
				if (value_less(float(vp_dimensions.y),
						has_calculated_media_value ? media_value
												   : ComputeLength(property.second.GetNumericValue(), font_size, font_size, dp_ratio, vp_dimensions),
						has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::MaxHeight:
				if (value_greater(float(vp_dimensions.y),
						has_calculated_media_value ? media_value
												   : ComputeLength(property.second.GetNumericValue(), font_size, font_size, dp_ratio, vp_dimensions),
						has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::AspectRatio:
				ratio = Vector2i(property.second.Get<Vector2f>());
				if (vp_dimensions_i.x * ratio.y != vp_dimensions_i.y * ratio.x)
					all_match = false;
				break;
			case MediaQueryId::MinAspectRatio:
				ratio = Vector2i(property.second.Get<Vector2f>());
				if (vp_dimensions_i.x * ratio.y < vp_dimensions_i.y * ratio.x)
					all_match = false;
				break;
			case MediaQueryId::MaxAspectRatio:
				ratio = Vector2i(property.second.Get<Vector2f>());
				if (vp_dimensions_i.x * ratio.y > vp_dimensions_i.y * ratio.x)
					all_match = false;
				break;
			case MediaQueryId::Resolution:
				if (!values_equal(dp_ratio, has_calculated_media_value ? media_value : property.second.Get<float>(), has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::MinResolution:
				if (value_less(dp_ratio, has_calculated_media_value ? media_value : property.second.Get<float>(), has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::MaxResolution:
				if (value_greater(dp_ratio, has_calculated_media_value ? media_value : property.second.Get<float>(), has_calculated_media_value))
					all_match = false;
				break;
			case MediaQueryId::Orientation:
				// Landscape (x > y) = 0
				// Portrait (x <= y) = 1
				if ((vp_dimensions.x <= vp_dimensions.y) != property.second.Get<bool>())
					all_match = false;
				break;
			case MediaQueryId::Theme:
				if (!context->IsThemeActive(property.second.Get<String>()))
					all_match = false;
				break;
				// Invalid properties
			case MediaQueryId::Invalid:
			case MediaQueryId::NumDefinedIds: break;
			}

			if (all_match != expected_match_value)
				break;
		}

		if (all_match == expected_match_value)
			new_active_media_block_indices.push_back(media_block_index);
	}

	const bool style_sheet_changed = (new_active_media_block_indices != active_media_block_indices || !compiled_style_sheet);

	if (style_sheet_changed)
	{
		StyleSheet* first_sheet = nullptr;
		UniquePtr<StyleSheet> new_sheet;

		for (int index : new_active_media_block_indices)
		{
			MediaBlock& media_block = media_blocks[index];
			if (!first_sheet)
				first_sheet = media_block.stylesheet.get();
			else if (!new_sheet)
				new_sheet = first_sheet->CombineStyleSheet(*media_block.stylesheet);
			else
				new_sheet->MergeStyleSheet(*media_block.stylesheet);
		}

		if (!first_sheet)
		{
			new_sheet.reset(new StyleSheet);
			first_sheet = new_sheet.get();
		}

		compiled_style_sheet = (new_sheet ? new_sheet.get() : first_sheet);
		combined_compiled_style_sheet = std::move(new_sheet);

		compiled_style_sheet->BuildNodeIndex();
	}

	active_media_block_indices = std::move(new_active_media_block_indices);

	return style_sheet_changed;
}

StyleSheet* StyleSheetContainer::GetCompiledStyleSheet()
{
	return compiled_style_sheet;
}

SharedPtr<StyleSheetContainer> StyleSheetContainer::CombineStyleSheetContainer(const StyleSheetContainer& container) const
{
	RMLUI_ZoneScoped;

	SharedPtr<StyleSheetContainer> new_sheet = MakeShared<StyleSheetContainer>();

	for (const MediaBlock& media_block : media_blocks)
	{
		new_sheet->media_blocks.emplace_back(media_block.properties, media_block.stylesheet, media_block.modifier);
	}

	new_sheet->MergeStyleSheetContainer(container);

	return new_sheet;
}

void StyleSheetContainer::MergeStyleSheetContainer(const StyleSheetContainer& other)
{
	RMLUI_ZoneScoped;

	// Style sheet container must not be merged after it's been compiled. This will invalidate references to the compiled style sheet.
	RMLUI_ASSERT(!compiled_style_sheet);

	auto it_other_begin = other.media_blocks.begin();

#if 0
	// If the last block here has the same media requirements as the first block in other, we can safely merge them
	// while retaining correct specificity of all properties. This is essentially an optimization to avoid more
	// style sheet merging later on.
	if (!media_blocks.empty() && !other.media_blocks.empty())
	{
		MediaBlock& block_local = media_blocks.back();
		const MediaBlock& block_other = other.media_blocks.front();
		if (block_local.properties.GetProperties() == block_other.properties.GetProperties())
		{
			// Now we can safely merge the two style sheets.
			block_local.stylesheet = block_local.stylesheet->CombineStyleSheet(*block_other.stylesheet);

			// And we need to skip the first media block in the 'other' style sheet, since we merged it just now.
			++it_other_begin;
		}
	}
#endif

	// Add all the other blocks into ours.
	for (auto it = it_other_begin; it != other.media_blocks.end(); ++it)
	{
		const MediaBlock& block_other = *it;
		media_blocks.emplace_back(block_other.properties, block_other.stylesheet, block_other.modifier);
	}
}

} // namespace Rml
