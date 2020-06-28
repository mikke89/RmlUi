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

#include "XMLNodeHandlerTextArea.h"
#include "../../Include/RmlUi/Controls/ElementFormControlTextArea.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/XMLParser.h"

namespace Rml {
namespace Controls {

XMLNodeHandlerTextArea::XMLNodeHandlerTextArea()
{
}

XMLNodeHandlerTextArea::~XMLNodeHandlerTextArea()
{
}

Core::Element* XMLNodeHandlerTextArea::ElementStart(Core::XMLParser* parser, const Rml::Core::String& name, const Rml::Core::XMLAttributes& attributes)
{
	ElementFormControlTextArea* text_area = rmlui_dynamic_cast< ElementFormControlTextArea* >(parser->GetParseFrame()->element);
	if (!text_area)
	{
		Core::ElementPtr new_element = Core::Factory::InstanceElement(parser->GetParseFrame()->element, name, name, attributes);
		if (!new_element)
			return nullptr;

		Core::Element* result = parser->GetParseFrame()->element->AppendChild(std::move(new_element));

		return result;
	}

	return nullptr;
}

bool XMLNodeHandlerTextArea::ElementEnd(Core::XMLParser* RMLUI_UNUSED_PARAMETER(parser), const Rml::Core::String& RMLUI_UNUSED_PARAMETER(name))
{
	RMLUI_UNUSED(parser);
	RMLUI_UNUSED(name);

	return true;
}

bool XMLNodeHandlerTextArea::ElementData(Core::XMLParser* parser, const Rml::Core::String& data, Core::XMLDataType RMLUI_UNUSED_PARAMETER(type))
{
	RMLUI_UNUSED(type);

	ElementFormControlTextArea* text_area = rmlui_dynamic_cast< ElementFormControlTextArea* >(parser->GetParseFrame()->element);
	if (text_area != nullptr)
	{
		// Do any necessary translation.
		Rml::Core::String translated_data;
		Core::GetSystemInterface()->TranslateString(translated_data, data);

		text_area->SetValue(translated_data);
	}

	return true;
}

}
}
