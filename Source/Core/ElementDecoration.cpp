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
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"

namespace Rml {

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

	if (!element->GetComputedValues().has_decorator)
		return true;

	const Property* property = element->GetLocalProperty(PropertyId::Decorator);
	if (!property || property->unit != Property::DECORATOR)
		return false;

	DecoratorsPtr decorators = property->Get<DecoratorsPtr>();
	if (!decorators)
		return false;

	const StyleSheet* style_sheet = element->GetStyleSheet();
	if (!style_sheet)
		return false;

	PropertySource document_source("", 0, "");
	const PropertySource* source = property->source.get();

	if (!source)
	{
		if (ElementDocument* document = element->GetOwnerDocument())
		{
			document_source.path = document->GetSourceURL();
			source = &document_source;
		}
	}

	for (const DecoratorDeclaration& decorator : decorators->list)
	{
		if (decorator.instancer)
		{
			RMLUI_ZoneScopedN("InstanceDecorator");
			SharedPtr<const Decorator> decorator_instance = decorator.instancer->InstanceDecorator(decorator.type, decorator.properties, DecoratorInstancerInterface(*style_sheet, source));

			if (decorator_instance)
				LoadDecorator(std::move(decorator_instance));
			else
				Log::Message(Log::LT_WARNING, "Decorator '%s' in '%s' could not be instanced, declared at %s:%d", decorator.type.c_str(), decorators->value.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
		}
		else
		{
			// If we have no instancer, this means the type is the name of an @decorator rule.
			SharedPtr<const Decorator> decorator_instance = style_sheet->GetDecorator(decorator.type);

			if (decorator_instance)
				LoadDecorator(std::move(decorator_instance));
			else
				Log::Message(Log::LT_WARNING, "Decorator name '%s' could not be found in any @decorator rule, declared at %s:%d", decorator.type.c_str(), source ? source->path.c_str() : "", source ? source->line_number : -1);
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

} // namespace Rml
