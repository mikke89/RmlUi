#pragma once

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/XMLNodeHandler.h>

struct MetaItem {
	Rml::String name;
	Rml::String content;
};
using MetaList = Rml::Vector<MetaItem>;

struct LinkItem {
	Rml::String rel;
	Rml::String href;
};
using LinkList = Rml::Vector<LinkItem>;

class XMLNodeHandlerMeta : public Rml::XMLNodeHandler {
public:
	XMLNodeHandlerMeta();
	~XMLNodeHandlerMeta();

	/// Called when a new element start is opened
	Rml::Element* ElementStart(Rml::XMLParser* parser, const Rml::String& name, const Rml::XMLAttributes& attributes) override;
	/// Called when an element is closed
	bool ElementEnd(Rml::XMLParser* parser, const Rml::String& name) override;
	/// Called for element data
	bool ElementData(Rml::XMLParser* parser, const Rml::String& data, Rml::XMLDataType type) override;

	const MetaList& GetMetaList() const { return meta_list; }
	void ClearMetaList() { meta_list.clear(); }

private:
	MetaList meta_list;
};

class XMLNodeHandlerLink : public Rml::XMLNodeHandler {
public:
	XMLNodeHandlerLink();
	~XMLNodeHandlerLink();

	/// Called when a new element start is opened
	Rml::Element* ElementStart(Rml::XMLParser* parser, const Rml::String& name, const Rml::XMLAttributes& attributes) override;
	/// Called when an element is closed
	bool ElementEnd(Rml::XMLParser* parser, const Rml::String& name) override;
	/// Called for element data
	bool ElementData(Rml::XMLParser* parser, const Rml::String& data, Rml::XMLDataType type) override;

	const LinkList& GetLinkList() const { return link_list; }
	void ClearLinkList() { link_list.clear(); }

private:
	LinkList link_list;
	Rml::XMLNodeHandler* node_handler_head;
};
