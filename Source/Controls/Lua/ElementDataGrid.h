#pragma once
/*
    This defines the ElementDataGrid type in the Lua global namespace
    
    inherits from Element

    //methods
    noreturn ElementDataGrid:AddColumn(string fields, string formatter, float initial_width, string header_rml)
    noreturn ElementDataGrid:SetDataSource(string data_source_name)

    //getter
    {}key=int,value=ElementDataGridRow ElementDataGrid.rows
    --it returns a table, so you can acess it just like any other table. 
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Controls/ElementDataGrid.h>

using Rocket::Controls::ElementDataGrid;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementDataGrid>::extra_init(lua_State* L, int metatable_index);

//methods
int ElementDataGridAddColumn(lua_State* L, ElementDataGrid* obj);
int ElementDataGridSetDataSource(lua_State* L, ElementDataGrid* obj);

//getter
int ElementDataGridGetAttrrows(lua_State* L);


RegType<ElementDataGrid> ElementDataGridMethods[];
luaL_reg ElementDataGridGetters[];
luaL_reg ElementDataGridSetters[];

template<> const char* GetTClassName<ElementDataGrid>();
template<> RegType<ElementDataGrid>* GetMethodTable<ElementDataGrid>();
template<> luaL_reg* GetAttrTable<ElementDataGrid>();
template<> luaL_reg* SetAttrTable<ElementDataGrid>();

}
}
}