#pragma once

#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Event>(lua_State* L, int metatable_index);
// methods
int EventStopPropagation(lua_State* L, Event* obj);
int EventStopImmediatePropagation(lua_State* L, Event* obj);

// getters
int EventGetAttrcurrent_element(lua_State* L);
int EventGetAttrtype(lua_State* L);
int EventGetAttrtarget_element(lua_State* L);
int EventGetAttrparameters(lua_State* L);

extern RegType<Event> EventMethods[];
extern luaL_Reg EventGetters[];
extern luaL_Reg EventSetters[];

RMLUI_LUATYPE_DECLARE(Event)
} // namespace Lua
} // namespace Rml
