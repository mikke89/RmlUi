/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/Property.h"

namespace Rml {

ElementHandle::ElementHandle(const String& tag) : Element(tag), drag_start(0, 0)
{
	// Make sure we can be dragged!
	SetProperty(PropertyId::Drag, Property(Style::Drag::Drag));

	move_target = nullptr;
	size_target = nullptr;
	initialised = false;
}

ElementHandle::~ElementHandle() {}

void ElementHandle::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	// Reset initialised state if the move or size targets have changed.
	if (changed_attributes.find("move_target") != changed_attributes.end() || changed_attributes.find("size_target") != changed_attributes.end())
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

		auto GetSize = [](const Box& box, const ComputedValues& computed) {
			return box.GetSize((computed.box_sizing() == Style::BoxSizing::BorderBox) ? BoxArea::Border : BoxArea::Content);
		};

		// Set any auto margins to their current value, since auto-margins may affect the size and position of an element.
		auto SetDefiniteMargins = [](Element* element, const ComputedValues& computed) {
			auto SetDefiniteMargin = [](Element* element, PropertyId margin_id, BoxEdge edge) {
				element->SetProperty(margin_id, Property(Math::Round(element->GetBox().GetEdge(BoxArea::Margin, edge)), Unit::PX));
			};
			using Style::Margin;
			if (computed.margin_top().type == Margin::Auto)
				SetDefiniteMargin(element, PropertyId::MarginTop, BoxEdge::Top);
			if (computed.margin_right().type == Margin::Auto)
				SetDefiniteMargin(element, PropertyId::MarginRight, BoxEdge::Right);
			if (computed.margin_bottom().type == Margin::Auto)
				SetDefiniteMargin(element, PropertyId::MarginBottom, BoxEdge::Bottom);
			if (computed.margin_left().type == Margin::Auto)
				SetDefiniteMargin(element, PropertyId::MarginLeft, BoxEdge::Left);
		};

		if (event == EventId::Dragstart)
		{
			// Store the drag starting position
			drag_start = event.GetUnprojectedMouseScreenPos();

			// Store current element position and size
			if (move_target)
			{
				using namespace Style;
				const Box& box = move_target->GetBox();
				const auto& computed = move_target->GetComputedValues();

				// Store the initial margin edge position, since top/left properties determine the margin position.
				move_original_position.x = move_target->GetOffsetLeft() - box.GetEdge(BoxArea::Margin, BoxEdge::Left);
				move_original_position.y = move_target->GetOffsetTop() - box.GetEdge(BoxArea::Margin, BoxEdge::Top);

				// Check if we have auto-size together with definite right/bottom; if so, the size needs to be fixed to the current size.
				if (computed.width().type == Width::Auto && computed.right().type != Right::Auto)
					move_target->SetProperty(PropertyId::Width, Property(Math::Round(GetSize(box, computed).x), Unit::PX));
				if (computed.height().type == Height::Auto && computed.bottom().type != Bottom::Auto)
					move_target->SetProperty(PropertyId::Height, Property(Math::Round(GetSize(box, computed).y), Unit::PX));

				SetDefiniteMargins(move_target, computed);
			}
			if (size_target)
			{
				using namespace Style;
				const Box& box = size_target->GetBox();
				const auto& computed = size_target->GetComputedValues();

				size_original_size = GetSize(box, computed);

				SetDefiniteMargins(size_target, computed);
			}
		}
		else if (event == EventId::Drag)
		{
			const Vector2f delta = event.GetUnprojectedMouseScreenPos() - drag_start;

			if (move_target)
			{
				const Vector2f new_position = (move_original_position + delta).Round();
				move_target->SetProperty(PropertyId::Left, Property(new_position.x, Unit::PX));
				move_target->SetProperty(PropertyId::Top, Property(new_position.y, Unit::PX));
			}

			if (size_target)
			{
				const Vector2f new_size = Math::Max((size_original_size + delta).Round(), Vector2f(0.f));
				size_target->SetProperty(PropertyId::Width, Property(new_size.x, Unit::PX));
				size_target->SetProperty(PropertyId::Height, Property(new_size.y, Unit::PX));
			}

			Dictionary parameters;
			parameters["handle_x"] = delta.x;
			parameters["handle_y"] = delta.y;
			DispatchEvent(EventId::Handledrag, parameters);
		}
	}
}

} // namespace Rml
