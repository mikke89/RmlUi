/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "XmlNodeHandlers.h"
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/XMLNodeHandler.h>
#include <RmlUi/Core/XMLParser.h>
#include <doctest.h>

using namespace Rml;



class XMLNodeHandlerMeta : public Rml::XMLNodeHandler
{
public:
	XMLNodeHandlerMeta(MetaList* meta_list) : meta_list(meta_list)
	{}
	~XMLNodeHandlerMeta()
	{}

	/// Called when a new element start is opened
	Element* ElementStart(XMLParser* /*parser*/, const String& /*name*/, const XMLAttributes& attributes) override
	{
		MetaItem item;

		auto it_name = attributes.find("name");
		if (it_name != attributes.end())
			item.name = it_name->second.Get<String>();
		
		auto it_content = attributes.find("content");
		if (it_content != attributes.end())
			item.content = it_content->second.Get<String>();

		if (!item.name.empty() && !item.content.empty())
			meta_list->push_back(std::move(item));

		return nullptr;
	}

	/// Called when an element is closed
	bool ElementEnd(XMLParser* /*parser*/, const String& /*name*/) override
	{
		return true;
	}
	/// Called for element data
	bool ElementData(XMLParser* /*parser*/, const String& /*data*/, XMLDataType /*type*/) override
	{
		return true;
	}

private:
	MetaList* meta_list;
};


class XMLNodeHandlerLink : public Rml::XMLNodeHandler
{
public:
	XMLNodeHandlerLink(LinkList* link_list) : link_list(link_list)
	{
		node_handler_head = XMLParser::GetNodeHandler("head");
		RMLUI_ASSERT(node_handler_head);
	}
	~XMLNodeHandlerLink() {}

	Element* ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes) override
	{
		RMLUI_ASSERT(name == "link");

		const String rel = StringUtilities::ToLower(Get<String>(attributes, "rel", ""));
		const String type = StringUtilities::ToLower(Get<String>(attributes, "type", ""));
		const String href = Get<String>(attributes, "href", "");

		if (!type.empty() && !href.empty())
		{
			// Pass it on to the head handler if it's a type it handles.
			if (type == "text/rcss" || type == "text/css" || type == "text/template")
			{
				return node_handler_head->ElementStart(parser, name, attributes);
			}
		}

		LinkItem item{ rel, href };

		if (rel == "match")
			item.href = StringUtilities::Replace(href, '/', '\\');
		else
			item.href = href;

		link_list->push_back(std::move(item));

		return nullptr;
	}

	bool ElementEnd(XMLParser* parser, const String& name) override
	{
		return node_handler_head->ElementEnd(parser, name);
	}
	bool ElementData(XMLParser* parser, const String& data, XMLDataType type) override
	{
		return node_handler_head->ElementData(parser, data, type);
	}

private:
	LinkList* link_list;
	Rml::XMLNodeHandler* node_handler_head;
};


static SharedPtr<XMLNodeHandlerMeta> meta_handler;
static SharedPtr<XMLNodeHandlerLink> link_handler;

void InitializeXmlNodeHandlers(MetaList* meta_list, LinkList* link_list)
{
	meta_handler = MakeShared<XMLNodeHandlerMeta>(meta_list);
	REQUIRE(meta_handler);
	Rml::XMLParser::RegisterNodeHandler("meta", meta_handler);

	link_handler = MakeShared<XMLNodeHandlerLink>(link_list);
	REQUIRE(link_handler);
	Rml::XMLParser::RegisterNodeHandler("link", link_handler);
}
