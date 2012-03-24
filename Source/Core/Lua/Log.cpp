#include "precompiled.h"
#include "Log.h"
#include <Rocket/Core/Log.h>


namespace Rocket {
namespace Core {
namespace Lua {

template<> void LuaType<Log>::extra_init(lua_State* L, int metatable_index)
{
    //due to they way that LuaType::Register is made, we know that the method table is at the index
    //directly below the metatable
    int method_index = metatable_index - 1;

    lua_pushcfunction(L,Log__call);
    lua_setfield(L,metatable_index, "__call");

    //construct the "logtype" table, so that we can use the Rocket::Core::Log::Type enum like Log.logtype.always in Lua for Log::LT_ALWAYS
    lua_newtable(L);
    int logtype = lua_gettop(L);
    lua_pushvalue(L,-1); //copy of the new table, so that the logtype index will stay valid
    lua_setfield(L,method_index,"logtype");

    lua_pushinteger(L,(int)Log::LT_ALWAYS);
    lua_setfield(L,logtype,"always");

    lua_pushinteger(L,(int)Log::LT_ERROR);
    lua_setfield(L,logtype,"error");

    lua_pushinteger(L,(int)Log::LT_WARNING);
    lua_setfield(L,logtype,"warning");

    lua_pushinteger(L,(int)Log::LT_INFO);
    lua_setfield(L,logtype,"info");

    lua_pushinteger(L,(int)Log::LT_DEBUG);
    lua_setfield(L,logtype,"debug");

    lua_pop(L,1); //pop the logtype table

    return;
}


int Log__call(lua_State* L)
{
    Log::Type type = Log::Type((int)luaL_checkinteger(L,1));
    const char* str = luaL_checkstring(L,2);
    
    Log::Message(type, str);
    return 0;
}


RegType<Log> LogMethods[] =
{
    { NULL, NULL },
};

luaL_reg LogGetters[] =
{
    { NULL, NULL },
};

luaL_reg LogSetters[] =
{
    { NULL, NULL },
};

template<> const char* GetTClassName<Log>() { return "Log"; }
template<> RegType<Log>* GetMethodTable<Log>() { return LogMethods; }
template<> luaL_reg* GetAttrTable<Log>() { return LogGetters; }
template<> luaL_reg* SetAttrTable<Log>() { return LogSetters; }

}
}
}