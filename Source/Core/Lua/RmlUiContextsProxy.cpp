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
 
#include "precompiled.h"
#include "RmlUiContextsProxy.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>


namespace Rml {
namespace Core {
namespace Lua {

template<> void ExtraInit<RmlUiContextsProxy>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,RmlUiContextsProxy__index);
    lua_setfield(L,metatable_index,"__index");
    lua_pushcfunction(L,RmlUiContextsProxy__pairs);
    lua_setfield(L,metatable_index,"__pairs");
    lua_pushcfunction(L,RmlUiContextsProxy__ipairs);
    lua_setfield(L,metatable_index,"__ipairs");
}

int RmlUiContextsProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int keytype = lua_type(L,2);
    if(keytype == LUA_TSTRING || keytype == LUA_TNUMBER) //only valid key types
    {
        RmlUiContextsProxy* obj = LuaType<RmlUiContextsProxy>::check(L,1);
        LUACHECKOBJ(obj);
        if(keytype == LUA_TSTRING)
        {
            const char* key = lua_tostring(L,2);
            LuaType<Context>::push(L,GetContext(key));
        }
        else
        {
            int key = (int)luaL_checkinteger(L,2);
            LuaType<Context>::push(L,GetContext(key));
        }
        return 1;
    }
    else
        return LuaType<RmlUiContextsProxy>::index(L);
}


//[1] is the object, [2] is the last used key, [3] is the userdata
int RmlUiContextsProxy__pairs(lua_State* L)
{
    RmlUiContextsProxy* obj = LuaType<RmlUiContextsProxy>::check(L,1);
    LUACHECKOBJ(obj);
    int* pindex = (int*)lua_touserdata(L,3);
    if((*pindex) == -1)
        *pindex = 0;
    Context* value = nullptr;
    if((*pindex)++ < GetNumContexts())
    {
        value = GetContext(*pindex);
    }
    if(value == nullptr)
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    else
    {
        lua_pushstring(L,value->GetName().c_str());
        LuaType<Context>::push(L,value);
    }
    return 2;
}

//[1] is the object, [2] is the last used key, [3] is the userdata
int RmlUiContextsProxy__ipairs(lua_State* L)
{
    RmlUiContextsProxy* obj = LuaType<RmlUiContextsProxy>::check(L,1);
    LUACHECKOBJ(obj);
    int* pindex = (int*)lua_touserdata(L,3);
    if((*pindex) == -1)
        *pindex = 0;
    Context* value = nullptr;
    if((*pindex)++ < GetNumContexts())
    {
        value = GetContext(*pindex);
    }
    if(value == nullptr)
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    else
    {
        lua_pushinteger(L,(*pindex)-1);
        LuaType<Context>::push(L,value);
    }
    return 2;
}

RegType<RmlUiContextsProxy> RmlUiContextsProxyMethods[] =
{
    { nullptr, nullptr },
};
luaL_Reg RmlUiContextsProxyGetters[] =
{
    { nullptr, nullptr },
};
luaL_Reg RmlUiContextsProxySetters[] =
{
    { nullptr, nullptr },
};

LUACORETYPEDEFINE(RmlUiContextsProxy)
}
}
}