#pragma once

#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "Calculation.h"

namespace Rml {

class Element;

struct CalculationResolverContext {
	float font_size = 0.f;
	float parent_font_size = 0.f;
	float document_font_size = 0.f;
	float line_height = 0.f;
	Vector2f viewport_dimensions = {};
	float dp_ratio = 1.f;
	RelativeTarget relative_target = RelativeTarget::None;
};

struct ResolvedCalculation {
	Calculation::ResidualForm form = Calculation::ResidualForm::Constant;
	bool is_constant = false;
	float value = 0.f;
	Unit unit = Unit::UNKNOWN;
	CalculationPtr residual;
};

bool ResolveCalculation(const Calculation& calculation, const CalculationResolverContext& context, ResolvedCalculation& result);
bool ResolveUsedCalculation(const Calculation& calculation, float percentage_basis, float& result);
bool ResolveElementCalculation(const Calculation& calculation, Element& element, float percentage_basis, float& result,
	RelativeTarget relative_target = RelativeTarget::None);

} // namespace Rml
