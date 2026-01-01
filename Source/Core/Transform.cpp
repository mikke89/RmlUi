#include "../../Include/RmlUi/Core/Transform.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"

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
}

void Transform::AddPrimitive(const TransformPrimitive& p)
{
	primitives.push_back(p);
}

int Transform::GetNumPrimitives() const noexcept
{
	return (int)primitives.size();
}

const TransformPrimitive& Transform::GetPrimitive(int i) const noexcept
{
	return primitives[i];
}

} // namespace Rml
