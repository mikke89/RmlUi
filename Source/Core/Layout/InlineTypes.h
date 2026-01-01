#pragma once

#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

using LayoutOverflowHandle = int;
using LayoutFragmentHandle = int;

enum class InlineLayoutMode {
	WrapAny,          // Allow wrapping to avoid overflow, even if nothing is placed.
	WrapAfterContent, // Allow wrapping to avoid overflow, but first place at least *some* content on this line.
	Nowrap,           // Place all content on this line, regardless of overflow.
};

enum class FragmentType : byte {
	Invalid,   // Could not be placed.
	InlineBox, // An inline box.
	SizedBox,  // Sized inline-level boxes that are not inline-boxes.
	TextRun,   // Text runs.
};

struct FragmentConstructor {
	FragmentType type = FragmentType::Invalid;
	float layout_width = 0.f;
	LayoutFragmentHandle fragment_handle = {}; // Handle to enable the inline-level box to reference any fragment-specific data.
	LayoutOverflowHandle overflow_handle = {}; // Overflow handle is non-zero when there is another fragment to be layed out.
};

struct PlacedFragment {
	Element* offset_parent;
	LayoutFragmentHandle handle;
	Vector2f position;
	float layout_width;
	bool split_left;
	bool split_right;
};

} // namespace Rml
