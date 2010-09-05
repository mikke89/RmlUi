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

#include "DragListener.h"
#include <Rocket/Core/Element.h>

static DragListener drag_listener;

// Registers an element as being a container of draggable elements.
void DragListener::RegisterDraggableContainer(Rocket::Core::Element* element)
{
	element->AddEventListener("dragdrop", &drag_listener);
}

void DragListener::ProcessEvent(Rocket::Core::Event& event)
{
	if (event == "dragdrop")
	{
		Rocket::Core::Element* dest_container = event.GetCurrentElement();
		Rocket::Core::Element* dest_element = event.GetTargetElement();
		Rocket::Core::Element* drag_element = static_cast< Rocket::Core::Element* >(event.GetParameter< void* >("drag_element", NULL));

		if (dest_container == dest_element)
		{
			// The dragged element was dragged directly onto a container.
			drag_element->GetParentNode()->RemoveChild(drag_element);
			dest_container->AppendChild(drag_element);
		}
		else
		{
			// The dragged element was dragged onto an item inside a container. In order to get the
			// element in the right place, it will be inserted into the container before the item
			// it was dragged on top of.
			Rocket::Core::Element* insert_before = dest_element;

			// Unless of course if it was dragged on top of an item in its own container; we need
			// to check then if it was moved up or down with the container.
			if (drag_element->GetParentNode() == dest_container)
			{
				// Check whether we're moving this icon from the left or the right.

				Rocket::Core::Element* previous_icon = insert_before->GetPreviousSibling();
				while (previous_icon != NULL)
				{
					if (previous_icon == drag_element)
					{
						insert_before = insert_before->GetNextSibling();
						break;
					}

					previous_icon = previous_icon->GetPreviousSibling();
				}
			}

			drag_element->GetParentNode()->RemoveChild(drag_element);
			dest_container->InsertBefore(drag_element, insert_before);
		}
	}
}
