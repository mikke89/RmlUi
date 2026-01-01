#include "XMLNodeHandlerTemplate.h"
#include "../../Include/RmlUi/Core/Dictionary.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "Template.h"
#include "TemplateCache.h"
#include "XMLParseTools.h"

namespace Rml {

XMLNodeHandlerTemplate::XMLNodeHandlerTemplate() {}

XMLNodeHandlerTemplate::~XMLNodeHandlerTemplate() {}

Element* XMLNodeHandlerTemplate::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	RMLUI_ASSERT(name == "template");
	(void)name;

	// Tell the parser to use the default handler for all child nodes
	parser->PushDefaultHandler();

	const String template_name = Get<String>(attributes, "src", "");
	Element* element = parser->GetParseFrame()->element;

	if (template_name.empty())
	{
		Log::Message(Log::LT_WARNING,
			"Inline template injection requires the 'src' attribute with the target template name, but none provided. In element %s",
			element->GetAddress().c_str());
		return element;
	}

	return XMLParseTools::ParseTemplate(element, template_name);
}

bool XMLNodeHandlerTemplate::ElementEnd(XMLParser* /*parser*/, const String& /*name*/)
{
	return true;
}

bool XMLNodeHandlerTemplate::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
{
	return Factory::InstanceElementText(parser->GetParseFrame()->element, data);
}

} // namespace Rml
