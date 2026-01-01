#include "DecoratorSVG.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "SVGCache.h"

namespace Rml {
namespace SVG {

	DecoratorSVG::DecoratorSVG(const String& source, const bool crop_to_content) : source_path(source), crop_to_content(crop_to_content)
	{
		RMLUI_ASSERT(!source_path.empty());
	}

	DecoratorSVG::~DecoratorSVG() {}

	DecoratorDataHandle DecoratorSVG::GenerateElementData(Element* element, BoxArea paint_area) const
	{
		SharedPtr<SVGData> handle = SVGCache::GetHandle(source_path, source_path, SVGCache::File, element, crop_to_content, paint_area);
		if (!handle)
			return {};

		Data* data = new Data{
			std::move(handle),
			paint_area,
		};

		return reinterpret_cast<DecoratorDataHandle>(data);
	}

	void DecoratorSVG::ReleaseElementData(DecoratorDataHandle element_data) const
	{
		Data* data = reinterpret_cast<Data*>(element_data);
		delete data;
	}

	void DecoratorSVG::RenderElement(Element* element, DecoratorDataHandle element_data) const
	{
		Data* data = reinterpret_cast<Data*>(element_data);
		RMLUI_ASSERT(data && data->handle);
		data->handle->geometry.Render(element->GetAbsoluteOffset(data->paint_area), data->handle->texture);
	}

	DecoratorSVGInstancer::DecoratorSVGInstancer()
	{
		source_id = RegisterProperty("source", "").AddParser("string").GetId();
		crop_id = RegisterProperty("crop", "crop-none").AddParser("keyword", "crop-none, crop-to-content").GetId();
		RegisterShorthand("decorator", "source, crop", ShorthandType::FallThrough);
	}

	DecoratorSVGInstancer::~DecoratorSVGInstancer() {}

	SharedPtr<Decorator> DecoratorSVGInstancer::InstanceDecorator(const String&, const PropertyDictionary& properties,
		const DecoratorInstancerInterface&)
	{
		String source = properties.GetProperty(source_id)->Get<String>();
		if (source.empty())
			return nullptr;

		const bool crop_to_content = properties.GetProperty(crop_id)->Get<int>() != 0;

		return MakeShared<DecoratorSVG>(source, crop_to_content);
	}

} // namespace SVG
} // namespace Rml
