#include "precompiled.h"
#include "ElementDataGridRow.h"
#include <Rocket/Controls/ElementDataGrid.h>

using Rocket::Controls::ElementDataGrid;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementDataGridRow>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<Element>::extra_init(L,metatable_index);
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

//getters
int ElementDataGridRowGetAttrrow_expanded(lua_State* L)
{
    ElementDataGridRow* obj = LuaType<ElementDataGridRow>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushboolean(L,obj->IsRowExpanded());
    return 1;
}

int ElementDataGridRowGetAttrparent_relative_index(lua_State* L)
{
    ElementDataGridRow* obj = LuaType<ElementDataGridRow>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetParentRelativeIndex());
    return 1;
}

int ElementDataGridRowGetAttrtable_relative_index(lua_State* L)
{
    ElementDataGridRow* obj = LuaType<ElementDataGridRow>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetTableRelativeIndex());
    return 1;
}

int ElementDataGridRowGetAttrparent_row(lua_State* L)
{
    ElementDataGridRow* obj = LuaType<ElementDataGridRow>::check(L,1);
    LUACHECKOBJ(obj);
    LuaType<ElementDataGridRow>::push(L,obj->GetParentRow(),false);
    return 1;
}

int ElementDataGridRowGetAttrparent_grid(lua_State* L)
{
    ElementDataGridRow* obj = LuaType<ElementDataGridRow>::check(L,1);
    LUACHECKOBJ(obj);
    LuaType<ElementDataGrid>::push(L,obj->GetParentGrid(),false);
    return 1;
}


//setter
int ElementDataGridRowSetAttrrow_expanded(lua_State* L)
{
    ElementDataGridRow* obj = LuaType<ElementDataGridRow>::check(L,1);
    LUACHECKOBJ(obj);
    bool expanded = CHECK_BOOL(L,2);
    if(expanded)
        obj->ExpandRow();
    else
        obj->CollapseRow();
    return 0;
}



RegType<ElementDataGridRow> ElementDataGridRowMethods[] =
{
    { NULL, NULL },
};

luaL_reg ElementDataGridRowGetters[] =
{
    LUAGETTER(ElementDataGridRow,row_expanded)
    LUAGETTER(ElementDataGridRow,parent_relative_index)
    LUAGETTER(ElementDataGridRow,table_relative_index)
    LUAGETTER(ElementDataGridRow,parent_row)
    LUAGETTER(ElementDataGridRow,parent_grid)
    { NULL, NULL },
};

luaL_reg ElementDataGridRowSetters[] =
{
    LUASETTER(ElementDataGridRow,row_expanded)
    { NULL, NULL },
};

template<> const char* GetTClassName<ElementDataGridRow>() { return "ElementDataGridRow"; }
template<> RegType<ElementDataGridRow>* GetMethodTable<ElementDataGridRow>() { return ElementDataGridRowMethods; }
template<> luaL_reg* GetAttrTable<ElementDataGridRow>() { return ElementDataGridRowGetters; }
template<> luaL_reg* SetAttrTable<ElementDataGridRow>() { return ElementDataGridRowSetters; }

}
}
}