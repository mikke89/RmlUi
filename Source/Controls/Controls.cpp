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

#include "../../Include/RmlUi/Controls/Controls.h"
#include "../../Include/RmlUi/Core/ElementInstancerGeneric.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/Core/Plugin.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "ElementTextSelection.h"
#include "XMLNodeHandlerDataGrid.h"
#include "XMLNodeHandlerTabSet.h"
#include "XMLNodeHandlerTextArea.h"
#include "../../Include/RmlUi/Controls/ElementFormControlInput.h"

namespace Rml {
namespace Controls {

// Registers the custom element instancers.
void RegisterElementInstancers()
{
	Core::ElementInstancer* instancer = new Core::ElementInstancerGeneric< ElementForm >();
	Core::Factory::RegisterElementInstancer("form", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementFormControlInput >();
	Core::Factory::RegisterElementInstancer("input", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementFormControlDataSelect >();
	instancer = Core::Factory::RegisterElementInstancer("dataselect", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementFormControlSelect >();
	Core::Factory::RegisterElementInstancer("select", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementFormControlTextArea >();
	Core::Factory::RegisterElementInstancer("textarea", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementTextSelection >();
	Core::Factory::RegisterElementInstancer("#selection", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementTabSet >();
	Core::Factory::RegisterElementInstancer("tabset", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementDataGrid >();
	Core::Factory::RegisterElementInstancer("datagrid", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementDataGridExpandButton >();
	Core::Factory::RegisterElementInstancer("datagridexpand", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementDataGridCell >();
	Core::Factory::RegisterElementInstancer("#rmlctl_datagridcell", instancer);
	instancer->RemoveReference();

	instancer = new Core::ElementInstancerGeneric< ElementDataGridRow >();
	Core::Factory::RegisterElementInstancer("#rmlctl_datagridrow", instancer);
	instancer->RemoveReference();
}

void RegisterXMLNodeHandlers()
{
	Core::XMLNodeHandler* node_handler = new XMLNodeHandlerDataGrid();
	Core::XMLParser::RegisterNodeHandler("datagrid", node_handler);
	node_handler->RemoveReference();

	node_handler = new XMLNodeHandlerTabSet();
	Core::XMLParser::RegisterNodeHandler("tabset", node_handler);
	node_handler->RemoveReference();

	node_handler = new XMLNodeHandlerTextArea();
	Core::XMLParser::RegisterNodeHandler("textarea", node_handler);
	node_handler->RemoveReference();
}

static bool initialised = false;

class ControlsPlugin : public Rml::Core::Plugin
{
public:
	void OnShutdown()
	{
		initialised = false;
		delete this;
	}

	int GetEventClasses()
	{
		return Rml::Core::Plugin::EVT_BASIC;
	}
};

void Initialise()
{
	// Prevent double initialisation
	if (!initialised)
	{
		Core::StyleSheetSpecification::RegisterProperty("min-rows", "0", false, false).AddParser("number");

		// Register the element instancers for our custom elements.
		RegisterElementInstancers();

		// Register the XML node handlers for our elements that require special parsing.
		RegisterXMLNodeHandlers();

		// Register the controls plugin, so we'll be notified on Shutdown
		Rml::Core::RegisterPlugin(new ControlsPlugin());

		initialised = true;
	}
}

}
}
