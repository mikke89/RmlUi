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

#include "DecoratorGradient.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"

namespace Rml {

DecoratorStraightGradient::DecoratorStraightGradient() {}

DecoratorStraightGradient::~DecoratorStraightGradient() {}

bool DecoratorStraightGradient::Initialise(const Direction in_direction, const Colourb in_start, const Colourb in_stop)
{
	direction = in_direction;
	start = in_start;
	stop = in_stop;
	return true;
}

DecoratorDataHandle DecoratorStraightGradient::GenerateElementData(Element* element) const
{
	Geometry* geometry = new Geometry();
	const Box& box = element->GetBox();

	const ComputedValues& computed = element->GetComputedValues();
	const float opacity = computed.opacity();

	GeometryUtilities::GenerateBackground(geometry, element->GetBox(), Vector2f(0), computed.border_radius(), Colourb());

	// Apply opacity
	Colourb colour_start = start;
	colour_start.alpha = (byte)(opacity * (float)colour_start.alpha);
	Colourb colour_stop = stop;
	colour_stop.alpha = (byte)(opacity * (float)colour_stop.alpha);

	const Vector2f padding_offset = box.GetPosition(BoxArea::Padding);
	const Vector2f padding_size = box.GetSize(BoxArea::Padding);

	Vector<Vertex>& vertices = geometry->GetVertices();

	if (direction == Direction::Horizontal)
	{
		for (int i = 0; i < (int)vertices.size(); i++)
		{
			const float t = Math::Clamp((vertices[i].position.x - padding_offset.x) / padding_size.x, 0.0f, 1.0f);
			vertices[i].colour = Math::RoundedLerp(t, colour_start, colour_stop);
		}
	}
	else if (direction == Direction::Vertical)
	{
		for (int i = 0; i < (int)vertices.size(); i++)
		{
			const float t = Math::Clamp((vertices[i].position.y - padding_offset.y) / padding_size.y, 0.0f, 1.0f);
			vertices[i].colour = Math::RoundedLerp(t, colour_start, colour_stop);
		}
	}

	return reinterpret_cast<DecoratorDataHandle>(geometry);
}

void DecoratorStraightGradient::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<Geometry*>(element_data);
}

void DecoratorStraightGradient::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	auto* data = reinterpret_cast<Geometry*>(element_data);
	data->Render(element->GetAbsoluteOffset(BoxArea::Border));
}

DecoratorStraightGradientInstancer::DecoratorStraightGradientInstancer()
{
	ids.direction = RegisterProperty("direction", "horizontal").AddParser("keyword", "horizontal, vertical").GetId();
	ids.start = RegisterProperty("start-color", "#ffffff").AddParser("color").GetId();
	ids.stop = RegisterProperty("stop-color", "#ffffff").AddParser("color").GetId();
	RegisterShorthand("decorator", "direction, start-color, stop-color", ShorthandType::FallThrough);
}

DecoratorStraightGradientInstancer::~DecoratorStraightGradientInstancer() {}

SharedPtr<Decorator> DecoratorStraightGradientInstancer::InstanceDecorator(const String& name, const PropertyDictionary& properties_,
	const DecoratorInstancerInterface& /*interface_*/)
{
	using Direction = DecoratorStraightGradient::Direction;
	Direction direction;
	if (name == "horizontal-gradient")
		direction = Direction::Horizontal;
	else if (name == "vertical-gradient")
		direction = Direction::Vertical;
	else
	{
		direction = (Direction)properties_.GetProperty(ids.direction)->Get<int>();
		Log::Message(Log::LT_WARNING,
			"Decorator syntax 'gradient(horizontal|vertical ...)' is deprecated, please replace with 'horizontal-gradient(...)' or "
			"'vertical-gradient(...)'");
	}

	Colourb start = properties_.GetProperty(ids.start)->Get<Colourb>();
	Colourb stop = properties_.GetProperty(ids.stop)->Get<Colourb>();

	auto decorator = MakeShared<DecoratorStraightGradient>();
	if (decorator->Initialise(direction, start, stop))
		return decorator;

	return nullptr;
}

} // namespace Rml
