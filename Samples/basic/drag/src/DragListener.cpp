#include "DragListener.h"
#include <RmlUi/Core/Element.h>

static DragListener drag_listener;

void DragListener::RegisterDraggableContainer(Rml::Element* element)
{
	element->AddEventListener(Rml::EventId::Dragdrop, &drag_listener);
}

void DragListener::ProcessEvent(Rml::Event& event)
{
	if (event == Rml::EventId::Dragdrop)
	{
		Rml::Element* dest_container = event.GetCurrentElement();
		Rml::Element* dest_element = event.GetTargetElement();
		Rml::Element* drag_element = static_cast<Rml::Element*>(event.GetParameter<void*>("drag_element", nullptr));

		if (dest_container == dest_element)
		{
			// The dragged element was dragged directly onto a container.
			dest_container->AppendChild(drag_element->Remove());
		}
		else
		{
			// The dragged element was dragged onto an item inside a container. In order to get the
			// element in the right place, it will be inserted into the container before the item
			// it was dragged on top of.
			Rml::Element* insert_before = dest_element;

			// Unless of course if it was dragged on top of an item in its own container; we need
			// to check then if it was moved up or down with the container.
			if (drag_element->GetParentElement() == dest_container)
			{
				// Check whether we're moving this icon from the left or the right.

				Rml::Element* previous_icon = insert_before->GetPreviousElementSibling();
				while (previous_icon != nullptr)
				{
					if (previous_icon == drag_element)
					{
						insert_before = insert_before->GetNextElementSibling();
						break;
					}

					previous_icon = previous_icon->GetPreviousElementSibling();
				}
			}

			Rml::NodePtr node = drag_element->GetParentElement()->RemoveChild(drag_element);
			dest_container->InsertBefore(std::move(node), insert_before);
		}
	}
}
