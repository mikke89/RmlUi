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

#include "ElementHandle.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/Event.h"

namespace Rml {
namespace Core {

ElementHandle::ElementHandle(const String& tag) : Element(tag), drag_start(0, 0)
{
	// Make sure we can be dragged!
	SetProperty(PropertyId::Drag, Property(Style::Drag::Drag));

	move_target = nullptr;
	size_target = nullptr;
	initialised = false;
}

ElementHandle::~ElementHandle()
{
}

void ElementHandle::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	// Reset initialised state if the move or size targets have changed.
	if (changed_attributes.find("move_target") != changed_attributes.end() ||
		changed_attributes.find("size_target") != changed_attributes.end())
	{
		initialised = false;
		move_target = nullptr;
		size_target = nullptr;
	}
}

void ElementHandle::ProcessDefaultAction(Event& event)
{
	Element::ProcessDefaultAction(event);

	if (event.GetTargetElement() == this)
	{
		// Lazy initialisation.
		if (!initialised && GetOwnerDocument())
		{
			String move_target_name = GetAttribute<String>("move_target", "");
			if (!move_target_name.empty())
				move_target = GetElementById(move_target_name);

			String size_target_name = GetAttribute<String>("size_target", "");
			if (!size_target_name.empty())
				size_target = GetElementById(size_target_name);

			initialised = true;
		}

		if (event == EventId::Dragstart)
		{
			// Store the drag starting position
			drag_start = event.GetUnprojectedMouseScreenPos();

			// Store current element position and size
			if (move_target)
			{
				move_original_position.x = move_target->GetOffsetLeft();
				move_original_position.y = move_target->GetOffsetTop();
			}
			if (size_target)
				size_original_size = size_target->GetBox().GetSize(Box::CONTENT);
		}
		else if (event == EventId::Drag)
		{
			// Work out the delta
			Vector2f delta = event.GetUnprojectedMouseScreenPos() - drag_start;

			// Update the move and size objects
			if (move_target)
			{
				move_target->SetProperty(PropertyId::Left, Property(Math::RoundFloat(move_original_position.x + delta.x), Property::PX));
				move_target->SetProperty(PropertyId::Top, Property(Math::RoundFloat(move_original_position.y + delta.y), Property::PX));
			}

			if (size_target)
			{
				using namespace Style;
				const auto& computed = size_target->GetComputedValues();

				// Check if we have auto-margins; if so, they have to be set to the current margins.
				if (computed.margin_top.type == Margin::Auto)
					size_target->SetProperty(PropertyId::MarginTop, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::TOP)), Property::PX));
				if (computed.margin_right.type == Margin::Auto)
					size_target->SetProperty(PropertyId::MarginRight, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::RIGHT)), Property::PX));
				if (computed.margin_bottom.type == Margin::Auto)
					size_target->SetProperty(PropertyId::MarginBottom, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::BOTTOM)), Property::PX));
				if (computed.margin_left.type == Margin::Auto)
					size_target->SetProperty(PropertyId::MarginLeft, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::LEFT)), Property::PX));

				float new_x = Math::RoundFloat(size_original_size.x + delta.x);
				float new_y = Math::RoundFloat(size_original_size.y + delta.y);

				size_target->SetProperty(PropertyId::Width, Property(Math::Max< float >(new_x, 0.f), Property::PX));
				size_target->SetProperty(PropertyId::Height, Property(Math::Max< float >(new_y, 0.f), Property::PX));
			}

			Dictionary parameters;
			parameters["handle_x"] = delta.x;
			parameters["handle_y"] = delta.y;
			DispatchEvent(EventId::Handledrag, parameters);
		}
	}
}

}
}
