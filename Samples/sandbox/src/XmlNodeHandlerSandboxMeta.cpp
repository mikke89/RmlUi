#include "XmlNodeHandlerSandboxMeta.h"
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/XMLNodeHandler.h>
#include <RmlUi/Core/XMLParser.h>

XMLNodeHandlerSandboxMeta::XMLNodeHandlerSandboxMeta() {}
XMLNodeHandlerSandboxMeta::~XMLNodeHandlerSandboxMeta() {}

Rml::Element* XMLNodeHandlerSandboxMeta::ElementStart(Rml::XMLParser* /*parser*/, const Rml::String& /*name*/, const Rml::XMLAttributes& attributes)
{
	MetaItem item;

	auto it_name = attributes.find("name");
	if (it_name != attributes.end())
		item.name = it_name->second.Get<Rml::String>();

	auto it_content = attributes.find("content");
	if (it_content != attributes.end())
		item.content = it_content->second.Get<Rml::String>();

	if (!item.name.empty() && !item.content.empty())
		meta_list.push_back(std::move(item));

	return nullptr;
}

bool XMLNodeHandlerSandboxMeta::ElementEnd(Rml::XMLParser* /*parser*/, const Rml::String& /*name*/)
{
	return true;
}
bool XMLNodeHandlerSandboxMeta::ElementData(Rml::XMLParser* /*parser*/, const Rml::String& /*data*/, Rml::XMLDataType /*type*/)
{
	return true;
}
