#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// where owner is the Element that we should look up information from
struct ElementChildNodesProxy {
	Element* owner;
};

template <>
void ExtraInit<ElementChildNodesProxy>(lua_State* L, int metatable_index);
int ElementChildNodesProxy__index(lua_State* L);
int ElementChildNodesProxy__pairs(lua_State* L);
int ElementChildNodesProxy__len(lua_State* L);

extern RegType<ElementChildNodesProxy> ElementChildNodesProxyMethods[];
extern luaL_Reg ElementChildNodesProxyGetters[];
extern luaL_Reg ElementChildNodesProxySetters[];

RMLUI_LUATYPE_DECLARE(ElementChildNodesProxy)
} // namespace Lua
} // namespace Rml
