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

using namespace Rml;




XMLNodeHandlerMeta::XMLNodeHandlerMeta()
{}
XMLNodeHandlerMeta::~XMLNodeHandlerMeta()
{}

/// Called when a new element start is opened
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

/// Called when an element is closed
bool XMLNodeHandlerMeta::ElementEnd(XMLParser* /*parser*/, const String& /*name*/)
{
	return true;
}
/// Called for element data
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

	link_list.push_back(LinkItem{ rel, href });

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

