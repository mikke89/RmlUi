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
 
#include "RmlUiContextsProxy.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include "Pairs.h"

namespace Rml {
namespace Lua {

template<> void ExtraInit<RmlUiContextsProxy>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,RmlUiContextsProxy__index);
    lua_setfield(L,metatable_index,"__index");
    lua_pushcfunction(L,RmlUiContextsProxy__pairs);
    lua_setfield(L,metatable_index,"__pairs");
}

int RmlUiContextsProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int keytype = lua_type(L,2);
    if(keytype == LUA_TSTRING || keytype == LUA_TNUMBER) //only valid key types
    {
        RmlUiContextsProxy* obj = LuaType<RmlUiContextsProxy>::check(L,1);
        RMLUI_CHECK_OBJ(obj);
        if(keytype == LUA_TSTRING)
        {
            const char* key = lua_tostring(L,2);
            LuaType<Context>::push(L,GetContext(key));
        }
        else
        {
            int key = (int)luaL_checkinteger(L,2);
            LuaType<Context>::push(L,GetContext(key-1));
        }
        return 1;
    }
    else
        return LuaType<RmlUiContextsProxy>::index(L);
}


int RmlUiContextsProxy__pairs(lua_State* L)
{
    return MakeIntPairs(L);
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

RMLUI_LUATYPE_DEFINE(RmlUiContextsProxy)
} // namespace Lua
} // namespace Rml
