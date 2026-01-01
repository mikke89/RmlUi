#pragma once

#include "LuaElementInstancer.h"
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<ElementInstancer>(lua_State* L, int metatable_index);
// method
int ElementInstancernew(lua_State* L);
// setter
int ElementInstancerSetAttrInstanceElement(lua_State* L);

extern RegType<ElementInstancer> ElementInstancerMethods[];
extern luaL_Reg ElementInstancerGetters[];
extern luaL_Reg ElementInstancerSetters[];

RMLUI_LUATYPE_DECLARE(ElementInstancer)
} // namespace Lua
} // namespace Rml
