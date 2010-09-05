/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "Inventory.h"
#include <Rocket/Core/Factory.h>
#include "DragListener.h"

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

	DragListener::RegisterDraggableContainer(document->GetElementById("content"));
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
