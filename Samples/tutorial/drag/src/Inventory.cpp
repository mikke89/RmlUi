#include "Inventory.h"
#include <RmlUi/Core/Factory.h>

// Constructs a new inventory and opens its window.
Inventory::Inventory(const Rml::String& title, const Rml::Vector2f& position, Rml::Context* context)
{
	document = context->LoadDocument("tutorial/drag/data/inventory.rml");
	if (document)
	{
		document->GetElementById("title")->SetInnerRML(title);
		document->SetProperty(Rml::PropertyId::Left, Rml::Property(position.x, Rml::Property::DP));
		document->SetProperty(Rml::PropertyId::Top, Rml::Property(position.y, Rml::Property::DP));
		document->Show();
	}
}

// Destroys the inventory and closes its window.
Inventory::~Inventory()
{
	if (document)
		document->Close();
}

// Adds a brand-new item into this inventory.
void Inventory::AddItem(const Rml::String& name)
{
	if (!document)
		return;

	Rml::Element* content = document->GetElementById("content");
	if (!content)
		return;

	// Create the new 'icon' element.
	Rml::ElementPtr icon = Rml::Factory::InstanceElement(content, "icon", "icon", Rml::XMLAttributes());
	icon->SetInnerRML(name);
	content->AppendChild(std::move(icon));
}
