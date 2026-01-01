#pragma once

#include "../../Include/RmlUi/Core/Filter.h"
#include "../../Include/RmlUi/Core/ID.h"
#include "../../Include/RmlUi/Core/NumericValue.h"

namespace Rml {

class FilterDropShadow : public Filter {
public:
	bool Initialise(Colourb color, NumericValue offset_x, NumericValue offset_y, NumericValue sigma);

	CompiledFilter CompileFilter(Element* element) const override;

	void ExtendInkOverflow(Element* element, Rectanglef& scissor_region) const override;

private:
	Colourb color;
	NumericValue value_offset_x, value_offset_y, value_sigma;
};

class FilterDropShadowInstancer : public FilterInstancer {
public:
	FilterDropShadowInstancer();

	SharedPtr<Filter> InstanceFilter(const String& name, const PropertyDictionary& properties) override;

private:
	struct PropertyIds {
		PropertyId color, offset_x, offset_y, sigma;
	};
	PropertyIds ids;
};

} // namespace Rml
