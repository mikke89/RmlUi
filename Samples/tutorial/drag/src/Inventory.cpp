#include "Inventory.h"
#include <RmlUi/Core/Factory.h>

// Constructs a new inventory and opens its window.
Inventory::Inventory(const Rml::Core::String& title, const Rml::Core::Vector2f& position, Rml::Core::Context* context)
{
	document = context->LoadDocument("tutorial/drag/data/inventory.rml");
	if (document)
	{
		document->GetElementById("title")->SetInnerRML(title);
		document->SetProperty(Rml::Core::PropertyId::Left, Rml::Core::Property(position.x, Rml::Core::Property::PX));
		document->SetProperty(Rml::Core::PropertyId::Top, Rml::Core::Property(position.y, Rml::Core::Property::PX));
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
void Inventory::AddItem(const Rml::Core::String& name)
{
	if (!document)
		return;

	Rml::Core::Element* content = document->GetElementById("content");
	if (!content)
		return;

	// Create the new 'icon' element.
	Rml::Core::ElementPtr icon = Rml::Core::Factory::InstanceElement(content, "icon", "icon", Rml::Core::XMLAttributes());
	icon->SetInnerRML(name);
	content->AppendChild(std::move(icon));
}
