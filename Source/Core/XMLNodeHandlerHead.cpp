/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "XMLNodeHandlerHead.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/URL.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "DocumentHeader.h"
#include "TemplateCache.h"

namespace Rml {

static String Absolutepath(const String& source, const String& base)
{
	String joined_path;
	::Rml::GetSystemInterface()->JoinPath(joined_path, StringUtilities::Replace(base, '|', ':'), StringUtilities::Replace(source, '|', ':'));
	return StringUtilities::Replace(joined_path, ':', '|');
}

static DocumentHeader::Resource MakeInlineResource(XMLParser* parser, const String& data)
{
	DocumentHeader::Resource resource;
	resource.is_inline = true;
	resource.content = data;
	resource.path = parser->GetSourceURL().GetURL();
	resource.line = parser->GetLineNumberOpenTag();
	return resource;
}

static DocumentHeader::Resource MakeExternalResource(XMLParser* parser, const String& path)
{
	DocumentHeader::Resource resource;
	resource.is_inline = false;
	resource.path = Absolutepath(path, parser->GetSourceURL().GetURL());
	return resource;
}

XMLNodeHandlerHead::XMLNodeHandlerHead() {}

XMLNodeHandlerHead::~XMLNodeHandlerHead() {}

Element* XMLNodeHandlerHead::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	if (name == "head")
	{
		// Process the head attribute
		parser->GetDocumentHeader()->source = parser->GetSourceURL().GetURL();
	}

	// Is it a link tag?
	else if (name == "link")
	{
		// Lookup the type and href
		String type = StringUtilities::ToLower(Get<String>(attributes, "type", ""));
		String href = Get<String>(attributes, "href", "");

		if (!type.empty() && !href.empty())
		{
			// If its RCSS (... or CSS!), add to the RCSS fields.
			if (type == "text/rcss" || type == "text/css")
			{
				parser->GetDocumentHeader()->rcss.push_back(MakeExternalResource(parser, href));
			}

			// If its an template, add to the template fields
			else if (type == "text/template")
			{
				parser->GetDocumentHeader()->template_resources.push_back(href);
			}

			else
			{
				Log::ParseError(parser->GetSourceURL().GetURL(), parser->GetLineNumber(), "Invalid link type '%s'", type.c_str());
			}
		}
		else
		{
			Log::ParseError(parser->GetSourceURL().GetURL(), parser->GetLineNumber(), "Link tag requires type and href attributes");
		}
	}

	// Process script tags
	else if (name == "script")
	{
		// Check if its an external string
		String src = Get<String>(attributes, "src", "");
		if (src.size() > 0)
		{
			parser->GetDocumentHeader()->scripts.push_back(MakeExternalResource(parser, src));
		}
	}

	// No elements constructed
	return nullptr;
}

bool XMLNodeHandlerHead::ElementEnd(XMLParser* parser, const String& name)
{
	// When the head tag closes, inject the header into the active document
	if (name == "head")
	{
		Element* element = parser->GetParseFrame()->element;
		if (!element)
			return true;

		ElementDocument* document = element->GetOwnerDocument();
		if (document)
			document->ProcessHeader(parser->GetDocumentHeader());
	}
	return true;
}

bool XMLNodeHandlerHead::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
{
	const String& tag = parser->GetParseFrame()->tag;

	// Store the title
	if (tag == "title")
	{
		SystemInterface* system_interface = GetSystemInterface();
		if (system_interface != nullptr)
			system_interface->TranslateString(parser->GetDocumentHeader()->title, data);
	}

	// Store an inline script
	if (tag == "script" && data.size() > 0)
	{
		parser->GetDocumentHeader()->scripts.push_back(MakeInlineResource(parser, data));
	}

	// Store an inline style
	if (tag == "style" && data.size() > 0)
	{
		parser->GetDocumentHeader()->rcss.push_back(MakeInlineResource(parser, data));
	}

	return true;
}

} // namespace Rml
