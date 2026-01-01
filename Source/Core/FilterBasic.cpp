#include "FilterBasic.h"
#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

namespace Rml {

bool FilterBasic::Initialise(const String& in_name, float in_value)
{
	name = in_name;
	value = in_value;
	return true;
}

CompiledFilter FilterBasic::CompileFilter(Element* element) const
{
	return element->GetRenderManager()->CompileFilter(name, Dictionary{{"value", Variant(value)}});
}

FilterBasicInstancer::FilterBasicInstancer(ValueType value_type, const char* default_value)
{
	switch (value_type)
	{
	case ValueType::NumberPercent: ids.value = RegisterProperty("value", default_value).AddParser("number_percent").GetId(); break;
	case ValueType::Angle: ids.value = RegisterProperty("value", default_value).AddParser("angle").GetId(); break;
	}

	RegisterShorthand("filter", "value", ShorthandType::FallThrough);
}

SharedPtr<Filter> FilterBasicInstancer::InstanceFilter(const String& name, const PropertyDictionary& properties)
{
	const Property* p_value = properties.GetProperty(ids.value);
	if (!p_value)
		return nullptr;

	float value = p_value->Get<float>();
	if (p_value->unit == Unit::PERCENT)
		value *= 0.01f;
	else if (p_value->unit == Unit::DEG)
		value = Rml::Math::DegreesToRadians(value);

	auto filter = MakeShared<FilterBasic>();
	if (filter->Initialise(name, value))
		return filter;

	return nullptr;
}

} // namespace Rml
