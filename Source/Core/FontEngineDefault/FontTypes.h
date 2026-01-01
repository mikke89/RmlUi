#pragma once

#include "../../../Include/RmlUi/Core/FontGlyph.h"
#include "../../../Include/RmlUi/Core/StyleTypes.h"
#include "../../../Include/RmlUi/Core/Types.h"

namespace Rml {

using FontFaceHandleFreetype = uintptr_t;

struct FaceVariation {
	Style::FontWeight weight;
	uint16_t width;
	int named_instance_index;
};

inline bool operator<(const FaceVariation& a, const FaceVariation& b)
{
	if (a.weight == b.weight)
		return a.width < b.width;
	return a.weight < b.weight;
}

} // namespace Rml
