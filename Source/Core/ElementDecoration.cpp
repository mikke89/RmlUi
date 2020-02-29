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

#include "ElementDecoration.h"
#include "ElementDefinition.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Profiling.h"

namespace Rml {
namespace Core {

ElementDecoration::ElementDecoration(Element* _element)
{
	element = _element;
	decorators_dirty = false;
}

ElementDecoration::~ElementDecoration()
{
	ReleaseDecorators();
}

// Releases existing decorators and loads all decorators required by the element's definition.
bool ElementDecoration::ReloadDecorators()
{
	RMLUI_ZoneScopedC(0xB22222);
	ReleaseDecorators();

	auto& decorators_ptr = element->GetComputedValues().decorator;
	if (!decorators_ptr)
		return true;

	for (const auto& decorator : decorators_ptr->list)
	{
		if (decorator)
		{
			LoadDecorator(decorator);
		}
	}

	return true;
}

// Loads a single decorator and adds it to the list of loaded decorators for this element.
int ElementDecoration::LoadDecorator(SharedPtr<const Decorator> decorator)
{
	DecoratorHandle element_decorator;
	element_decorator.decorator_data = decorator->GenerateElementData(element);
	element_decorator.decorator = std::move(decorator);

	decorators.push_back(element_decorator);
	return (int) (decorators.size() - 1);
}

// Releases all existing decorators and frees their data.
void ElementDecoration::ReleaseDecorators()
{
	for (size_t i = 0; i < decorators.size(); i++)
	{
		if (decorators[i].decorator_data)
			decorators[i].decorator->ReleaseElementData(decorators[i].decorator_data);
	}

	decorators.clear();
}


void ElementDecoration::RenderDecorators()
{
	// @performance: Ignore dirty flag if e.g. pseudo classes do not affect the decorators
	if (decorators_dirty)
	{
		decorators_dirty = false;
		ReloadDecorators();
	}

	// Render the decorators attached to this element in its current state.
	// Render from back to front for correct render order.
	for (int i = (int)decorators.size() - 1; i >= 0; i--)
	{
		DecoratorHandle& decorator = decorators[i];
		decorator.decorator->RenderElement(element, decorator.decorator_data);
	}
}

void ElementDecoration::DirtyDecorators()
{
	decorators_dirty = true;
}

}
}
