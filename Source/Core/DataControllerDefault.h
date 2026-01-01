#pragma once

#include "../../Include/RmlUi/Core/DataVariable.h"
#include "../../Include/RmlUi/Core/EventListener.h"
#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "DataController.h"

namespace Rml {

class Element;
class DataModel;
class DataExpression;
using DataExpressionPtr = UniquePtr<DataExpression>;

class DataControllerValue : public DataController, private EventListener {
public:
	DataControllerValue(Element* element);
	~DataControllerValue();

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

private:
	// Responds to 'Change' events.
	void ProcessEvent(Event& event) override;

	// Delete this.
	void Release() override;

	DataAddress address;
};

class DataControllerEvent final : public DataController, private EventListener {
public:
	DataControllerEvent(Element* element);
	~DataControllerEvent();

	bool Initialize(DataModel& model, Element* element, const String& expression, const String& modifier) override;

protected:
	// Responds to the event type specified in the attribute modifier.
	void ProcessEvent(Event& event) override;

	// Delete this.
	void Release() override;

private:
	EventId id = EventId::Invalid;
	DataExpressionPtr expression;
};

} // namespace Rml
