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

#include "DataControllerDefault.h"
#include "../../Include/RmlUi/Core/DataController.h"
#include "../../Include/RmlUi/Core/DataModel.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "DataExpression.h"
#include "EventSpecification.h"

namespace Rml {
namespace Core {



DataControllerValue::DataControllerValue(Element* element) : DataController(element)
{}

DataControllerValue::~DataControllerValue()
{
	if (Element* element = GetElement())
	{
		element->RemoveEventListener(EventId::Change, this);
	}
}

bool DataControllerValue::Initialize(DataModel& model, Element* element, const String& variable_name, const String& modifier)
{
	RMLUI_ASSERT(element);

	DataAddress variable_address = model.ResolveAddress(variable_name, element);
	if (variable_address.empty())
		return false;

	if (model.GetVariable(variable_address))
		address = std::move(variable_address);
	
	element->AddEventListener(EventId::Change, this);

	return true;
}

void DataControllerValue::ProcessEvent(Event& event)
{
	if (Element* element = GetElement())
	{
		const auto& parameters = event.GetParameters();
		auto it = parameters.find("value");
		if (it == parameters.end())
		{
			Log::Message(Log::LT_WARNING, "A 'change' event was received, but it did not contain a value. During processing of 'data-value' in %s", element->GetAddress().c_str());
			return;
		}

		SetValue(it->second);
	}
}

void DataControllerValue::Release()
{
	delete this;
}

void DataControllerValue::SetValue(const Variant& value)
{
	Element* element = GetElement();
	if (!element)
		return;

	DataModel* model = element->GetDataModel();
	if (!model)
		return;

	if (DataVariable variable = model->GetVariable(address))
	{
		variable.Set(value);
		model->DirtyVariable(address.front().name);
	}
}


DataControllerEvent::DataControllerEvent(Element* element) : DataController(element)
{}

DataControllerEvent::~DataControllerEvent()
{
	if (Element* element = GetElement())
	{
		if (id != EventId::Invalid)
			element->RemoveEventListener(id, this);
	}
}

bool DataControllerEvent::Initialize(DataModel& model, Element* element, const String& expression_str, const String& modifier)
{
	RMLUI_ASSERT(element);

	expression = std::make_unique<DataExpression>(expression_str);
	DataExpressionInterface interface(&model, element);

	if (!expression->Parse(interface, true))
		return false;

	id = EventSpecificationInterface::GetIdOrInsert(modifier);
	if (id == EventId::Invalid)
	{
		Log::Message(Log::LT_WARNING, "Event type '%s' could not be recognized, while adding 'data-event' to %s", modifier.c_str(), element->GetAddress().c_str());
		return false;
	}

	element->AddEventListener(id, this);

	return true;
}

void DataControllerEvent::ProcessEvent(Event& event)
{
	if (!expression)
		return;

	if (Element* element = GetElement())
	{
		DataExpressionInterface interface(element->GetDataModel(), element, &event);
		Variant unused_value_out;
		expression->Run(interface, unused_value_out);
	}
}

void DataControllerEvent::Release()
{
	delete this;
}

}
}
