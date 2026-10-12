#pragma once

namespace Rml {

enum class ScrollBehavior {
	Auto,    // Scroll using the context's configured setting.
	Smooth,  // Scroll using a smooth animation.
	Instant, // Scroll instantly.
};

enum class ScrollAlignment {
	Start,    // Align to the top or left edge of the scroll container.
	Center,   // Align to the center of the scroll container.
	End,      // Align to the bottom or right edge of the scroll container.
	Nearest,  // Align with minimal scroll change.
	Adaptive, // Do not scroll if already in view, otherwise align to the center of the scroll container.
};

enum class ScrollParentage {
	All,     // Scroll all ancestor scroll containers as needed.
	Closest, // Scroll only the closest scroll container.
};

/**
    Defines behavior of Element::ScrollIntoView.
 */
struct ScrollIntoViewOptions {
	ScrollIntoViewOptions(ScrollAlignment vertical = ScrollAlignment::Start, ScrollAlignment horizontal = ScrollAlignment::Nearest,
		ScrollBehavior behavior = ScrollBehavior::Instant, ScrollParentage parentage = ScrollParentage::All) :
		vertical(vertical), horizontal(horizontal), behavior(behavior), parentage(parentage)
	{}
	ScrollAlignment vertical;
	ScrollAlignment horizontal;
	ScrollBehavior behavior;
	ScrollParentage parentage;
};

} // namespace Rml
