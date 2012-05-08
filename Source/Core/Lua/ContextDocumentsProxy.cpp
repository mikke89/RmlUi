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
#include "ContextDocumentsProxy.h"
#include <Rocket/Core/ElementDocument.h>

namespace Rocket {
namespace Core {
namespace Lua {
typedef Rocket::Core::ElementDocument Document;
int ContextDocumentsProxy__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int type = lua_type(L,2);
    if(type == LUA_TNUMBER || type == LUA_TSTRING) //only valid key types
    {
        ContextDocumentsProxy* proxy = LuaType<ContextDocumentsProxy>::check(L,1);
        Document* ret = NULL;
        if(type == LUA_TSTRING)
            ret = proxy->owner->GetDocument(luaL_checkstring(L,2));
        else
            ret = proxy->owner->GetDocument(luaL_checkint(L,2));
        LuaType<Document>::push(L,ret,false);
        return 1;
    }
    else
        return LuaType<ContextDocumentsProxy>::index(L);
    
}

//method
int ContextDocumentsProxyGetTable(lua_State* L, ContextDocumentsProxy* obj)
{
    Context* cont = obj->owner;
    Element* root = cont->GetRootElement();

    lua_newtable(L);
    int tableindex = lua_gettop(L);
    for(int i = 0; i < root->GetNumChildren(); i++)
    {
        Document* doc = root->GetChild(i)->GetOwnerDocument();
        if(doc == NULL)
            continue;

        LuaType<Document>::push(L,doc);
        lua_pushvalue(L,-1); //put it on the stack twice, since we assign it to 
                                //both a string and integer index
        lua_setfield(L, tableindex,doc->GetId().CString());
        lua_rawseti(L,tableindex,i);
    }
    lua_settop(L,tableindex); //to make sure
    return 1;
}

RegType<ContextDocumentsProxy> ContextDocumentsProxyMethods[] =
{
    LUAMETHOD(ContextDocumentsProxy,GetTable)
    { NULL, NULL },
};

luaL_reg ContextDocumentsProxyGetters[] =
{
    { NULL, NULL },
};

luaL_reg ContextDocumentsProxySetters[] =
{
    { NULL, NULL },
};

}
}
}