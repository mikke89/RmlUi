#pragma once

#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

// getters
int ElementFormControlGetAttrdisabled(lua_State* L);
int ElementFormControlGetAttrname(lua_State* L);
int ElementFormControlGetAttrvalue(lua_State* L);

// setters
int ElementFormControlSetAttrdisabled(lua_State* L);
int ElementFormControlSetAttrname(lua_State* L);
int ElementFormControlSetAttrvalue(lua_State* L);

extern RegType<ElementFormControl> ElementFormControlMethods[];
extern luaL_Reg ElementFormControlGetters[];
extern luaL_Reg ElementFormControlSetters[];

template <>
void ExtraInit<ElementFormControl>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementFormControl)
} // namespace Lua
} // namespace Rml
