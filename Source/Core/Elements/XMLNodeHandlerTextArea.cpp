#include "XMLNodeHandlerTextArea.h"
#include "../../../Include/RmlUi/Core/Core.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlTextArea.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/SystemInterface.h"
#include "../../../Include/RmlUi/Core/XMLParser.h"

namespace Rml {

XMLNodeHandlerTextArea::XMLNodeHandlerTextArea() {}

XMLNodeHandlerTextArea::~XMLNodeHandlerTextArea() {}

Element* XMLNodeHandlerTextArea::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	ElementFormControlTextArea* text_area = rmlui_dynamic_cast<ElementFormControlTextArea*>(parser->GetParseFrame()->element);
	if (!text_area)
	{
		ElementPtr new_element = Factory::InstanceElement(parser->GetParseFrame()->element, name, name, attributes);
		if (!new_element)
			return nullptr;

		Element* result = parser->GetParseFrame()->element->AppendChild(std::move(new_element));

		return result;
	}

	return nullptr;
}

bool XMLNodeHandlerTextArea::ElementEnd(XMLParser* /*parser*/, const String& /*name*/)
{
	return true;
}

bool XMLNodeHandlerTextArea::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
{
	ElementFormControlTextArea* text_area = rmlui_dynamic_cast<ElementFormControlTextArea*>(parser->GetParseFrame()->element);
	if (text_area != nullptr)
	{
		// Do any necessary translation.
		String translated_data;
		GetSystemInterface()->TranslateString(translated_data, data);

		text_area->SetValue(translated_data);
	}

	return true;
}

} // namespace Rml
