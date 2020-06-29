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
 
#include "ElementTabSet.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Lua/Utilities.h>


namespace Rml {
namespace Lua {

//methods
int ElementTabSetSetPanel(lua_State* L, ElementTabSet* obj)
{
    RMLUI_CHECK_OBJ(obj);
    int index = (int)luaL_checkinteger(L,1);
    const char* rml = luaL_checkstring(L,2);

    obj->SetPanel(index,rml);
    return 0;
}

int ElementTabSetSetTab(lua_State* L, ElementTabSet* obj)
{
    RMLUI_CHECK_OBJ(obj);
    int index = (int)luaL_checkinteger(L,1);
    const char* rml = luaL_checkstring(L,2);

    obj->SetTab(index,rml);
    return 0;
}


//getters
int ElementTabSetGetAttractive_tab(lua_State* L)
{
    ElementTabSet* obj = LuaType<ElementTabSet>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int tab = obj->GetActiveTab();
    lua_pushinteger(L,tab);
    return 1;
}

int ElementTabSetGetAttrnum_tabs(lua_State* L)
{
    ElementTabSet* obj = LuaType<ElementTabSet>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int num = obj->GetNumTabs();
    lua_pushinteger(L,num);
    return 1;
}


//setter
int ElementTabSetSetAttractive_tab(lua_State* L)
{
    ElementTabSet* obj = LuaType<ElementTabSet>::check(L,1);
    RMLUI_CHECK_OBJ(obj);
    int tab = (int)luaL_checkinteger(L,2);
    obj->SetActiveTab(tab);
    return 0;
}


RegType<ElementTabSet> ElementTabSetMethods[] =
{
    RMLUI_LUAMETHOD(ElementTabSet,SetPanel)
    RMLUI_LUAMETHOD(ElementTabSet,SetTab)
    { nullptr, nullptr },
};

luaL_Reg ElementTabSetGetters[] =
{
    RMLUI_LUAGETTER(ElementTabSet,active_tab)
    RMLUI_LUAGETTER(ElementTabSet,num_tabs)
    { nullptr, nullptr },
};

luaL_Reg ElementTabSetSetters[] =
{
    RMLUI_LUASETTER(ElementTabSet,active_tab)
    { nullptr, nullptr },
};


//this will be used to "inherit" from Element
template<> void ExtraInit<ElementTabSet>(lua_State* L, int metatable_index)
{
    ExtraInit<Element>(L,metatable_index);
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
    AddTypeToElementAsTable<ElementTabSet>(L);
}

RMLUI_LUATYPE_DEFINE(ElementTabSet)
} // namespace Lua
} // namespace Rml
