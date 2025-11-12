#include "XMLNodeHandlerTabSet.h"
#include "../../../Include/RmlUi/Core/Elements/ElementTabSet.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/XMLParser.h"

namespace Rml {

XMLNodeHandlerTabSet::XMLNodeHandlerTabSet() {}

XMLNodeHandlerTabSet::~XMLNodeHandlerTabSet() {}

Element* XMLNodeHandlerTabSet::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	RMLUI_ASSERT(name == "tabset" || name == "tabs" || name == "tab" || name == "panels" || name == "panel");

	if (name == "tabset")
	{
		// Call this node handler for all children
		parser->PushHandler("tabset");

		NodePtr Node = Factory::InstanceNode(name, name);
		ElementTabSet* tabset = rmlui_dynamic_cast<ElementTabSet*>(Node.get());
		if (!tabset)
		{
			Log::Message(Log::LT_ERROR, "Instancer failed to create element for tag %s.", name.c_str());
			return nullptr;
		}
		tabset->SetAttributes(attributes);

		parser->GetParseFrame()->element->AppendChild(std::move(Node));

		return tabset;
	}
	else if (name == "tab")
	{
		// Call default element handler for all children.
		parser->PushDefaultHandler();

		ElementPtr tab_element = As<ElementPtr>(Factory::InstanceNode("*", "tab"));
		tab_element->SetAttributes(attributes);

		Element* result = nullptr;
		ElementTabSet* tabset = rmlui_dynamic_cast<ElementTabSet*>(parser->GetParseFrame()->element);
		if (tabset)
		{
			result = tab_element.get();
			tabset->SetTab(-1, std::move(tab_element));
		}

		return result;
	}
	else if (name == "panel")
	{
		// Call default element handler for all children.
		parser->PushDefaultHandler();

		ElementPtr panel_element = As<ElementPtr>(Factory::InstanceNode("*", "panel"));
		panel_element->SetAttributes(attributes);

		Element* result = nullptr;
		ElementTabSet* tabset = rmlui_dynamic_cast<ElementTabSet*>(parser->GetParseFrame()->element);
		if (tabset)
		{
			result = panel_element.get();
			tabset->SetPanel(-1, std::move(panel_element));
		}

		return result;
	}
	else if (name == "tabs" || name == "panels")
	{
		// Use the element handler to add the tabs and panels elements to the the tabset (this allows users to
		// style them nicely), but don't return the new element, as we still want the tabset to be the top of the
		// parser's node stack.

		Element* parent = parser->GetParseFrame()->element;

		ElementPtr element = As<ElementPtr>(Factory::InstanceNode(name, name));
		element->SetAttributes(attributes);

		parent->AppendChild(std::move(element));

		return nullptr;
	}

	return nullptr;
}

bool XMLNodeHandlerTabSet::ElementEnd(XMLParser* /*parser*/, const String& /*name*/)
{
	return true;
}

bool XMLNodeHandlerTabSet::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
{
	return Factory::InstanceElementText(parser->GetParseFrame()->element, data);
}

} // namespace Rml
