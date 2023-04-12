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

#include "XMLNodeHandlerTabSet.h"
#include "../../../Include/RmlUi/Core/Elements/ElementTabSet.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/XMLParser.h"

namespace Rml {

XMLNodeHandlerTabSet::XMLNodeHandlerTabSet() {}

XMLNodeHandlerTabSet::~XMLNodeHandlerTabSet() {}

Element* XMLNodeHandlerTabSet::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	RMLUI_ASSERT(name == "tabset" || name == "tabs" || name == "tab" || name == "panels" || name == "panel");

	if (name == "tabset")
	{
		// Call this node handler for all children
		parser->PushHandler("tabset");

		// Attempt to instance the tabset
		ElementPtr element = Factory::InstanceElement(parser->GetParseFrame()->element, name, name, attributes);
		ElementTabSet* tabset = rmlui_dynamic_cast<ElementTabSet*>(element.get());
		if (!tabset)
		{
			Log::Message(Log::LT_ERROR, "Instancer failed to create element for tag %s.", name.c_str());
			return nullptr;
		}

		// Add the TabSet into the document
		Element* result = parser->GetParseFrame()->element->AppendChild(std::move(element));

		return result;
	}
	else if (name == "tab")
	{
		// Call default element handler for all children.
		parser->PushDefaultHandler();

		ElementPtr tab_element = Factory::InstanceElement(parser->GetParseFrame()->element, "*", "tab", attributes);
		Element* result = nullptr;

		ElementTabSet* tabset = rmlui_dynamic_cast<ElementTabSet*>(parser->GetParseFrame()->element);
		if (tabset)
		{
			result = tab_element.get();
			tabset->SetTab(-1, std::move(tab_element));
		}

		return result;
	}
	else if (name == "panel")
	{
		// Call default element handler for all children.
		parser->PushDefaultHandler();

		ElementPtr panel_element = Factory::InstanceElement(parser->GetParseFrame()->element, "*", "panel", attributes);
		Element* result = nullptr;

		ElementTabSet* tabset = rmlui_dynamic_cast<ElementTabSet*>(parser->GetParseFrame()->element);
		if (tabset)
		{
			result = panel_element.get();
			tabset->SetPanel(-1, std::move(panel_element));
		}

		return result;
	}
	else if (name == "tabs" || name == "panels")
	{
		// Use the element handler to add the tabs and panels elements to the the tabset (this allows users to
		// style them nicely), but don't return the new element, as we still want the tabset to be the top of the
		// parser's node stack.

		Element* parent = parser->GetParseFrame()->element;

		ElementPtr element = Factory::InstanceElement(parent, name, name, attributes);
		if (!element)
		{
			Log::Message(Log::LT_ERROR, "Instancer failed to create element for tag %s.", name.c_str());
			return nullptr;
		}

		parent->AppendChild(std::move(element));

		return nullptr;
	}

	return nullptr;
}

bool XMLNodeHandlerTabSet::ElementEnd(XMLParser* /*parser*/, const String& /*name*/)
{
	return true;
}

bool XMLNodeHandlerTabSet::ElementData(XMLParser* parser, const String& data, XMLDataType /*type*/)
{
	return Factory::InstanceElementText(parser->GetParseFrame()->element, data);
}

} // namespace Rml
