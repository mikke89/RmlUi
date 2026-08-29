#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "Calculation.h"
#endif

namespace Rml {

Property::Property() : unit(Unit::UNKNOWN), specificity(-1)
{
	definition = nullptr;
	parser_index = -1;
}

String Property::ToString() const
{
#ifdef RMLUI_MATH_EXPRESSIONS
	if (unit == Unit::CALCULATION)
	{
		const CalculationPtr calculation = value.Get<CalculationPtr>();
		return calculation ? calculation->ToString() : String();
	}
#endif
	if (!definition)
		return value.Get<String>() + Rml::ToString(unit);

	String string;
	definition->GetValue(string, *this);
	return string;
}

NumericValue Property::GetNumericValue() const
{
	NumericValue result;
	if (Any(unit & Unit::NUMERIC))
	{
		if (value.GetInto(result.number))
			result.unit = unit;
	}
	return result;
}

} // namespace Rml
