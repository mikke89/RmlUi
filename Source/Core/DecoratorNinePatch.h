#pragma once

#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/ID.h"
#include "../../Include/RmlUi/Core/Spritesheet.h"

namespace Rml {

class DecoratorNinePatch : public Decorator {
public:
	DecoratorNinePatch();
	virtual ~DecoratorNinePatch();

	bool Initialise(const Rectanglef& rect_outer, const Rectanglef& rect_inner, const Array<NumericValue, 4>* _edges, Texture texture,
		float display_scale);

	DecoratorDataHandle GenerateElementData(Element* element, BoxArea paint_area) const override;
	void ReleaseElementData(DecoratorDataHandle element_data) const override;

	void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

private:
	Rectanglef rect_outer, rect_inner;
	float display_scale = 1;
	UniquePtr<Array<NumericValue, 4>> edges;
};

class DecoratorNinePatchInstancer : public DecoratorInstancer {
public:
	DecoratorNinePatchInstancer();
	~DecoratorNinePatchInstancer();

	SharedPtr<Decorator> InstanceDecorator(const String& name, const PropertyDictionary& properties,
		const DecoratorInstancerInterface& instancer_interface) override;

private:
	PropertyId sprite_outer_id, sprite_inner_id;
	PropertyId edge_ids[4];
};

} // namespace Rml
