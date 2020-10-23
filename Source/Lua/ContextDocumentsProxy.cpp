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
 
#include "ContextDocumentsProxy.h"
#include <RmlUi/Core/ElementDocument.h>

namespace Rml {
namespace Lua {
typedef ElementDocument Document;
template<> void ExtraInit<ContextDocumentsProxy>(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,ContextDocumentsProxy__index);
    lua_setfield(L,metatable_index,"__index");
    lua_pushcfunction(L,ContextDocumentsProxy__pairs);
    lua_setfield(L,metatable_index,"__pairs");
}

int ContextDocumentsProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int type = lua_type(L,2);
    if(type == LUA_TNUMBER || type == LUA_TSTRING) //only valid key types
    {
        ContextDocumentsProxy* proxy = LuaType<ContextDocumentsProxy>::check(L,1);
        Document* ret = nullptr;
        if(type == LUA_TSTRING)
            ret = proxy->owner->GetDocument(luaL_checkstring(L,2));
        else
            ret = proxy->owner->GetDocument((int)luaL_checkinteger(L,2)-1);
        LuaType<Document>::push(L,ret,false);
        return 1;
    }
    else
        return LuaType<ContextDocumentsProxy>::index(L);
    
}

struct ContextDocumentsProxyPairs
{
    static int next(lua_State* L) 
    {
        ContextDocumentsProxy* obj = LuaType<ContextDocumentsProxy>::check(L,1);
        ContextDocumentsProxyPairs* self = static_cast<ContextDocumentsProxyPairs*>(lua_touserdata(L, lua_upvalueindex(1)));
        Document* doc = nullptr;
        int num_docs = obj->owner->GetNumDocuments();
        //because there can be missing indexes, make sure to continue until there
        //is actually a document at the index
        while (self->m_cur < num_docs)
        {
            doc = obj->owner->GetDocument(self->m_cur++);
            if (doc != nullptr)
                break;
        }
        if (doc == nullptr)
        {
            return 0;
        }
        lua_pushstring(L,doc->GetId().c_str());
        LuaType<Document>::push(L,doc);
        return 2;
    }
    static int destroy(lua_State* L)
    {
        static_cast<ContextDocumentsProxyPairs*>(lua_touserdata(L, 1))->~ContextDocumentsProxyPairs();
        return 0;
    }
    static int constructor(lua_State* L)
    {
        void* storage = lua_newuserdata(L, sizeof(ContextDocumentsProxyPairs));
        if (luaL_newmetatable(L, "RmlUi::Lua::ContextDocumentsProxyPairs"))
        {
            static luaL_Reg mt[] =
            {
                {"__gc", destroy},
                {NULL, NULL},
            };
            luaL_setfuncs(L, mt, 0);
        }
        lua_setmetatable(L, -2);
        lua_pushcclosure(L, next, 1);
        new (storage) ContextDocumentsProxyPairs();
        return 1;
    }
    ContextDocumentsProxyPairs()
        : m_cur(0)
    { }
    int m_cur;
};

int ContextDocumentsProxy__pairs(lua_State* L)
{
    ContextDocumentsProxyPairs::constructor(L);
    lua_pushvalue(L, 1);
    return 2;
}

RegType<ContextDocumentsProxy> ContextDocumentsProxyMethods[] =
{
    { nullptr, nullptr },
};

luaL_Reg ContextDocumentsProxyGetters[] =
{
    { nullptr, nullptr },
};

luaL_Reg ContextDocumentsProxySetters[] =
{
    { nullptr, nullptr },
};

RMLUI_LUATYPE_DEFINE(ContextDocumentsProxy)

} // namespace Lua
} // namespace Rml
