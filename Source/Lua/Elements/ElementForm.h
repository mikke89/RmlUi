#pragma once

#include <RmlUi/Core/Elements/ElementForm.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

// method
int ElementFormSubmit(lua_State* L, ElementForm* obj);

extern RegType<ElementForm> ElementFormMethods[];
extern luaL_Reg ElementFormGetters[];
extern luaL_Reg ElementFormSetters[];

// this will be used to "inherit" from Element
template <>
void ExtraInit<ElementForm>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementForm)
} // namespace Lua
} // namespace Rml
