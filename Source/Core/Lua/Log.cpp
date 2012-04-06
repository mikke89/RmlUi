#include "precompiled.h"
#include "Log.h"
#include <Rocket/Core/Log.h>


namespace Rocket {
namespace Core {
namespace Lua {



int LogMessage(lua_State* L)
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

/*
template<> const char* GetTClassName<Log>() { return "Log"; }
template<> RegType<Log>* GetMethodTable<Log>() { return LogMethods; }
template<> luaL_reg* GetAttrTable<Log>() { return LogGetters; }
template<> luaL_reg* SetAttrTable<Log>() { return LogSetters; }
*/
}
}
}