#include "XmlNodeHandlers.h"
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/XMLNodeHandler.h>
#include <RmlUi/Core/XMLParser.h>

using namespace Rml;

XMLNodeHandlerMeta::XMLNodeHandlerMeta() {}
XMLNodeHandlerMeta::~XMLNodeHandlerMeta() {}

Element* XMLNodeHandlerMeta::ElementStart(XMLParser* /*parser*/, const String& /*name*/, const XMLAttributes& attributes)
{
	MetaItem item;

	auto it_name = attributes.find("name");
	if (it_name != attributes.end())
		item.name = it_name->second.Get<String>();

	auto it_content = attributes.find("content");
	if (it_content != attributes.end())
		item.content = it_content->second.Get<String>();

	if (!item.name.empty() && !item.content.empty())
		meta_list.push_back(std::move(item));

	return nullptr;
}

bool XMLNodeHandlerMeta::ElementEnd(XMLParser* /*parser*/, const String& /*name*/)
{
	return true;
}
bool XMLNodeHandlerMeta::ElementData(XMLParser* /*parser*/, const String& /*data*/, XMLDataType /*type*/)
{
	return true;
}

XMLNodeHandlerLink::XMLNodeHandlerLink()
{
	node_handler_head = XMLParser::GetNodeHandler("head");
	RMLUI_ASSERT(node_handler_head);
}
XMLNodeHandlerLink::~XMLNodeHandlerLink() {}

Element* XMLNodeHandlerLink::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	RMLUI_ASSERT(name == "link");

	const String type = StringUtilities::ToLower(Get<String>(attributes, "type", ""));
	const String rel = Get<String>(attributes, "rel", "");
	const String href = Get<String>(attributes, "href", "");

	if (!type.empty() && !href.empty())
	{
		// Pass it on to the head handler if it's a type it handles.
		if (type == "text/rcss" || type == "text/css" || type == "text/template")
		{
			return node_handler_head->ElementStart(parser, name, attributes);
		}
	}

	link_list.push_back(LinkItem{rel, href});

	return nullptr;
}

bool XMLNodeHandlerLink::ElementEnd(XMLParser* parser, const String& name)
{
	return node_handler_head->ElementEnd(parser, name);
}
bool XMLNodeHandlerLink::ElementData(XMLParser* parser, const String& data, XMLDataType type)
{
	return node_handler_head->ElementData(parser, data, type);
}
