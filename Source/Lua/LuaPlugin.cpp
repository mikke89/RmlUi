#include "LuaPlugin.h"
#include "LuaDocumentElementInstancer.h"
#include "LuaEventListenerInstancer.h"
#include "RmlUi.h"
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Lua/Lua.h>
#include <RmlUi/Lua/Utilities.h>
// the types I made
#include <RmlUi/Lua/Colourb.h>
#include <RmlUi/Lua/Colourf.h>
#include <RmlUi/Lua/Context.h>
#include "ContextDocumentsProxy.h"
#include <RmlUi/Lua/Document.h>
#include <RmlUi/Lua/Element.h>
#include "ElementAttributesProxy.h"
#include "ElementChildNodesProxy.h"
#include "ElementInstancer.h"
#include "ElementStyleProxy.h"
#include <RmlUi/Lua/ElementText.h>
#include <RmlUi/Lua/Event.h>
#include "EventParametersProxy.h"
#include "GlobalLuaFunctions.h"
#include "Log.h"
#include "RmlUiContextsProxy.h"
#include <RmlUi/Lua/Vector2f.h>
#include <RmlUi/Lua/Vector2i.h>
// Control types
#include <RmlUi/Lua/Elements/ElementForm.h>
#include <RmlUi/Lua/Elements/ElementFormControl.h>
#include <RmlUi/Lua/Elements/ElementFormControlInput.h>
#include <RmlUi/Lua/Elements/ElementFormControlSelect.h>
#include <RmlUi/Lua/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Lua/Elements/ElementTabSet.h>
#include "Elements/SelectOptionsProxy.h"

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

	// Create unique ref storage table as a weak table in the registry
	lua_newtable(L); // [1] = {}

	lua_newtable(L); // [2] = {}
	lua_pushstring(L, "v");
	lua_setfield(L, -2, "__mode"); // [2].__mode = "v"
	lua_setmetatable(L, -2); // setmetatable([1], [2])

	lua_setfield(L, LUA_REGISTRYINDEX, UNIQUE_REF_TABLE_NAME); // Place table in registry

	LuaType<Vector2i>::Register(L);
	LuaType<Vector2f>::Register(L);
	LuaType<Colourf>::Register(L);
	LuaType<Colourb>::Register(L);
	LuaType<Log>::Register(L);
	LuaType<ElementStyleProxy>::Register(L);
	LuaType<Element>::Register(L);
	// things that inherit from Element
	LuaType<Document>::Register(L);
	LuaType<ElementText>::Register(L);
	LuaType<Event>::Register(L);
	LuaType<Context>::Register(L);
	LuaType<LuaRmlUi>::Register(L);
	LuaType<LuaElementInstancer>::Register(L);
	// Proxy tables
	LuaType<ContextDocumentsProxy>::Register(L);
	LuaType<EventParametersProxy>::Register(L);
	LuaType<ElementAttributesProxy>::Register(L);
	LuaType<ElementChildNodesProxy>::Register(L);
	LuaType<RmlUiContextsProxy>::Register(L);
	OverrideLuaGlobalFunctions(L);
	// push the global variable "rmlui" to use the "RmlUi" methods
	LuaRmlUiPushrmluiGlobal(L);

	// Control types
	LuaType<ElementForm>::Register(L);
	LuaType<ElementFormControl>::Register(L);
	// Inherits from ElementFormControl
	LuaType<ElementFormControlSelect>::Register(L);
	LuaType<ElementFormControlInput>::Register(L);
	LuaType<ElementFormControlTextArea>::Register(L);
	LuaType<ElementTabSet>::Register(L);
	// proxy tables
	LuaType<SelectOptionsProxy>::Register(L);
}

lua_State* LuaPlugin::GetLuaState()
{
	return g_L;
}

} // namespace Lua
} // namespace Rml
