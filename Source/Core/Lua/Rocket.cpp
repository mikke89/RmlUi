#include "precompiled.h"
#include "Rocket.h"
#include <Rocket/Core/Core.h>

namespace Rocket {
namespace Core {
namespace Lua {

template<> void LuaType<rocket>::extra_init(lua_State* L, int metatable_index)
{
    //because of the way LuaType::Register is done, we know that the methods table is directly
    //before the metatable 
    int method_index = metatable_index - 1;

    lua_pushcfunction(L,rocketCreateContext);
    lua_setfield(L,method_index,"CreateContext");

    lua_pushcfunction(L,rocketLoadFontFace);
    lua_setfield(L,method_index,"LoadFontFace");

    lua_pushcfunction(L,rocketRegisterTag);
    lua_setfield(L,method_index,"RegisterTag");

    return;
}

int rocketCreateContext(lua_State* L)
{
    const char* name = luaL_checkstring(L,1);
    Vector2i* dimensions = LuaType<Vector2i>::check(L,2);
    Context* new_context = CreateContext(name, *dimensions);
    if(new_context == NULL || dimensions == NULL)
    {
        lua_pushnil(L);
        return 1;
    }
    else
    {
        LuaType<Context>::push(L, new_context);
        return 1;
    }
}

int rocketLoadFontFace(lua_State* L)
{
    const char* file = luaL_checkstring(L,1);
    FontDatabase::LoadFontFace(file);
    return 0;
}

int rocketRegisterTag(lua_State* L)
{
    return 0;
}

int rocketGetAttrcontexts(lua_State* L)
{
    return 0;
}


RegType<rocket> rocketMethods[] = 
{
    { NULL, NULL },
};

luaL_reg rocketGetters[] = 
{
    LUAGETTER(rocket,contexts)
    { NULL, NULL },
};

luaL_reg rocketSetters[] = 
{
    { NULL, NULL },
};


template<> const char* GetTClassName<rocket>() { return "rocket"; }
template<> RegType<rocket>* GetMethodTable<rocket>() { return rocketMethods; }
template<> luaL_reg* GetAttrTable<rocket>() { return rocketGetters; }
template<> luaL_reg* SetAttrTable<rocket>() { return rocketSetters; }

}
}
}