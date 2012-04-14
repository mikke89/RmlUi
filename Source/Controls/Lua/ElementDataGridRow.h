#pragma once
/*
    This defines the ElementDataGridRow type in the Lua global namespace

    inherits from Element

    no methods

    //getters
    bool ElementDataGridRow.row_expanded
    int ElementDataGridRow.parent_relative_index
    int ElementDataGridRow.table_relative_index
    ElementDataGridRow ElementDataGridRow.parent_row
    ElementDataGrid ElementDataGridRow.parent_grid

    //setter
    ElementDataGridRow.row_expanded = bool
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Controls/ElementDataGridRow.h>

using Rocket::Controls::ElementDataGridRow;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementDataGridRow>::extra_init(lua_State* L, int metatable_index);
template<> bool LuaType<ElementDataGridRow>::is_reference_counted();

//getters
int ElementDataGridRowGetAttrrow_expanded(lua_State* L);
int ElementDataGridRowGetAttrparent_relative_index(lua_State* L);
int ElementDataGridRowGetAttrtable_relative_index(lua_State* L);
int ElementDataGridRowGetAttrparent_row(lua_State* L);
int ElementDataGridRowGetAttrparent_grid(lua_State* L);

//setter
int ElementDataGridRowSetAttrrow_expanded(lua_State* L);


RegType<ElementDataGridRow> ElementDataGridRowMethods[];
luaL_reg ElementDataGridRowGetters[];
luaL_reg ElementDataGridRowSetters[];

/*
template<> const char* GetTClassName<ElementDataGridRow>();
template<> RegType<ElementDataGridRow>* GetMethodTable<ElementDataGridRow>();
template<> luaL_reg* GetAttrTable<ElementDataGridRow>();
template<> luaL_reg* SetAttrTable<ElementDataGridRow>();
*/

}
}
}