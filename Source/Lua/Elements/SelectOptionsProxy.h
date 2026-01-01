#pragma once

#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// where owner is the ElementFormControlSelect that we should look up information from
struct SelectOptionsProxy {
	ElementFormControlSelect* owner;
};

int SelectOptionsProxy__index(lua_State* L);
int SelectOptionsProxy__pairs(lua_State* L);

extern RegType<SelectOptionsProxy> SelectOptionsProxyMethods[];
extern luaL_Reg SelectOptionsProxyGetters[];
extern luaL_Reg SelectOptionsProxySetters[];

template <>
void ExtraInit<SelectOptionsProxy>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(SelectOptionsProxy)
} // namespace Lua
} // namespace Rml
