#include "DataControllerDefault.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "DataController.h"
#include "DataExpression.h"
#include "DataModel.h"
#include "EventSpecification.h"

namespace Rml {

DataControllerValue::DataControllerValue(Element* element) : DataController(element) {}

DataControllerValue::~DataControllerValue()
{
	if (Element* element = GetElement())
		element->RemoveEventListener(EventId::Change, this);
}

bool DataControllerValue::Initialize(DataModel& model, Element* element, const String& variable_name, const String& /*modifier*/)
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
	if (const Element* element = GetElement())
	{
		Variant value_to_set;
		const auto& parameters = event.GetParameters();
		const auto override_value_it = parameters.find("data-binding-override-value");
		const auto value_it = parameters.find("value");
		if (override_value_it != parameters.cend())
			value_to_set = override_value_it->second;
		else if (value_it != parameters.cend())
			value_to_set = value_it->second;
		else
			Log::Message(Log::LT_WARNING,
				"A 'change' event was received, but it did not contain the attribute 'value' when processing a data binding in %s",
				element->GetAddress().c_str());

		DataModel* model = element->GetDataModel();
		if (value_to_set.GetType() == Variant::NONE || !model)
			return;

		if (DataVariable variable = model->GetVariable(address))
			if (variable.Set(value_to_set))
				model->DirtyVariable(address.front().name);
	}
}

void DataControllerValue::Release()
{
	delete this;
}

DataControllerEvent::DataControllerEvent(Element* element) : DataController(element) {}

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

	expression = MakeUnique<DataExpression>(expression_str);
	DataExpressionInterface expr_interface(&model, element);

	if (!expression->Parse(expr_interface, true))
		return false;

	id = EventSpecificationInterface::GetIdOrInsert(modifier);
	if (id == EventId::Invalid)
	{
		Log::Message(Log::LT_WARNING, "Event type '%s' could not be recognized, while adding 'data-event' to %s", modifier.c_str(),
			element->GetAddress().c_str());
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
		DataExpressionInterface expr_interface(element->GetDataModel(), element, &event);
		Variant unused_value_out;
		expression->Run(expr_interface, unused_value_out);
	}
}

void DataControllerEvent::Release()
{
	delete this;
}

} // namespace Rml
