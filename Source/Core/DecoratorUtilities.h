#pragma once

#include "../../Include/RmlUi/Core/NumericValue.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

using Vector2Numeric = Vector2<NumericValue>;

// Compute a 2d-position property value into a percentage-length vector.
Vector2Numeric ComputePosition(Array<const Property*, 2> p_position);

} // namespace Rml
