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