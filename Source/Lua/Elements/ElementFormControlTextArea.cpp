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
 
#include "ElementFormControlTextArea.h"
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include "ElementFormControl.h"
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

//getters
int ElementFormControlTextAreaGetAttrcols(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    lua_pushinteger(L,obj->GetNumColumns());
    return 1;
}

int ElementFormControlTextAreaGetAttrmaxlength(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    lua_pushinteger(L,obj->GetMaxLength());
    return 1;
}

int ElementFormControlTextAreaGetAttrrows(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    lua_pushinteger(L,obj->GetNumRows());
    return 1;
}

int ElementFormControlTextAreaGetAttrwordwrap(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    lua_pushboolean(L,obj->GetWordWrap());
    return 1;
}


//setters
int ElementFormControlTextAreaSetAttrcols(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int cols = (int)luaL_checkinteger(L,2);
    obj->SetNumColumns(cols);
    return 0;
}

int ElementFormControlTextAreaSetAttrmaxlength(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int ml = (int)luaL_checkinteger(L,2);
    obj->SetMaxLength(ml);
    return 0;
}

int ElementFormControlTextAreaSetAttrrows(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int rows = (int)luaL_checkinteger(L,2);
    obj->SetNumRows(rows);
    return 0;
}

int ElementFormControlTextAreaSetAttrwordwrap(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    bool ww = RMLUI_CHECK_BOOL(L,2);
    obj->SetWordWrap(ww);
    return 0;
}


RegType<ElementFormControlTextArea> ElementFormControlTextAreaMethods[] =
{
    { nullptr, nullptr },
};

luaL_Reg ElementFormControlTextAreaGetters[] =
{
    RMLUI_LUAGETTER(ElementFormControlTextArea,cols)
    RMLUI_LUAGETTER(ElementFormControlTextArea,maxlength)
    RMLUI_LUAGETTER(ElementFormControlTextArea,rows)
    RMLUI_LUAGETTER(ElementFormControlTextArea,wordwrap)
    { nullptr, nullptr },
};

luaL_Reg ElementFormControlTextAreaSetters[] =
{
    RMLUI_LUASETTER(ElementFormControlTextArea,cols)
    RMLUI_LUASETTER(ElementFormControlTextArea,maxlength)
    RMLUI_LUASETTER(ElementFormControlTextArea,rows)
    RMLUI_LUASETTER(ElementFormControlTextArea,wordwrap)
    { nullptr, nullptr },
};


template<> void ExtraInit<ElementFormControlTextArea>(lua_State* L, int metatable_index)
{
    ExtraInit<ElementFormControl>(L,metatable_index);
    LuaType<ElementFormControl>::_regfunctions(L,metatable_index,metatable_index-1);
    AddTypeToElementAsTable<ElementFormControlTextArea>(L);
}

RMLUI_LUATYPE_DEFINE(ElementFormControlTextArea)
} // namespace Lua
} // namespace Rml
