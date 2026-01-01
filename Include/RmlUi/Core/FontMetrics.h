#pragma once

#include "Header.h"

namespace Rml {

struct FontMetrics {
	int size;                  // Specified font size [px].

	float ascent;              // Distance above the baseline to the upper grid coordinate [px, positive above baseline].
	float descent;             // Distance below the baseline to the lower grid coordinate [px, positive below baseline].
	float line_spacing;        // Font-specified distance between two consecutive baselines [px].

	float x_height;            // Height of the lowercase 'x' character [px].

	float underline_position;  // Position of the underline relative to the baseline [px, positive below baseline].
	float underline_thickness; // Width of underline [px].

	bool has_ellipsis;         // True if the ellipsis character (U+2026) is available in this font.
};

} // namespace Rml
