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
#include "ElementAttributesProxy.h"
#include <RmlUi/Core/Variant.h>
#include <RmlUi/Core/Lua/Utilities.h>

namespace Rml {
namespace Core {
namespace Lua {
template<> void ExtraInit<ElementAttributesProxy>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,ElementAttributesProxy__index);
    lua_setfield(L,metatable_index,"__index");
    lua_pushcfunction(L,ElementAttributesProxy__pairs);
    lua_setfield(L,metatable_index,"__pairs");
    lua_pushcfunction(L,ElementAttributesProxy__ipairs);
    lua_setfield(L,metatable_index,"__ipairs");
}

int ElementAttributesProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int keytype = lua_type(L,2);
    if(keytype == LUA_TSTRING) //only valid key types
    {
        ElementAttributesProxy* obj = LuaType<ElementAttributesProxy>::check(L,1);
        LUACHECKOBJ(obj);
        const char* key = lua_tostring(L,2);
        Variant* attr = obj->owner->GetAttribute(key);
        PushVariant(L,attr); //Utilities.h
        return 1;
    }
    else
        return LuaType<ElementAttributesProxy>::index(L);
}

//[1] is the object, [2] is the key that was used previously, [3] is the userdata
int ElementAttributesProxy__pairs(lua_State* L)
{
    ElementAttributesProxy* obj = LuaType<ElementAttributesProxy>::check(L,1);
    LUACHECKOBJ(obj);
    int& pindex = *(int*)lua_touserdata(L,3);
    if((pindex) == -1) 
        pindex = 0;
	const ElementAttributes& attributes = obj->owner->GetAttributes();

    if(pindex >= 0 && pindex < (int)attributes.size())
    {
		auto it = attributes.begin();
		for (int i = 0; i < pindex; ++i)
			++it;
		const String& key = it->first;
		const Variant* value = &it->second;
        lua_pushstring(L,key.c_str());
        PushVariant(L,value);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 2;
}

//Doesn't index by integer, so don't return anything
int ElementAttributesProxy__ipairs(lua_State* L)
{
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}

RegType<ElementAttributesProxy> ElementAttributesProxyMethods[] =
{
    { nullptr, nullptr },
};

luaL_Reg ElementAttributesProxyGetters[] =
{
    { nullptr, nullptr },
};
luaL_Reg ElementAttributesProxySetters[] =
{
    { nullptr, nullptr },
};

LUACORETYPEDEFINE(ElementAttributesProxy)
}
}
}
