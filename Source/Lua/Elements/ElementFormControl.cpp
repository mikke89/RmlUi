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
 
#include "../Element.h"
#include "ElementFormControl.h"
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Lua/Utilities.h>


namespace Rml {
namespace Lua {

//getters
int ElementFormControlGetAttrdisabled(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    RMLUI_CHECK_OBJ(efc);
    lua_pushboolean(L,efc->IsDisabled());
    return 1;
}

int ElementFormControlGetAttrname(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    RMLUI_CHECK_OBJ(efc);
    lua_pushstring(L,efc->GetName().c_str());
    return 1;
}

int ElementFormControlGetAttrvalue(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    RMLUI_CHECK_OBJ(efc);
    lua_pushstring(L,efc->GetValue().c_str());
    return 1;
}


//setters
int ElementFormControlSetAttrdisabled(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    RMLUI_CHECK_OBJ(efc);
    efc->SetDisabled(RMLUI_CHECK_BOOL(L,2));
    return 0;
}

int ElementFormControlSetAttrname(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    RMLUI_CHECK_OBJ(efc);
    const char* name = luaL_checkstring(L,2);
    efc->SetName(name);
    return 0;
}

int ElementFormControlSetAttrvalue(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    RMLUI_CHECK_OBJ(efc);
    const char* value = luaL_checkstring(L,2);
    efc->SetValue(value);
    return 0;
}


RegType<ElementFormControl> ElementFormControlMethods[] = 
{
    { nullptr, nullptr },
};

luaL_Reg ElementFormControlGetters[] = 
{
    RMLUI_LUAGETTER(ElementFormControl,disabled)
    RMLUI_LUAGETTER(ElementFormControl,name)
    RMLUI_LUAGETTER(ElementFormControl,value)
    { nullptr, nullptr },
};

luaL_Reg ElementFormControlSetters[] = 
{
    RMLUI_LUASETTER(ElementFormControl,disabled)
    RMLUI_LUASETTER(ElementFormControl,name)
    RMLUI_LUASETTER(ElementFormControl,value)
    { nullptr, nullptr },
};


template<> void ExtraInit<ElementFormControl>(lua_State* L, int metatable_index)
{
    ExtraInit<Element>(L,metatable_index);
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
    AddTypeToElementAsTable<ElementFormControl>(L);
}
RMLUI_LUATYPE_DEFINE(ElementFormControl)
} // namespace Lua
} // namespace Rml
