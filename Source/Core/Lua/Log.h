#include "LuaType.h"
#include "lua.hpp"

/*
    Declares "Log" in the global Lua namespace. Lua usage example:
    Log(Log.logtype.always, "Hello World")
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