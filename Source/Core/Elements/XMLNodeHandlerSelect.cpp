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

#include "XMLNodeHandlerSelect.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlSelect.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/XMLParser.h"

namespace Rml {

XMLNodeHandlerSelect::XMLNodeHandlerSelect() {}

XMLNodeHandlerSelect::~XMLNodeHandlerSelect() {}

Element* XMLNodeHandlerSelect::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	RMLUI_ASSERT(name == "select" || name == "option");

	if (name == "select")
	{
		// Call this node handler for all children
		parser->PushHandler("select");

		// Attempt to instance the tabset
		ElementPtr element = Factory::InstanceElement(parser->GetParseFrame()->element, name, name, attributes);
		ElementFormControlSelect* select_element = rmlui_dynamic_cast<ElementFormControlSelect*>(element.get());
		if (!select_element)
		{
			Log::Message(Log::LT_ERROR, "Instancer failed to create element for tag %s.", name.c_str());
			return nullptr;
		}

		// Add the Select element into the document
		Element* result = parser->GetParseFrame()->element->AppendChild(std::move(element));

		return result;
	}
	else if (name == "option")
	{
		// Call default element handler for all children.
		parser->PushDefaultHandler();

		ElementPtr option_element = Factory::InstanceElement(parser->GetParseFrame()->element, name, name, attributes);
		Element* result = nullptr;

		ElementFormControlSelect* select_element = rmlui_dynamic_cast<ElementFormControlSelect*>(parser->GetParseFrame()->element);
		if (select_element)
		{
			result = option_element.get();
			select_element->Add(std::move(option_element));
		}

		return result;
	}

	return nullptr;
}

} // namespace Rml
