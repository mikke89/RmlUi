#ifndef ROCKETCORELUAEVENTPARAMETERSPROXY_H
#define ROCKETCORELUAEVENTPARAMETERSPROXY_H
/*
    Proxy table for Event.parameters
    read-only Dictionary
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Event.h>

namespace Rocket {
namespace Core {
namespace Lua {
//where owner is the Element that we should look up information from
struct EventParametersProxy { Event* owner;  };

template<> void LuaType<EventParametersProxy>::extra_init(lua_State* L, int metatable_index);
int EventParametersProxy__index(lua_State* L);

//method
int EventParametersProxyGetTable(lua_State* L, EventParametersProxy* obj);

RegType<EventParametersProxy> EventParametersProxyMethods[];
luaL_reg EventParametersProxyGetters[];
luaL_reg EventParametersProxySetters[];
}
}
}
#endif
