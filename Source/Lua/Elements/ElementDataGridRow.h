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
 
#ifndef RMLUI_LUA_ELEMENTS_ELEMENTDATAGRIDROW_H
#define RMLUI_LUA_ELEMENTS_ELEMENTDATAGRIDROW_H


#include <RmlUi/Lua/LuaType.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Core/Elements/ElementDataGridRow.h>

namespace Rml {
namespace Lua {

//getters
int ElementDataGridRowGetAttrrow_expanded(lua_State* L);
int ElementDataGridRowGetAttrparent_relative_index(lua_State* L);
int ElementDataGridRowGetAttrtable_relative_index(lua_State* L);
int ElementDataGridRowGetAttrparent_row(lua_State* L);
int ElementDataGridRowGetAttrparent_grid(lua_State* L);

//setter
int ElementDataGridRowSetAttrrow_expanded(lua_State* L);


extern RegType<ElementDataGridRow> ElementDataGridRowMethods[];
extern luaL_Reg ElementDataGridRowGetters[];
extern luaL_Reg ElementDataGridRowSetters[];


//this will be used to "inherit" from Element
template<> void ExtraInit<ElementDataGridRow>(lua_State* L, int metatable_index);
RMLUI_LUATYPE_DECLARE(ElementDataGridRow)
} // namespace Lua
} // namespace Rml

#endif
