#include "LuaType.h"
#include "lua.hpp"

/*
    Declares "Log" in the global Lua namespace.

    //It is not nessecarry to call it on a "Log" object, just
    //call it on the Log table
    Log(logtype type, string message)

    where logtype is defined in Log.logtype, and can be:
    logtype.always
    logtype.error
    logtype.warning
    logtype.info
    logtype.debug
    and they have the same value as the C++ Log::Type of the same name
*/

namespace Rocket {
namespace Core {
namespace Lua {

template<> void LuaType<Log>::extra_init(lua_State* L, int metatable_index);
int Log__call(lua_State* L);

RegType<Log> LogMethods[];
luaL_reg LogGetters[];
luaL_reg LogSetters[];

template<> const char* GetTClassName<Log>();
template<> RegType<Log>* GetMethodTable<Log>();
template<> luaL_reg* GetAttrTable<Log>();
template<> luaL_reg* SetAttrTable<Log>();
}
}
}