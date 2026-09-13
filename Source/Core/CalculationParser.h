#pragma once

#include "Calculation.h"

namespace Rml {

enum class CalculationPercentageHint : uint8_t { None, Length, Angle };

enum class CalculationFinalType : uint8_t {
	Number = 1 << 0,
	Length = 1 << 1,
	Angle = 1 << 2,
	Resolution = 1 << 3,
	Percent = 1 << 4,
	Time = 1 << 5,
};

using CalculationTypeMask = uint8_t;

constexpr CalculationTypeMask operator|(CalculationFinalType lhs, CalculationFinalType rhs)
{
	return CalculationTypeMask(lhs) | CalculationTypeMask(rhs);
}

struct CalculationParseTarget {
	CalculationTypeMask allowed_final_types = 0;
	CalculationPercentageHint percentage_hint = CalculationPercentageHint::None;
};

struct CalculationConstantValue {
	float value = 0;
	Unit unit = Unit::UNKNOWN;
};

CalculationParseTarget MakeCalculationParseTarget(CalculationFinalType type, CalculationPercentageHint hint = CalculationPercentageHint::None);
bool ParseCalculation(const String& expression, const CalculationParseTarget& target, CalculationPtr& result);
bool EvaluateCalculation(const Calculation& calculation, CalculationConstantValue& result);
bool EvaluateCalculationTime(const Calculation& calculation, float& seconds);
CalculationTypeMask GetCalculationFinalType(const Calculation& calculation);
bool SimplifyCalculation(const Calculation& calculation, CalculationPtr& result);

} // namespace Rml
