#include "Inventory.h"
#include <RmlUi/Core/Factory.h>

// Constructs a new inventory and opens its window.
Inventory::Inventory(const Rml::Core::String& title, const Rml::Core::Vector2f& position, Rml::Core::Context* context)
{
	document = context->LoadDocument("tutorial/tutorial_drag/data/inventory.rml");
	if (document != NULL)
	{
		document->GetElementById("title")->SetInnerRML(title);
		document->SetProperty("left", Rml::Core::Property(position.x, Rml::Core::Property::PX));
		document->SetProperty("top", Rml::Core::Property(position.y, Rml::Core::Property::PX));
		document->Show();
	}
}

// Destroys the inventory and closes its window.
Inventory::~Inventory()
{
	if (document != NULL)
	{
		document->RemoveReference();
		document->Close();
	}
}

// Adds a brand-new item into this inventory.
void Inventory::AddItem(const Rml::Core::String& name)
{
	if (document == NULL)
		return;

	Rml::Core::Element* content = document->GetElementById("content");
	if (content == NULL)
		return;

	// Create the new 'icon' element.
	Rml::Core::Element* icon = Rml::Core::Factory::InstanceElement(content, "icon", "icon", Rml::Core::XMLAttributes());
	icon->SetInnerRML(name);
	content->AppendChild(icon);

	// Release the initial reference on the element now that the document has it.
	icon->RemoveReference();
}
