#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// where owner is the Element that we should look up information from
struct ElementAttributesProxy {
	Element* owner;
};

template <>
void ExtraInit<ElementAttributesProxy>(lua_State* L, int metatable_index);
int ElementAttributesProxy__index(lua_State* L);
int ElementAttributesProxy__pairs(lua_State* L);

extern RegType<ElementAttributesProxy> ElementAttributesProxyMethods[];
extern luaL_Reg ElementAttributesProxyGetters[];
extern luaL_Reg ElementAttributesProxySetters[];

RMLUI_LUATYPE_DECLARE(ElementAttributesProxy)
} // namespace Lua
} // namespace Rml
