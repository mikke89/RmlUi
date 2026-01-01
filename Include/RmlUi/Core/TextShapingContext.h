#pragma once

#include "StyleTypes.h"
#include "Types.h"

namespace Rml {

/*
    Data extracted from the properties of an element to help provide context for text shaping and spacing.
*/
struct TextShapingContext {
	const String& language;
	Style::Direction text_direction = Style::Direction::Auto;
	Style::FontKerning font_kerning = Style::FontKerning::Auto;
	float letter_spacing = 0.0f; // Measured in pixels.
};

} // namespace Rml
