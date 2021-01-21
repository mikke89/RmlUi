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

#include "XMLNodeHandlerBody.h"
#include "XMLParseTools.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Factory.h"

namespace Rml {

XMLNodeHandlerBody::XMLNodeHandlerBody()
{
}

XMLNodeHandlerBody::~XMLNodeHandlerBody()
{
}

Element* XMLNodeHandlerBody::ElementStart(XMLParser* parser, const String& RMLUI_UNUSED_PARAMETER(name), const XMLAttributes& attributes)
{
	RMLUI_UNUSED(name);

	Element* element = parser->GetParseFrame()->element;

	// Check for and apply any template
	String template_name = Get<String>(attributes, "template", "");
	if (!template_name.empty())
	{
		element = XMLParseTools::ParseTemplate(element, template_name);
	}

	// Apply any attributes to the document
	ElementDocument* document = parser->GetParseFrame()->element->GetOwnerDocument();
	if (document)
		document->SetAttributes(attributes);

	// Tell the parser to use the element handler for all children
	parser->PushDefaultHandler();
	
	return element;
}

bool XMLNodeHandlerBody::ElementEnd(XMLParser* RMLUI_UNUSED_PARAMETER(parser), const String& RMLUI_UNUSED_PARAMETER(name))
{
	RMLUI_UNUSED(parser);
	RMLUI_UNUSED(name);

	return true;
}

bool XMLNodeHandlerBody::ElementData(XMLParser* parser, const String& data, XMLDataType RMLUI_UNUSED_PARAMETER(type))
{
	RMLUI_UNUSED(type);
	return Factory::InstanceElementText(parser->GetParseFrame()->element, data);
}

} // namespace Rml
