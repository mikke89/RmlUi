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

#include "precompiled.h"
#include "DecoratorGradient.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"

/*
Gradient decorator usage in CSS:

decorator: gradient( direction start-color stop-color );

direction: horizontal|vertical;
start-color: #ff00ff;
stop-color: #00ff00;
*/

namespace Rml {
namespace Core {

//=======================================================

DecoratorGradient::DecoratorGradient()
{
}

DecoratorGradient::~DecoratorGradient()
{
}

bool DecoratorGradient::Initialise(const Direction &dir_, const Colourb &start_, const Colourb & stop_)
{
	dir = dir_;
	start = start_;
	stop = stop_;
	return true;
}

DecoratorDataHandle DecoratorGradient::GenerateElementData(Element* element) const
{
	auto *data = new Rml::Core::Geometry(element);
	Vector2f padded_size = element->GetBox().GetSize(Rml::Core::Box::PADDING);

	auto &vertices = data->GetVertices();
	vertices.resize(4);

	auto &indices = data->GetIndices();
	indices.resize(6);

	Rml::Core::GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), padded_size, start, 0);

	if (dir == Direction::Horizontal) {
		vertices[1].colour = vertices[2].colour = stop;
	} else if (dir == Direction::Vertical) {
		vertices[2].colour = vertices[3].colour = stop;
	}

	data->SetHostElement(element);
	return reinterpret_cast<DecoratorDataHandle>(data);
}

void DecoratorGradient::ReleaseElementData(DecoratorDataHandle element_data) const
{
	delete reinterpret_cast<Rml::Core::Geometry*>(element_data);
}

void DecoratorGradient::RenderElement(Element* element, DecoratorDataHandle element_data) const
{
	auto* data = reinterpret_cast<Rml::Core::Geometry*>(element_data);
	data->Render(element->GetAbsoluteOffset(Rml::Core::Box::PADDING).Round());
}

//=======================================================

DecoratorGradientInstancer::DecoratorGradientInstancer()
{
	// register properties for the decorator
	ids.direction = RegisterProperty("direction", "horizontal").AddParser("keyword", "horizontal, vertical").GetId();
	ids.start = RegisterProperty("start-color", "#ffffff").AddParser("color").GetId();
	ids.stop = RegisterProperty("stop-color", "#ffffff").AddParser("color").GetId();
	RegisterShorthand("decorator", "direction, start-color, stop-color", Rml::Core::ShorthandType::FallThrough);
}

DecoratorGradientInstancer::~DecoratorGradientInstancer()
{
}

std::shared_ptr<Rml::Core::Decorator> DecoratorGradientInstancer::InstanceDecorator(const String & RMLUI_UNUSED_PARAMETER(name), const PropertyDictionary& properties_,
	const Rml::Core::DecoratorInstancerInterface& RMLUI_UNUSED_PARAMETER(interface_))
{
	RMLUI_UNUSED(name);
	RMLUI_UNUSED(interface_);

	DecoratorGradient::Direction dir = (DecoratorGradient::Direction)properties_.GetProperty(ids.direction)->Get< int >();
	Colourb start = properties_.GetProperty(ids.start)->Get<Colourb>();
	Colourb stop = properties_.GetProperty(ids.stop)->Get<Colourb>();

	auto decorator = std::make_shared<DecoratorGradient>();
	if (decorator->Initialise(dir, start, stop)) {
		return decorator;
	}

	return nullptr;
}

}
}
