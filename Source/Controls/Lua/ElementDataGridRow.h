/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
 
#ifndef ROCKETCORELUAELEMENTDATAGRIDROW_H
#define ROCKETCORELUAELEMENTDATAGRIDROW_H
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
#endif
