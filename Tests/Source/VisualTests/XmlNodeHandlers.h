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

#ifndef RMLUI_TESTS_VISUALTESTS_XMLNODEHANDLERS_H
#define RMLUI_TESTS_VISUALTESTS_XMLNODEHANDLERS_H

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



class XMLNodeHandlerMeta : public Rml::XMLNodeHandler
{
public:
	XMLNodeHandlerMeta();
	~XMLNodeHandlerMeta();

	/// Called when a new element start is opened
	Rml::Element* ElementStart(Rml::XMLParser* parser, const Rml::String& name, const Rml::XMLAttributes& attributes) override;
	/// Called when an element is closed
	bool ElementEnd(Rml::XMLParser* parser, const Rml::String& name) override;
	/// Called for element data
	bool ElementData(Rml::XMLParser* parser, const Rml::String& data, Rml::XMLDataType type) override;

	const MetaList& GetMetaList() const {
		return meta_list;
	}
	void ClearMetaList() {
		meta_list.clear();
	}

private:
	MetaList meta_list;
};


class XMLNodeHandlerLink : public Rml::XMLNodeHandler
{
public:
	XMLNodeHandlerLink();
	~XMLNodeHandlerLink();

	/// Called when a new element start is opened
	Rml::Element* ElementStart(Rml::XMLParser* parser, const Rml::String& name, const Rml::XMLAttributes& attributes) override;
	/// Called when an element is closed
	bool ElementEnd(Rml::XMLParser* parser, const Rml::String& name) override;
	/// Called for element data
	bool ElementData(Rml::XMLParser* parser, const Rml::String& data, Rml::XMLDataType type) override;

	const LinkList& GetLinkList() const {
		return link_list;
	}
	void ClearLinkList() {
		link_list.clear();
	}

private:
	LinkList link_list;
	Rml::XMLNodeHandler* node_handler_head;
};


#endif
