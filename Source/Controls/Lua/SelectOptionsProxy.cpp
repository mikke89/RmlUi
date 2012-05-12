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
#include "SelectOptionsProxy.h"
#include <Rocket/Core/Element.h>

template<> void Rocket::Core::Lua::ExtraInit<Rocket::Controls::Lua::SelectOptionsProxy>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,Rocket::Controls::Lua::SelectOptionsProxy__index);
    lua_setfield(L,metatable_index,"__index");
}


namespace Rocket {
namespace Controls {
namespace Lua {


int SelectOptionsProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int keytype = lua_type(L,2);
    if(keytype == LUA_TNUMBER) //only valid key types
    {
        SelectOptionsProxy* proxy = LuaType<SelectOptionsProxy>::check(L,1);
        LUACHECKOBJ(proxy);
        int index = luaL_checkint(L,2);
        Rocket::Controls::SelectOption* opt = proxy->owner->GetOption(index);
        LUACHECKOBJ(opt);
        lua_newtable(L);
        LuaType<Rocket::Core::Element>::push(L,opt->GetElement(),false);
        lua_setfield(L,-2,"element");
        lua_pushstring(L,opt->GetValue().CString());
        lua_setfield(L,-2,"value");
        return 1;
    }
    else
        return LuaType<SelectOptionsProxy>::index(L);
}

//method
int SelectOptionsProxyGetTable(lua_State* L, SelectOptionsProxy* obj)
{
    int numOptions = obj->owner->GetNumOptions();

    //local variables for the loop
    Rocket::Controls::SelectOption* opt; 
    Rocket::Core::Element* ele;
    Rocket::Core::String value;
    lua_newtable(L); //table to return
    int retindex = lua_gettop(L);
    for(int index = 0; index < numOptions; index++)
    {
        opt = obj->owner->GetOption(index);
        if(opt == NULL) continue;
    
        ele = opt->GetElement();
        value = opt->GetValue();

        lua_newtable(L);
        LuaType<Rocket::Core::Element>::push(L,ele,false);
        lua_setfield(L,-2,"element");
        lua_pushstring(L,value.CString());
        lua_setfield(L,-2,"value");
        lua_rawseti(L,retindex,index); //sets the table that is being returned's 'index' to be the table with element and value
    }
    return 1;
}

Rocket::Core::Lua::RegType<SelectOptionsProxy> SelectOptionsProxyMethods[] =
{
    LUAMETHOD(SelectOptionsProxy,GetTable)
    { NULL, NULL },
};

luaL_reg SelectOptionsProxyGetters[] =
{
    { NULL, NULL },
};

luaL_reg SelectOptionsProxySetters[] =
{
    { NULL, NULL },
};


}
}
}
using Rocket::Controls::Lua::SelectOptionsProxy;
LUACONTROLSTYPEDEFINE(SelectOptionsProxy,false);
