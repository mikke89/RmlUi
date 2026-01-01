#pragma once

#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
class Element;

namespace Lua {
struct ElementStyleProxy {
	Element* owner;
};

template <>
void ExtraInit<ElementStyleProxy>(lua_State* L, int metatable_index);
int ElementStyleProxy__index(lua_State* L);
int ElementStyleProxy__newindex(lua_State* L);
int ElementStyleProxy__pairs(lua_State* L);

extern RegType<ElementStyleProxy> ElementStyleProxyMethods[];
extern luaL_Reg ElementStyleProxyGetters[];
extern luaL_Reg ElementStyleProxySetters[];

RMLUI_LUATYPE_DECLARE(ElementStyleProxy)
} // namespace Lua
} // namespace Rml
