#include "DecoratorDefender.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/MeshUtilities.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/Texture.h>
#include <RmlUi/Core/Types.h>

struct DecoratorDefenderElementData {
	Rml::Texture texture;
	Rml::Geometry geometry;
};

DecoratorDefender::~DecoratorDefender() {}

bool DecoratorDefender::Initialise(const Rml::Texture& texture)
{
	image_index = AddTexture(texture);
	if (image_index == -1)
	{
		return false;
	}

	return true;
}

Rml::DecoratorDataHandle DecoratorDefender::GenerateElementData(Rml::Element* element, Rml::BoxArea /*paint_area*/) const
{
	Rml::RenderManager* render_manager = element->GetRenderManager();
	if (!render_manager)
		return Rml::Decorator::INVALID_DECORATORDATAHANDLE;

	Rml::Vector2f position = element->GetAbsoluteOffset(Rml::BoxArea::Padding);
	Rml::Vector2f size = element->GetBox().GetSize(Rml::BoxArea::Padding);
	Rml::Math::SnapToPixelGrid(position, size);

	Rml::ColourbPremultiplied color = element->GetProperty<Rml::Colourb>("image-color").ToPremultiplied();
	Rml::Mesh mesh;
	Rml::MeshUtilities::GenerateQuad(mesh, Rml::Vector2f(0.f), size, color);

	DecoratorDefenderElementData* element_data = new DecoratorDefenderElementData{
		GetTexture(image_index),
		render_manager->MakeGeometry(std::move(mesh)),
	};

	if (!element_data->texture || !element_data->geometry)
		return Rml::Decorator::INVALID_DECORATORDATAHANDLE;

	return reinterpret_cast<Rml::DecoratorDataHandle>(element_data);
}

void DecoratorDefender::ReleaseElementData(Rml::DecoratorDataHandle element_data_handle) const
{
	delete reinterpret_cast<DecoratorDefenderElementData*>(element_data_handle);
}

void DecoratorDefender::RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data_handle) const
{
	Rml::Vector2f position = element->GetAbsoluteOffset(Rml::BoxArea::Padding).Round();
	DecoratorDefenderElementData* element_data = reinterpret_cast<DecoratorDefenderElementData*>(element_data_handle);
	element_data->geometry.Render(position, element_data->texture);
}

DecoratorInstancerDefender::DecoratorInstancerDefender()
{
	id_image_src = RegisterProperty("image-src", "").AddParser("string").GetId();
	RegisterShorthand("decorator", "image-src", Rml::ShorthandType::FallThrough);
}

DecoratorInstancerDefender::~DecoratorInstancerDefender() {}

Rml::SharedPtr<Rml::Decorator> DecoratorInstancerDefender::InstanceDecorator(const Rml::String& /*name*/, const Rml::PropertyDictionary& properties,
	const Rml::DecoratorInstancerInterface& instancer_interface)
{
	const Rml::Property* image_source_property = properties.GetProperty(id_image_src);
	Rml::String image_source = image_source_property->Get<Rml::String>();
	Rml::Texture texture = instancer_interface.GetTexture(image_source);

	auto decorator = Rml::MakeShared<DecoratorDefender>();
	if (decorator->Initialise(texture))
		return decorator;

	return nullptr;
}
