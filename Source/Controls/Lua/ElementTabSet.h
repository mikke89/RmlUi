#pragma once
/*
    This defines the ElementTabSet type in the Lua global namespace

    It inherits from Element

    //methods:
    noreturn ElementTabSet:SetPanel(int index, string rml)
    noreturn ElementTabSet:SetTab(int index, string rml)

    //getters
    int ElementTabSet.active_tab
    int ElementTabSet.num_tabs

    //setter
    ElementTabSet.active_tab = int
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Controls/ElementTabSet.h>

using Rocket::Controls::ElementTabSet;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementTabSet>::extra_init(lua_State* L, int metatable_index);
template<> bool LuaType<ElementTabSet>::is_reference_counted();

//methods
int ElementTabSetSetPanel(lua_State* L, ElementTabSet* obj);
int ElementTabSetSetTab(lua_State* L, ElementTabSet* obj);

//getters
int ElementTabSetGetAttractive_tab(lua_State* L);
int ElementTabSetGetAttrnum_tabs(lua_State* L);

//setter
int ElementTabSetSetAttractive_tab(lua_State* L);

RegType<ElementTabSet> ElementTabSetMethods[];
luaL_reg ElementTabSetGetters[];
luaL_reg ElementTabSetSetters[];

/*
template<> const char* GetTClassName<ElementTabSet>();
template<> RegType<ElementTabSet>* GetMethodTable<ElementTabSet>();
template<> luaL_reg* GetAttrTable<ElementTabSet>();
template<> luaL_reg* SetAttrTable<ElementTabSet>();
*/
}
}
}