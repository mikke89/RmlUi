#pragma once

#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// will inherit from Element
template <>
void ExtraInit<ElementText>(lua_State* L, int metatable_index);

int ElementTextGetAttrtext(lua_State* L);
int ElementTextSetAttrtext(lua_State* L);

extern RegType<ElementText> ElementTextMethods[];
extern luaL_Reg ElementTextGetters[];
extern luaL_Reg ElementTextSetters[];

RMLUI_LUATYPE_DECLARE(ElementText)
} // namespace Lua
} // namespace Rml
