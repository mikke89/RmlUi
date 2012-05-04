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