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
#include "../../Include/RmlUi/Core/ElementInstancer.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/XMLParser.h"
#include "../../Include/RmlUi/Core/Plugin.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "ElementTextSelection.h"
#include "XMLNodeHandlerDataGrid.h"
#include "XMLNodeHandlerTabSet.h"
#include "XMLNodeHandlerTextArea.h"
#include "../../Include/RmlUi/Controls/ElementForm.h"
#include "../../Include/RmlUi/Controls/ElementFormControlInput.h"
#include "../../Include/RmlUi/Controls/ElementFormControlDataSelect.h"
#include "../../Include/RmlUi/Controls/ElementFormControlSelect.h"
#include "../../Include/RmlUi/Controls/ElementFormControlSelect.h"
#include "../../Include/RmlUi/Controls/ElementFormControlTextArea.h"
#include "../../Include/RmlUi/Controls/ElementTabSet.h"
#include "../../Include/RmlUi/Controls/ElementProgressBar.h"
#include "../../Include/RmlUi/Controls/ElementDataGrid.h"
#include "../../Include/RmlUi/Controls/ElementDataGridExpandButton.h"
#include "../../Include/RmlUi/Controls/ElementDataGridCell.h"
#include "../../Include/RmlUi/Controls/ElementDataGridRow.h"

namespace Rml {
namespace Controls {

struct ElementInstancers {
	using Ptr = std::unique_ptr<Core::ElementInstancer>;
	template<typename T> using ElementInstancerGeneric = Core::ElementInstancerGeneric<T>;

	Ptr form = std::make_unique<ElementInstancerGeneric<ElementForm>>();
	Ptr input = std::make_unique<ElementInstancerGeneric<ElementFormControlInput>>();
	Ptr dataselect = std::make_unique<ElementInstancerGeneric<ElementFormControlDataSelect>>();
	Ptr select = std::make_unique<ElementInstancerGeneric<ElementFormControlSelect>>();
	
	Ptr textarea = std::make_unique<ElementInstancerGeneric<ElementFormControlTextArea>>();
	Ptr selection = std::make_unique<ElementInstancerGeneric<ElementTextSelection>>();
	Ptr tabset  = std::make_unique<ElementInstancerGeneric<ElementTabSet>>();

	Ptr progressbar  = std::make_unique<ElementInstancerGeneric<ElementProgressBar>>();
	
	Ptr datagrid = std::make_unique<ElementInstancerGeneric<ElementDataGrid>>();
	Ptr datagrid_expand = std::make_unique<ElementInstancerGeneric<ElementDataGridExpandButton>>();
	Ptr datagrid_cell = std::make_unique<ElementInstancerGeneric<ElementDataGridCell>>();
	Ptr datagrid_row = std::make_unique<ElementInstancerGeneric<ElementDataGridRow>>();
};

static std::unique_ptr<ElementInstancers> element_instancers;


// Registers the custom element instancers.
void RegisterElementInstancers()
{
	element_instancers = std::make_unique<ElementInstancers>();

	Core::Factory::RegisterElementInstancer("form", element_instancers->form.get());
	Core::Factory::RegisterElementInstancer("input", element_instancers->input.get());
	Core::Factory::RegisterElementInstancer("dataselect", element_instancers->dataselect.get());
	Core::Factory::RegisterElementInstancer("select", element_instancers->select.get());

	Core::Factory::RegisterElementInstancer("textarea", element_instancers->textarea.get());
	Core::Factory::RegisterElementInstancer("#selection", element_instancers->selection.get());
	Core::Factory::RegisterElementInstancer("tabset", element_instancers->tabset.get());

	Core::Factory::RegisterElementInstancer("progressbar", element_instancers->progressbar.get());

	Core::Factory::RegisterElementInstancer("datagrid", element_instancers->datagrid.get());
	Core::Factory::RegisterElementInstancer("datagridexpand", element_instancers->datagrid_expand.get());
	Core::Factory::RegisterElementInstancer("#rmlctl_datagridcell", element_instancers->datagrid_cell.get());
	Core::Factory::RegisterElementInstancer("#rmlctl_datagridrow", element_instancers->datagrid_row.get());
}

void RegisterXMLNodeHandlers()
{
	Core::XMLParser::RegisterNodeHandler("datagrid", std::make_shared<XMLNodeHandlerDataGrid>());
	Core::XMLParser::RegisterNodeHandler("tabset", std::make_shared<XMLNodeHandlerTabSet>());
	Core::XMLParser::RegisterNodeHandler("textarea", std::make_shared<XMLNodeHandlerTextArea>());
}

static bool initialised = false;

class ControlsPlugin : public Rml::Core::Plugin
{
public:
	void OnShutdown() override
	{
		element_instancers.reset();
		initialised = false;
		delete this;
	}

	int GetEventClasses() override
	{
		return Rml::Core::Plugin::EVT_BASIC;
	}
};

void Initialise()
{
	// Prevent double initialisation
	if (!initialised)
	{
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
