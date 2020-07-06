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
 
#include "LuaPlugin.h"
#include <RmlUi/Lua/Lua.h>
#include <RmlUi/Lua/Utilities.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Lua/LuaType.h>
#include "LuaDocumentElementInstancer.h"
#include <RmlUi/Core/Factory.h>
#include "LuaEventListenerInstancer.h"
#include "RmlUi.h"
//the types I made
#include "ContextDocumentsProxy.h"
#include "EventParametersProxy.h"
#include "ElementAttributesProxy.h"
#include "Log.h"
#include "Element.h"
#include "ElementStyleProxy.h"
#include "Document.h"
#include "Colourb.h"
#include "Colourf.h"
#include "Vector2f.h"
#include "Vector2i.h"
#include "Context.h"
#include "Event.h"
#include "ElementInstancer.h"
#include "ElementChildNodesProxy.h"
#include "ElementText.h"
#include "GlobalLuaFunctions.h"
#include "RmlUiContextsProxy.h"
// Control types
#include "Elements/SelectOptionsProxy.h"
#include "Elements/DataFormatter.h"
#include "Elements/DataSource.h"
#include "Elements/ElementForm.h"
#include "Elements/ElementFormControl.h"
#include "Elements/ElementFormControlSelect.h"
#include "Elements/ElementFormControlDataSelect.h"
#include "Elements/ElementFormControlInput.h"
#include "Elements/ElementFormControlTextArea.h"
#include "Elements/ElementDataGrid.h"
#include "Elements/ElementDataGridRow.h"
#include "Elements/ElementTabSet.h"

namespace Rml {
namespace Lua {

static lua_State* g_L = nullptr;


/** This will populate the global Lua table with all of the Lua core types by calling LuaType<T>::Register
@remark This is called automatically by LuaPlugin::OnInitialise(). */
static void RegisterTypes();


LuaPlugin::LuaPlugin(lua_State* lua_state)
{
	RMLUI_ASSERT(g_L == nullptr);
	g_L = lua_state;
}

int LuaPlugin::GetEventClasses()
{
    return EVT_BASIC;
}

void LuaPlugin::OnInitialise()
{
	if (g_L == nullptr)
	{
		Log::Message(Log::LT_INFO, "Loading Lua plugin using a new Lua state.");
		g_L = luaL_newstate();
		luaL_openlibs(g_L);
		owns_lua_state = true;
	}
	else
	{
		Log::Message(Log::LT_INFO, "Loading Lua plugin using the provided Lua state.");
		owns_lua_state = false;
	}
	RegisterTypes();

	lua_document_element_instancer = new LuaDocumentElementInstancer();
	lua_event_listener_instancer = new LuaEventListenerInstancer();
    Factory::RegisterElementInstancer("body", lua_document_element_instancer);
	Factory::RegisterEventListenerInstancer(lua_event_listener_instancer);
}

void LuaPlugin::OnShutdown()
{
	delete lua_document_element_instancer;
	delete lua_event_listener_instancer;
	lua_document_element_instancer = nullptr;
	lua_event_listener_instancer = nullptr;

	if (owns_lua_state)
		lua_close(g_L);
	
	g_L = nullptr;

	delete this;
}


static void RegisterTypes()
{
    RMLUI_ASSERT(g_L);
    lua_State* L = g_L;

    LuaType<Vector2i>::Register(L);
    LuaType<Vector2f>::Register(L);
    LuaType<Colourf>::Register(L);
    LuaType<Colourb>::Register(L);
    LuaType<Log>::Register(L);
    LuaType<ElementStyleProxy>::Register(L);
    LuaType<Element>::Register(L);
    //things that inherit from Element
    LuaType<Document>::Register(L);
    LuaType<ElementText>::Register(L);
    LuaType<ElementPtr>::Register(L);
    LuaType<Event>::Register(L);
    LuaType<Context>::Register(L);
    LuaType<LuaRmlUi>::Register(L);
    LuaType<ElementInstancer>::Register(L);
    //Proxy tables
    LuaType<ContextDocumentsProxy>::Register(L);
    LuaType<EventParametersProxy>::Register(L);
    LuaType<ElementAttributesProxy>::Register(L);
    LuaType<ElementChildNodesProxy>::Register(L);
    LuaType<RmlUiContextsProxy>::Register(L);
    OverrideLuaGlobalFunctions(L);
    //push the global variable "rmlui" to use the "RmlUi" methods
    LuaRmlUiPushrmluiGlobal(L);

    // Control types
    LuaType<ElementForm>::Register(L);
    LuaType<ElementFormControl>::Register(L);
    //Inherits from ElementFormControl
    LuaType<ElementFormControlSelect>::Register(L);
    LuaType<ElementFormControlDataSelect>::Register(L);
    LuaType<ElementFormControlInput>::Register(L);
    LuaType<ElementFormControlTextArea>::Register(L);
    LuaType<DataFormatter>::Register(L);
    LuaType<DataSource>::Register(L);
    LuaType<ElementDataGrid>::Register(L);
    LuaType<ElementDataGridRow>::Register(L);
    LuaType<ElementTabSet>::Register(L);
    //proxy tables
    LuaType<SelectOptionsProxy>::Register(L);
}


lua_State* LuaPlugin::GetLuaState()
{
    return g_L;
}

} // namespace Lua
} // namespace Rml
