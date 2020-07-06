/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include "DragListener.h"
#include <RmlUi/Core/Element.h>

static DragListener drag_listener;

// Registers an element as being a container of draggable elements.
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
		Rml::Element* drag_element = static_cast< Rml::Element* >(event.GetParameter< void* >("drag_element", nullptr));

		if (dest_container == dest_element)
		{
			// The dragged element was dragged directly onto a container.
			Rml::ElementPtr element = drag_element->GetParentNode()->RemoveChild(drag_element);
			dest_container->AppendChild(std::move(element));
		}
		else
		{
			// The dragged element was dragged onto an item inside a container. In order to get the
			// element in the right place, it will be inserted into the container before the item
			// it was dragged on top of.
			Rml::Element* insert_before = dest_element;

			// Unless of course if it was dragged on top of an item in its own container; we need
			// to check then if it was moved up or down with the container.
			if (drag_element->GetParentNode() == dest_container)
			{
				// Check whether we're moving this icon from the left or the right.

				Rml::Element* previous_icon = insert_before->GetPreviousSibling();
				while (previous_icon != nullptr)
				{
					if (previous_icon == drag_element)
					{
						insert_before = insert_before->GetNextSibling();
						break;
					}

					previous_icon = previous_icon->GetPreviousSibling();
				}
			}

			Rml::ElementPtr element = drag_element->GetParentNode()->RemoveChild(drag_element);
			dest_container->InsertBefore(std::move(element), insert_before);
		}
	}
}
