#include "precompiled.h"
#include "LuaType.h"
#include "lua.hpp"
#include "ElementStyle.h"
#include <ElementStyle.h>

namespace Rocket {
namespace Core {
namespace Lua {


int ElementStyle__index(lua_State* L)
{
    /*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
    int keytype = lua_type(L,2);
    if(keytype == LUA_TSTRING) //if we are trying to access a string, then we will assume that it is a property
    {
        ElementStyle* es = LuaType<ElementStyle>::check(L,1);
        if(es == NULL)
        {
            lua_pushnil(L);
            return 1;
        }
        const Property* prop = es->GetProperty(lua_tostring(L,2));
        if(prop == NULL)
        {
            lua_pushnil(L);
            return 1;
        }
        else
        {
            lua_pushstring(L,prop->ToString().CString());
            return 1;
        }
    }
    else //if it wasn't trying to get a string
    {
        lua_settop(L,2);
        return LuaType<ElementStyle>::index(L);
    }
}

int ElementStyle__newindex(lua_State* L)
{
    //[1] = obj, [2] = key, [3] = value
    ElementStyle* es = LuaType<ElementStyle>::check(L,1);
    if(es == NULL)
    {
        lua_pushnil(L);
        return 1;
    }
    int keytype = lua_type(L,2);
    int valuetype = lua_type(L,3);
    if(keytype == LUA_TSTRING )
    {
        const char* key = lua_tostring(L,2);
        if(valuetype == LUA_TSTRING)
        {
            const char* value = lua_tostring(L,3);
            lua_pushboolean(L,es->SetProperty(key,value));
            return 1; 
        }
        else if (valuetype == LUA_TNIL)
        {
            es->RemoveProperty(key);
            return 0;
        }
    }
    //everything else returns when it needs to, so we are safe to pass it
    //on if needed

    lua_settop(L,3);
    return LuaType<ElementStyle>::newindex(L);

}


int ElementStyleGetTable(lua_State* L, ElementStyle* obj)
{
    LUACHECKOBJ(obj);
    int index = 0;
    String key,sval;
    const Property* value;
    PseudoClassList pseudo;
    lua_newtable(L);
    while(obj->IterateProperties(index,pseudo,key,value))
    {
        lua_pushstring(L,key.CString());
        value->definition->GetValue(sval,*value);
        lua_pushstring(L,sval.CString());
        lua_settable(L,-3);
    }
    return 1;
}


RegType<ElementStyle> ElementStyleMethods[] = 
{
    LUAMETHOD(ElementStyle,GetTable)
    { NULL, NULL },
};

luaL_reg ElementStyleGetters[] = 
{
    { NULL, NULL },
};

luaL_reg ElementStyleSetters[] = 
{
    { NULL, NULL },
};

/*
template<> const char* GetTClassName<ElementStyle>() { return "ElementStyle"; }
template<> RegType<ElementStyle>* GetMethodTable<ElementStyle>() { return ElementStyleMethods; }
template<> luaL_reg* GetAttrTable<ElementStyle>() { return ElementStyleGetters; }
template<> luaL_reg* SetAttrTable<ElementStyle>() { return ElementStyleSetters; }
*/
}
}
}