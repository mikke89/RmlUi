#include "DecoratorUtilities.h"
#include "../../Include/RmlUi/Core/Property.h"

namespace Rml {

Vector2Numeric ComputePosition(Array<const Property*, 2> p_position)
{
	Vector2Numeric position;
	for (int dimension = 0; dimension < 2; dimension++)
	{
		NumericValue& value = position[dimension];
		const Property& property = *p_position[dimension];
		if (property.unit == Unit::KEYWORD)
		{
			enum { TOP_LEFT, CENTER, BOTTOM_RIGHT };
			switch (property.Get<int>())
			{
			case TOP_LEFT: value = NumericValue(0.f, Unit::PERCENT); break;
			case CENTER: value = NumericValue(50.f, Unit::PERCENT); break;
			case BOTTOM_RIGHT: value = NumericValue(100.f, Unit::PERCENT); break;
			}
		}
		else
		{
			value = property.GetNumericValue();
		}
	}
	return position;
}

} // namespace Rml
