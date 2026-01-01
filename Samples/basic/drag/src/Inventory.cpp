#include "Inventory.h"
#include "DragListener.h"
#include <RmlUi/Core/Factory.h>

Inventory::Inventory(const Rml::String& title, const Rml::Vector2f& position, Rml::Context* context)
{
	document = context->LoadDocument("basic/drag/data/inventory.rml");
	if (document != nullptr)
	{
		using Rml::PropertyId;
		document->GetElementById("title")->SetInnerRML(title);
		document->SetProperty(PropertyId::Left, Rml::Property(position.x, Rml::Unit::DP));
		document->SetProperty(PropertyId::Top, Rml::Property(position.y, Rml::Unit::DP));
		document->Show();
	}

	DragListener::RegisterDraggableContainer(document->GetElementById("content"));
}

Inventory::~Inventory()
{
	if (document != nullptr)
	{
		document->Close();
	}
}

void Inventory::AddItem(const Rml::String& name)
{
	if (document == nullptr)
		return;

	Rml::Element* content = document->GetElementById("content");
	if (content == nullptr)
		return;

	// Create the new 'icon' element.
	Rml::Element* icon = content->AppendChild(Rml::Factory::InstanceElement(content, "icon", "icon", Rml::XMLAttributes()));
	icon->SetInnerRML(name);
}
