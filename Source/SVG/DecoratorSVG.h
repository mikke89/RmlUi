#pragma once

#include "../../Include/RmlUi/Core/Decorator.h"

namespace Rml {
namespace SVG {

	struct SVGData;

	class DecoratorSVG : public Decorator {
	public:
		DecoratorSVG(const String& source, const bool crop_to_content);
		virtual ~DecoratorSVG();

		DecoratorDataHandle GenerateElementData(Element* element, BoxArea paint_area) const override;
		void ReleaseElementData(DecoratorDataHandle element_data) const override;

		void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

	private:
		struct Data {
			SharedPtr<SVGData> handle;
			BoxArea paint_area;
		};

		String source_path;
		bool crop_to_content;
	};

	class DecoratorSVGInstancer : public DecoratorInstancer {
	public:
		DecoratorSVGInstancer();
		~DecoratorSVGInstancer();

		SharedPtr<Decorator> InstanceDecorator(const String&, const PropertyDictionary& properties, const DecoratorInstancerInterface&) override;

	private:
		PropertyId source_id;
		PropertyId crop_id;
	};

} // namespace SVG
} // namespace Rml
