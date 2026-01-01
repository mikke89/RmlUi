#include "XMLNodeHandlerBody.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "XMLParseTools.h"

namespace Rml {

XMLNodeHandlerBody::XMLNodeHandlerBody() {}

XMLNodeHandlerBody::~XMLNodeHandlerBody() {}

Element* XMLNodeHandlerBody::ElementStart(XMLParser* parser, const String& /*name*/, const XMLAttributes& attributes)
{
	Element* element = parser->GetParseFrame()->element;

	// Apply any attributes to the document, but only if the current element is the root of the current document,
	// which should only hold for body elements, but not for inline injected templates.
	ElementDocument* document = parser->GetParseFrame()->element->GetOwnerDocument();
	if (document && document == element)
	{
		for (auto& pair : attributes)
		{
			Variant* attribute = document->GetAttribute(pair.first);
			if (attribute && *attribute != pair.second && pair.first != "template")
			{
				Log::Message(Log::LT_WARNING, "Overriding attribute '%s' in element %s during template injection.", pair.first.c_str(),
					element->GetAddress().c_str());
			}
		}

		document->SetAttributes(attributes);
	}

	// Check for and apply any template
	String template_name = Get<String>(attributes, "template", "");
	if (!template_name.empty())
	{
		element = XMLParseTools::ParseTemplate(element, template_name);
	}

	// Tell the parser to use the element handler for all children
	parser->PushDefaultHandler();

	return element;
}

bool XMLNodeHandlerBody::ElementEnd(XMLParser* /*parser*/, const String& /*name*/)
{
	return true;
}

bool XMLNodeHandlerBody::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
{
	return Factory::InstanceElementText(parser->GetParseFrame()->element, data);
}

} // namespace Rml
