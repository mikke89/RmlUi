#pragma once

#include <RmlUi/Core/Elements/ElementTabSet.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

// methods
int ElementTabSetSetPanel(lua_State* L, ElementTabSet* obj);
int ElementTabSetSetTab(lua_State* L, ElementTabSet* obj);

// getters
int ElementTabSetGetAttractive_tab(lua_State* L);
int ElementTabSetGetAttrnum_tabs(lua_State* L);

// setter
int ElementTabSetSetAttractive_tab(lua_State* L);

extern RegType<ElementTabSet> ElementTabSetMethods[];
extern luaL_Reg ElementTabSetGetters[];
extern luaL_Reg ElementTabSetSetters[];

// this will be used to "inherit" from Element
template <>
void ExtraInit<ElementTabSet>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementTabSet)
} // namespace Lua
} // namespace Rml
