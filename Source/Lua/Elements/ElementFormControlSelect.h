#pragma once

#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

// methods
int ElementFormControlSelectAdd(lua_State* L, ElementFormControlSelect* obj);
int ElementFormControlSelectRemove(lua_State* L, ElementFormControlSelect* obj);

// getters
int ElementFormControlSelectGetAttroptions(lua_State* L);
int ElementFormControlSelectGetAttrselection(lua_State* L);

// setter
int ElementFormControlSelectSetAttrselection(lua_State* L);

extern RegType<ElementFormControlSelect> ElementFormControlSelectMethods[];
extern luaL_Reg ElementFormControlSelectGetters[];
extern luaL_Reg ElementFormControlSelectSetters[];

// inherits from ElementFormControl which inherits from Element
template <>
void ExtraInit<ElementFormControlSelect>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementFormControlSelect)
} // namespace Lua
} // namespace Rml
