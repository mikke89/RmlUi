#pragma once

#include "NumericValue.h"
#include "Types.h"

namespace Rml {

struct ColorStop {
	ColourbPremultiplied color;
	NumericValue position;
};
inline bool operator==(const ColorStop& a, const ColorStop& b)
{
	return a.color == b.color && a.position == b.position;
}
inline bool operator!=(const ColorStop& a, const ColorStop& b)
{
	return !(a == b);
}

struct BoxShadow {
	ColourbPremultiplied color;
	NumericValue offset_x, offset_y;
	NumericValue blur_radius;
	NumericValue spread_distance;
	bool inset = false;
};
inline bool operator==(const BoxShadow& a, const BoxShadow& b)
{
	return a.color == b.color && a.offset_x == b.offset_x && a.offset_y == b.offset_y && a.blur_radius == b.blur_radius &&
		a.spread_distance == b.spread_distance && a.inset == b.inset;
}
inline bool operator!=(const BoxShadow& a, const BoxShadow& b)
{
	return !(a == b);
}

} // namespace Rml
