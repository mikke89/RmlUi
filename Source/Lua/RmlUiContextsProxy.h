#pragma once

#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// where owner is the Element that we should look up information from
struct RmlUiContextsProxy {
	void* nothing;
};

template <>
void ExtraInit<RmlUiContextsProxy>(lua_State* L, int metatable_index);
int RmlUiContextsProxy__index(lua_State* L);
int RmlUiContextsProxy__pairs(lua_State* L);

extern RegType<RmlUiContextsProxy> RmlUiContextsProxyMethods[];
extern luaL_Reg RmlUiContextsProxyGetters[];
extern luaL_Reg RmlUiContextsProxySetters[];

RMLUI_LUATYPE_DECLARE(RmlUiContextsProxy)
} // namespace Lua
} // namespace Rml
