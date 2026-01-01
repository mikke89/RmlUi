#pragma once

#include "Unit.h"

namespace Rml {

/**
    A numeric value is a number combined with a unit.
 */
struct NumericValue {
	NumericValue() noexcept : number(0.f), unit(Unit::UNKNOWN) {}
	NumericValue(float number, Unit unit) noexcept : number(number), unit(unit) {}
	float number;
	Unit unit;
};
inline bool operator==(const NumericValue& a, const NumericValue& b)
{
	return a.number == b.number && a.unit == b.unit;
}
inline bool operator!=(const NumericValue& a, const NumericValue& b)
{
	return !(a == b);
}

} // namespace Rml
