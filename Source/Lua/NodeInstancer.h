#pragma once

#include "LuaNodeInstancer.h"
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<NodeInstancer>(lua_State* L, int metatable_index);
// method
int NodeInstancernew(lua_State* L);
// setter
int NodeInstancerSetAttrInstanceNode(lua_State* L);

extern RegType<NodeInstancer> NodeInstancerMethods[];
extern luaL_Reg NodeInstancerGetters[];
extern luaL_Reg NodeInstancerSetters[];

RMLUI_LUATYPE_DECLARE(NodeInstancer)
} // namespace Lua
} // namespace Rml
