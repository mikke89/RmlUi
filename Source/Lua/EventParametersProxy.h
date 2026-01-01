#pragma once

#include <RmlUi/Core/Event.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// where owner is the Element that we should look up information from
struct EventParametersProxy {
	Event* owner;
};

template <>
void ExtraInit<EventParametersProxy>(lua_State* L, int metatable_index);
int EventParametersProxy__index(lua_State* L);
int EventParametersProxy__pairs(lua_State* L);

extern RegType<EventParametersProxy> EventParametersProxyMethods[];
extern luaL_Reg EventParametersProxyGetters[];
extern luaL_Reg EventParametersProxySetters[];

RMLUI_LUATYPE_DECLARE(EventParametersProxy)
} // namespace Lua
} // namespace Rml
