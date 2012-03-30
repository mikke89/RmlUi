#include "precompiled.h"
#include "ElementDataGrid.h"
#include <Rocket/Core/Element.h>
#include <Rocket/Controls/ElementDataGridRow.h>

using Rocket::Controls::ElementDataGridRow;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementDataGrid>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<Element>::extra_init(L,metatable_index);
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

//methods
int ElementDataGridAddColumn(lua_State* L, ElementDataGrid* obj)
{
    LUACHECKOBJ(obj);
    const char* fields = luaL_checkstring(L,1);
    const char* formatter = luaL_checkstring(L,2);
    float width = (float)luaL_checknumber(L,3);
    const char* rml = luaL_checkstring(L,4);

    obj->AddColumn(fields,formatter,width,rml);
    return 0;
}

int ElementDataGridSetDataSource(lua_State* L, ElementDataGrid* obj)
{
    LUACHECKOBJ(obj);
    const char* source = luaL_checkstring(L,1);
    
    obj->SetDataSource(source);
    return 0;
}


//getter
int ElementDataGridGetAttrrows(lua_State* L)
{
    ElementDataGrid* obj = LuaType<ElementDataGrid>::check(L,1);
    LUACHECKOBJ(obj);

    lua_newtable(L);
    int tbl = lua_gettop(L);
    int numrows = obj->GetNumRows();
    ElementDataGridRow* row;
    for(int i = 0; i < numrows; i++)
    {
        row = obj->GetRow(i);
        LuaType<ElementDataGridRow>::push(L,row,false);
        lua_rawseti(L,tbl,i);
    }
    return 1;
}



RegType<ElementDataGrid> ElementDataGridMethods[] =
{
    LUAMETHOD(ElementDataGrid,AddColumn)
    LUAMETHOD(ElementDataGrid,SetDataSource)
    { NULL, NULL },
};

luaL_reg ElementDataGridGetters[] =
{
    LUAGETTER(ElementDataGrid,rows)
    { NULL, NULL },
};

luaL_reg ElementDataGridSetters[] =
{
    { NULL, NULL },
};


template<> const char* GetTClassName<ElementDataGrid>() { return "ElementDataGrid"; }
template<> RegType<ElementDataGrid>* GetMethodTable<ElementDataGrid>() { return ElementDataGridMethods; }
template<> luaL_reg* GetAttrTable<ElementDataGrid>() { return ElementDataGridGetters; }
template<> luaL_reg* SetAttrTable<ElementDataGrid>() { return ElementDataGridSetters; }

}
}
}