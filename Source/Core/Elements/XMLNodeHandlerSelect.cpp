#include "XMLNodeHandlerSelect.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlSelect.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/XMLParser.h"

namespace Rml {

XMLNodeHandlerSelect::XMLNodeHandlerSelect() {}

XMLNodeHandlerSelect::~XMLNodeHandlerSelect() {}

Element* XMLNodeHandlerSelect::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	RMLUI_ASSERT(name == "select" || name == "option");

	if (name == "select")
	{
		// Call this node handler for all children
		parser->PushHandler("select");

		// Attempt to instance the tabset
		ElementPtr element = Factory::InstanceElement(parser->GetParseFrame()->element, name, name, attributes);
		ElementFormControlSelect* select_element = rmlui_dynamic_cast<ElementFormControlSelect*>(element.get());
		if (!select_element)
		{
			Log::Message(Log::LT_ERROR, "Instancer failed to create element for tag %s.", name.c_str());
			return nullptr;
		}

		// Add the Select element into the document
		Element* result = parser->GetParseFrame()->element->AppendChild(std::move(element));

		return result;
	}
	else if (name == "option")
	{
		// Call default element handler for all children.
		parser->PushDefaultHandler();

		ElementPtr option_element = Factory::InstanceElement(parser->GetParseFrame()->element, name, name, attributes);
		Element* result = nullptr;

		ElementFormControlSelect* select_element = rmlui_dynamic_cast<ElementFormControlSelect*>(parser->GetParseFrame()->element);
		if (select_element)
		{
			result = option_element.get();
			select_element->Add(std::move(option_element));
		}

		return result;
	}

	return nullptr;
}

} // namespace Rml
