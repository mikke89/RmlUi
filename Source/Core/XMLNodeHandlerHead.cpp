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

#include "XMLNodeHandlerHead.h"
#include "DocumentHeader.h"
#include "TemplateCache.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/Core/URL.h"

namespace Rml {

XMLNodeHandlerHead::XMLNodeHandlerHead()
{
}

XMLNodeHandlerHead::~XMLNodeHandlerHead()
{
}

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
			if (type == "text/rcss" ||
				 type == "text/css")
			{
				parser->GetDocumentHeader()->rcss_external.push_back(href);
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
			parser->GetDocumentHeader()->scripts_external.push_back(src);
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

bool XMLNodeHandlerHead::ElementData(XMLParser* parser, const String& data, XMLDataType RMLUI_UNUSED_PARAMETER(type))
{
	RMLUI_UNUSED(type);
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
		parser->GetDocumentHeader()->scripts_inline.push_back(data);

	// Store an inline style
	if (tag == "style" && data.size() > 0)
	{
		parser->GetDocumentHeader()->rcss_inline.push_back(data);
		parser->GetDocumentHeader()->rcss_inline_line_numbers.push_back(parser->GetLineNumberOpenTag());
	}

	return true;
}

} // namespace Rml
