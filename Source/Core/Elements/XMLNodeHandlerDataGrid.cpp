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

#include "XMLNodeHandlerDataGrid.h"
#include "../../../Include/RmlUi/Core/StreamMemory.h"
#include "../../../Include/RmlUi/Core/Log.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/XMLParser.h"
#include "../../../Include/RmlUi/Core/Elements/ElementDataGrid.h"

namespace Rml {

XMLNodeHandlerDataGrid::XMLNodeHandlerDataGrid()
{
}

XMLNodeHandlerDataGrid::~XMLNodeHandlerDataGrid()
{
}

Element* XMLNodeHandlerDataGrid::ElementStart(XMLParser* parser, const String& name, const XMLAttributes& attributes)
{
	Element* result = nullptr;
	Element* parent = parser->GetParseFrame()->element;

	RMLUI_ASSERT(name == "datagrid" || name == "col");

	if (name == "datagrid")
	{
		// Attempt to instance the grid.
		ElementPtr element = Factory::InstanceElement(parent, name, name, attributes);
		ElementDataGrid* grid = rmlui_dynamic_cast< ElementDataGrid* >(element.get());
		if (!grid)
		{
			element.reset();
			Log::Message(Log::LT_ERROR, "Instancer failed to create data grid for tag %s.", name.c_str());
			return nullptr;
		}

		// Set the data source and table on the data grid.
		String data_source = Get<String>(attributes, "source", "");
		grid->SetDataSource(data_source);

		result = parent->AppendChild(std::move(element));

		// Switch to this handler for all columns.
		parser->PushHandler("datagrid");
	}
	else if (name == "col")
	{
		// Make a new node handler to handle the header elements.		
		ElementPtr element = Factory::InstanceElement(parent, "datagridcolumn", "datagridcolumn", attributes);
		if (!element)
			return nullptr;

		result = element.get();

		ElementDataGrid* grid = rmlui_dynamic_cast< ElementDataGrid* >(parent);
		if (grid)
			grid->AddColumn(Get<String>(attributes, "fields", ""), Get<String>(attributes, "formatter", ""), Get(attributes, "width", 0.0f), std::move(element));

		// Switch to element handler for all children.
		parser->PushDefaultHandler();
	}
	else
	{
		RMLUI_ERROR;
	}

	return result;
}

bool XMLNodeHandlerDataGrid::ElementEnd(XMLParser* RMLUI_UNUSED_PARAMETER(parser), const String& RMLUI_UNUSED_PARAMETER(name))
{
	RMLUI_UNUSED(parser);
	RMLUI_UNUSED(name);

	return true;
}

bool XMLNodeHandlerDataGrid::ElementData(XMLParser* parser, const String& data, XMLDataType RMLUI_UNUSED_PARAMETER(type))
{
	RMLUI_UNUSED(type);
	Element* parent = parser->GetParseFrame()->element;

	// Parse the text into the parent element.
	return Factory::InstanceElementText(parent, data);
}

} // namespace Rml
