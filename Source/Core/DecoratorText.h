#pragma once

#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/ID.h"
#include "DecoratorUtilities.h"

namespace Rml {

class DecoratorText : public Decorator {
public:
	DecoratorText();
	virtual ~DecoratorText();

	void Initialise(String text, bool inherit_color, Colourb color, Vector2Numeric align);

	DecoratorDataHandle GenerateElementData(Element* element, BoxArea paint_area) const override;
	void ReleaseElementData(DecoratorDataHandle element_data) const override;

	void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

private:
	struct TexturedGeometry {
		Geometry geometry;
		Texture texture;
	};
	struct ElementData {
		BoxArea paint_area;
		Vector<TexturedGeometry> textured_geometry;
		int font_handle_version;
	};

	bool GenerateGeometry(Element* element, ElementData& element_data) const;

	String text;
	bool inherit_color = false;
	Colourb color;
	Vector2Numeric align;
};

class DecoratorTextInstancer : public DecoratorInstancer {
public:
	DecoratorTextInstancer();
	~DecoratorTextInstancer();

	SharedPtr<Decorator> InstanceDecorator(const String& name, const PropertyDictionary& properties,
		const DecoratorInstancerInterface& instancer_interface) override;

private:
	struct PropertyIds {
		PropertyId text, color, align_x, align_y;
	};
	PropertyIds ids;
};

} // namespace Rml
