#include "precompiled.h"
#include "SelectOptionsProxy.h"
#include <Rocket/Core/Element.h>

namespace Rocket {
namespace Core {
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
        LuaType<Element>::push(L,opt->GetElement(),false);
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
    Element* ele;
    String value;
    lua_newtable(L); //table to return
    int retindex = lua_gettop(L);
    for(int index = 0; index < numOptions; index++)
    {
        opt = obj->owner->GetOption(index);
        if(opt == NULL) continue;
    
        ele = opt->GetElement();
        value = opt->GetValue();

        lua_newtable(L);
        LuaType<Element>::push(L,ele,false);
        lua_setfield(L,-2,"element");
        lua_pushstring(L,value.CString());
        lua_setfield(L,-2,"value");
        lua_rawseti(L,retindex,index); //sets the table that is being returned's 'index' to be the table with element and value
    }
    return 1;
}

RegType<SelectOptionsProxy> SelectOptionsProxyMethods[] =
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
