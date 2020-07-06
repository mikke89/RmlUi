/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
 
#include "ElementDataGrid.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementDataGridRow.h>
#include <RmlUi/Lua/Utilities.h>


namespace Rml {
namespace Lua {


//methods
int ElementDataGridAddColumn(lua_State* L, ElementDataGrid* obj)
{
    RMLUI_CHECK_OBJ(obj);
    const char* fields = luaL_checkstring(L,1);
    const char* formatter = luaL_checkstring(L,2);
    float width = (float)luaL_checknumber(L,3);
    const char* rml = luaL_checkstring(L,4);

    obj->AddColumn(fields,formatter,width,rml);
    return 0;
}

int ElementDataGridSetDataSource(lua_State* L, ElementDataGrid* obj)
{
    RMLUI_CHECK_OBJ(obj);
    const char* source = luaL_checkstring(L,1);
    
    obj->SetDataSource(source);
    return 0;
}


//getter
int ElementDataGridGetAttrrows(lua_State* L)
{
    ElementDataGrid* obj = LuaType<ElementDataGrid>::check(L,1);
    RMLUI_CHECK_OBJ(obj);

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
    RMLUI_LUAMETHOD(ElementDataGrid,AddColumn)
    RMLUI_LUAMETHOD(ElementDataGrid,SetDataSource)
    { nullptr, nullptr },
};

luaL_Reg ElementDataGridGetters[] =
{
    RMLUI_LUAGETTER(ElementDataGrid,rows)
    { nullptr, nullptr },
};

luaL_Reg ElementDataGridSetters[] =
{
    { nullptr, nullptr },
};


template<> void ExtraInit<ElementDataGrid>(lua_State* L, int metatable_index)
{
    ExtraInit<Element>(L,metatable_index);
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
    AddTypeToElementAsTable<ElementDataGrid>(L);
}
RMLUI_LUATYPE_DEFINE(ElementDataGrid)
} // namespace Lua
} // namespace Rml
