#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/XMLNodeHandler.h>

struct MetaItem {
	Rml::String name;
	Rml::String content;
};

class XMLNodeHandlerSandboxMeta : public Rml::XMLNodeHandler {
public:
	XMLNodeHandlerSandboxMeta();
	~XMLNodeHandlerSandboxMeta();

	Rml::Element* ElementStart(Rml::XMLParser* parser, const Rml::String& name, const Rml::XMLAttributes& attributes) override;
	bool ElementEnd(Rml::XMLParser* parser, const Rml::String& name) override;
	bool ElementData(Rml::XMLParser* parser, const Rml::String& data, Rml::XMLDataType type) override;

	const Rml::Vector<MetaItem>& GetMetaList() const { return meta_list; }
	void ClearMetaList() { meta_list.clear(); }

private:
	Rml::Vector<MetaItem> meta_list;
};
