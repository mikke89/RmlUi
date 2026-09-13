#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#ifdef RMLUI_MATH_EXPRESSIONS
	#include "CalculationResolver.h"
#endif

namespace Rml {

Transform::Transform() {}

Transform::Transform(PrimitiveList primitives) : primitives(std::move(primitives)) {}

Property Transform::MakeProperty(PrimitiveList primitives)
{
	Property p(MakeShared<Transform>(std::move(primitives)), Unit::TRANSFORM);
	p.definition = StyleSheetSpecification::GetProperty(PropertyId::Transform);
	return p;
}

void Transform::ClearPrimitives()
{
	primitives.clear();
#ifdef RMLUI_MATH_EXPRESSIONS
	calculations.clear();
#endif
}

void Transform::AddPrimitive(const TransformPrimitive& p)
{
	primitives.push_back(p);
}

#ifdef RMLUI_MATH_EXPRESSIONS
void Transform::AddCalculation(int primitive_index, int argument_index, CalculationPtr calculation)
{
	RMLUI_ASSERT(calculation);
	calculations.push_back({primitive_index, argument_index, std::move(calculation)});
}

TransformPrimitive Transform::ResolvePrimitive(int primitive_index, Element& element) const
{
	TransformPrimitive result = primitives[primitive_index];
	const Vector2f border_size = element.GetBox().GetSize(BoxArea::Border);

	auto set_argument = [&](int argument_index, float value) {
		using namespace Transforms;
		switch (result.type)
		{
		case TransformPrimitive::MATRIX2D: result.matrix_2d.values[argument_index] = value; break;
		case TransformPrimitive::MATRIX3D: result.matrix_3d.values[argument_index] = value; break;
		case TransformPrimitive::TRANSLATEX: result.translate_x.values[argument_index] = NumericValue(value, Unit::PX); break;
		case TransformPrimitive::TRANSLATEY: result.translate_y.values[argument_index] = NumericValue(value, Unit::PX); break;
		case TransformPrimitive::TRANSLATEZ: result.translate_z.values[argument_index] = NumericValue(value, Unit::PX); break;
		case TransformPrimitive::TRANSLATE2D: result.translate_2d.values[argument_index] = NumericValue(value, Unit::PX); break;
		case TransformPrimitive::TRANSLATE3D: result.translate_3d.values[argument_index] = NumericValue(value, Unit::PX); break;
		case TransformPrimitive::SCALEX: result.scale_x.values[argument_index] = value; break;
		case TransformPrimitive::SCALEY: result.scale_y.values[argument_index] = value; break;
		case TransformPrimitive::SCALEZ: result.scale_z.values[argument_index] = value; break;
		case TransformPrimitive::SCALE2D: result.scale_2d.values[argument_index] = value; break;
		case TransformPrimitive::SCALE3D: result.scale_3d.values[argument_index] = value; break;
		case TransformPrimitive::ROTATEX: result.rotate_x.values[argument_index] = value; break;
		case TransformPrimitive::ROTATEY: result.rotate_y.values[argument_index] = value; break;
		case TransformPrimitive::ROTATEZ: result.rotate_z.values[argument_index] = value; break;
		case TransformPrimitive::ROTATE2D: result.rotate_2d.values[argument_index] = value; break;
		case TransformPrimitive::ROTATE3D: result.rotate_3d.values[argument_index] = value; break;
		case TransformPrimitive::SKEWX: result.skew_x.values[argument_index] = value; break;
		case TransformPrimitive::SKEWY: result.skew_y.values[argument_index] = value; break;
		case TransformPrimitive::SKEW2D: result.skew_2d.values[argument_index] = value; break;
		case TransformPrimitive::PERSPECTIVE: result.perspective.values[argument_index] = NumericValue(value, Unit::PX); break;
		case TransformPrimitive::DECOMPOSEDMATRIX4: break;
		}
	};

	for (const CalculationEntry& entry : calculations)
	{
		if (entry.primitive_index != primitive_index || !entry.calculation)
			continue;

		float percentage_basis = 0.f;
		switch (result.type)
		{
		case TransformPrimitive::TRANSLATEX: percentage_basis = border_size.x; break;
		case TransformPrimitive::TRANSLATEY: percentage_basis = border_size.y; break;
		case TransformPrimitive::TRANSLATE2D: percentage_basis = (entry.argument_index == 0 ? border_size.x : border_size.y); break;
		case TransformPrimitive::TRANSLATE3D:
			percentage_basis = (entry.argument_index == 0 ? border_size.x : (entry.argument_index == 1 ? border_size.y : 0.f));
			break;
		default: break;
		}

		float value = 0.f;
		if (ResolveElementCalculation(*entry.calculation, element, percentage_basis, value))
			set_argument(entry.argument_index, value);
	}

	return result;
}
#endif

int Transform::GetNumPrimitives() const noexcept
{
	return (int)primitives.size();
}

const TransformPrimitive& Transform::GetPrimitive(int i) const noexcept
{
	return primitives[i];
}

} // namespace Rml
