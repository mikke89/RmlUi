#include "DataController.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "EventSpecification.h"

namespace Rml {

DataController::DataController(Element* element) : attached_element(element->GetObserverPtr()) {}

DataController::~DataController() {}
Element* DataController::GetElement() const
{
	return attached_element.get();
}

bool DataController::IsValid() const
{
	return static_cast<bool>(attached_element);
}

DataControllers::DataControllers() {}

DataControllers::~DataControllers() {}

void DataControllers::Add(DataControllerPtr controller)
{
	RMLUI_ASSERT(controller);

	Element* element = controller->GetElement();
	RMLUI_ASSERTMSG(element, "Invalid controller, make sure it is valid before adding");
	if (!element)
		return;

	controllers.emplace(element, std::move(controller));
}

void DataControllers::OnElementRemove(Element* element)
{
	controllers.erase(element);
}

} // namespace Rml
