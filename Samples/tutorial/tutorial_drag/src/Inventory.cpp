#include "Inventory.h"
#include <Rocket/Core/Factory.h>

// Constructs a new inventory and opens its window.
Inventory::Inventory(const Rocket::Core::String& title, const Rocket::Core::Vector2f& position, Rocket::Core::Context* context)
{
	document = context->LoadDocument("data/inventory.rml");
	if (document != NULL)
	{
		document->GetElementById("title")->SetInnerRML(title);
		document->SetProperty("left", Rocket::Core::Property(position.x, Rocket::Core::Property::PX));
		document->SetProperty("top", Rocket::Core::Property(position.y, Rocket::Core::Property::PX));
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
void Inventory::AddItem(const Rocket::Core::String& name)
{
	if (document == NULL)
		return;

	Rocket::Core::Element* content = document->GetElementById("content");
	if (content == NULL)
		return;

	// Create the new 'icon' element.
	Rocket::Core::Element* icon = Rocket::Core::Factory::InstanceElement(content, "icon", "icon", Rocket::Core::XMLAttributes());
	icon->SetInnerRML(name);
	content->AppendChild(icon);

	// Release the initial reference on the element now that the document has it.
	icon->RemoveReference();
}
