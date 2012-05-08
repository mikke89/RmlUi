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
 
#include "precompiled.h"
#include "EventParametersProxy.h"
#include "Utilities.h"
#include <Rocket/Core/Variant.h>
#include <Rocket/Core/Dictionary.h>


namespace Rocket {
namespace Core {
namespace Lua {
int EventParametersProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int keytype = lua_type(L,2);
    if(keytype == LUA_TSTRING) //only valid key types
    {
        EventParametersProxy* obj = LuaType<EventParametersProxy>::check(L,1);
        LUACHECKOBJ(obj);
        const char* key = lua_tostring(L,2);
        Variant* param = obj->owner->GetParameters()->Get(key);
        PushVariant(L,param);
        return 1;
    }
    else
        return LuaType<EventParametersProxy>::index(L);
}

//method
int EventParametersProxyGetTable(lua_State* L, EventParametersProxy* obj)
{
    const Dictionary* params = obj->owner->GetParameters();
    int index = 0;
    String key;
    Variant* value;

    lua_newtable(L);
    int tableindex = lua_gettop(L);
    while(params->Iterate(index,key,value))
    {
        lua_pushstring(L,key.CString());
        PushVariant(L,value);
        lua_settable(L,tableindex);
    }
    return 1;
}

RegType<EventParametersProxy> EventParametersProxyMethods[] =
{
    LUAMETHOD(EventParametersProxy,GetTable)
    { NULL, NULL },
};
luaL_reg EventParametersProxyGetters[] =
{
    { NULL, NULL },
};
luaL_reg EventParametersProxySetters[] =
{
    { NULL, NULL },
};
}
}
}