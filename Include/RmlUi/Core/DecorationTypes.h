#pragma once

#include "NumericValue.h"
#include "Types.h"

namespace Rml {

struct ColorStop {
	ColourbPremultiplied color;
	NumericValue position;
#ifdef RMLUI_MATH_EXPRESSIONS
	CalculationPtr position_calculation;
#endif
};
inline bool operator==(const ColorStop& a, const ColorStop& b)
{
	return a.color == b.color && a.position == b.position
#ifdef RMLUI_MATH_EXPRESSIONS
		&& a.position_calculation == b.position_calculation
#endif
		;
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
#ifdef RMLUI_MATH_EXPRESSIONS
	CalculationPtr offset_x_calculation, offset_y_calculation;
	CalculationPtr blur_radius_calculation, spread_distance_calculation;
#endif
};
inline bool operator==(const BoxShadow& a, const BoxShadow& b)
{
	return a.color == b.color && a.offset_x == b.offset_x && a.offset_y == b.offset_y && a.blur_radius == b.blur_radius &&
		a.spread_distance == b.spread_distance
#ifdef RMLUI_MATH_EXPRESSIONS
		&& a.offset_x_calculation == b.offset_x_calculation && a.offset_y_calculation == b.offset_y_calculation &&
		a.blur_radius_calculation == b.blur_radius_calculation && a.spread_distance_calculation == b.spread_distance_calculation
#endif
		&& a.inset == b.inset;
}
inline bool operator!=(const BoxShadow& a, const BoxShadow& b)
{
	return !(a == b);
}

} // namespace Rml
