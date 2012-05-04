#include "precompiled.h"
#include "ElementAttributesProxy.h"
#include <Rocket/Core/Variant.h>
#include "Utilities.h"

namespace Rocket {
namespace Core {
namespace Lua {
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

//method
int ElementAttributesProxyGetTable(lua_State* L, ElementAttributesProxy* obj)
{    
    int index;
    String key;
    Variant* var;

    lua_newtable(L);
    int tbl = lua_gettop(L);
    while(obj->owner->IterateAttributes(index,key,var))
    {
        lua_pushstring(L,key.CString());
        PushVariant(L,var);
        lua_settable(L,tbl);
    }
    return 1;
}

RegType<ElementAttributesProxy> ElementAttributesProxyMethods[] =
{
    LUAMETHOD(ElementAttributesProxy,GetTable)
    { NULL, NULL },
};

luaL_reg ElementAttributesProxyGetters[] =
{
    { NULL, NULL },
};
luaL_reg ElementAttributesProxySetters[] =
{
    { NULL, NULL },
};
}
}
}
