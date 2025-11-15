#include "Inventory.h"
#include <RmlUi/Core/Factory.h>

Inventory::Inventory(const Rml::String& title, const Rml::Vector2f& position, Rml::Context* context)
{
	document = context->LoadDocument("tutorial/drag/data/inventory.rml");
	if (document)
	{
		document->GetElementById("title")->SetInnerRML(title);
		document->SetProperty(Rml::PropertyId::Left, Rml::Property(position.x, Rml::Unit::DP));
		document->SetProperty(Rml::PropertyId::Top, Rml::Property(position.y, Rml::Unit::DP));
		document->Show();
	}
}

Inventory::~Inventory()
{
	if (document)
		document->Close();
}

void Inventory::AddItem(const Rml::String& name)
{
	if (!document)
		return;

	Rml::Element* content = document->GetElementById("content");
	if (!content)
		return;

	// Create the new 'icon' element.
	Rml::Element* icon = Rml::As<Rml::Element*>(content->AppendChild(Rml::Factory::InstanceNode("icon", "icon")));
	icon->SetInnerRML(name);
}
