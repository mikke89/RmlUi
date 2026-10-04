#include "EventParametersProxy.h"
#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Lua/Element.h>
#include <RmlUi/Lua/Event.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Event>(lua_State* /*L*/, int /*metatable_index*/)
{
	return;
}

// methods
int EventStopPropagation(lua_State* /*L*/, Event* obj)
{
	obj->StopPropagation();
	return 0;
}

int EventStopImmediatePropagation(lua_State* /*L*/, Event* obj)
{
	obj->StopImmediatePropagation();
	return 0;
}

// getters
int EventGetAttrcurrent_element(lua_State* L)
{
	Event* evt = LuaType<Event>::check(L, 1);
	RMLUI_CHECK_OBJ(evt);
	Element* ele = evt->GetCurrentElement();
	LuaType<Element>::push(L, ele);
	return 1;
}

int EventGetAttrtype(lua_State* L)
{
	Event* evt = LuaType<Event>::check(L, 1);
	RMLUI_CHECK_OBJ(evt);
	String type = evt->GetType();
	lua_pushstring(L, type.c_str());
	return 1;
}

int EventGetAttrtarget_element(lua_State* L)
{
	Event* evt = LuaType<Event>::check(L, 1);
	RMLUI_CHECK_OBJ(evt);
	Element* target = evt->GetTargetElement();
	LuaType<Element>::push(L, target);
	return 1;
}

int EventGetAttrparameters(lua_State* L)
{
	Event* evt = LuaType<Event>::check(L, 1);
	RMLUI_CHECK_OBJ(evt);
	LuaType<EventParametersProxy>::emplace(L, evt);
	return 1;
}

RegType<Event> EventMethods[] = {
	RMLUI_LUAMETHOD(Event, StopPropagation),
	RMLUI_LUAMETHOD(Event, StopImmediatePropagation),
	{nullptr, nullptr},
};

luaL_Reg EventGetters[] = {
	RMLUI_LUAGETTER(Event, current_element),
	RMLUI_LUAGETTER(Event, type),
	RMLUI_LUAGETTER(Event, target_element),
	RMLUI_LUAGETTER(Event, parameters),
	{nullptr, nullptr},
};

luaL_Reg EventSetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(Event)
} // namespace Lua
} // namespace Rml
