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

#ifndef RMLUI_CORE_DECORATORGRADIENT_H
#define RMLUI_CORE_DECORATORGRADIENT_H

#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/Property.h"

namespace Rml {

class DecoratorGradient : public Decorator
{
public:
	enum class Direction { Horizontal = 0, Vertical = 1 };

	DecoratorGradient();
	virtual ~DecoratorGradient();

	bool Initialise(const Direction &dir_, const Colourb &start_, const Colourb & stop_);

	DecoratorDataHandle GenerateElementData(Element* element) const override;
	void ReleaseElementData(DecoratorDataHandle element_data) const override;

	void RenderElement(Element* element, DecoratorDataHandle element_data) const override;

private:
	Direction dir;
	Colourb start, stop;
};



class DecoratorGradientInstancer : public DecoratorInstancer
{
public:
	DecoratorGradientInstancer();
	~DecoratorGradientInstancer();

	SharedPtr<Decorator> InstanceDecorator(const String& name, const PropertyDictionary& properties, const DecoratorInstancerInterface& interface) override;

private:
	struct GradientPropertyIds {
		PropertyId direction, start, stop;
	};
	GradientPropertyIds ids;

};

} // namespace Rml
#endif
