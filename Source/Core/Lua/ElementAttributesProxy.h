#ifndef ROCKETCORELUAELEMENTATTRIBUTESPROXY_H
#define ROCKETCORELUAELEMENTATTRIBUTESPROXY_H
/*
    Proxy table for Element.attribues
    read-only Dictionary
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Element.h>

namespace Rocket {
namespace Core {
namespace Lua {
//where owner is the Element that we should look up information from
struct ElementAttributesProxy { Element* owner;  };

template<> void LuaType<ElementAttributesProxy>::extra_init(lua_State* L, int metatable_index);
int ElementAttributesProxy__index(lua_State* L);

//method
int ElementAttributesProxyGetTable(lua_State* L, ElementAttributesProxy* obj);

RegType<ElementAttributesProxy> ElementAttributesProxyMethods[];
luaL_reg ElementAttributesProxyGetters[];
luaL_reg ElementAttributesProxySetters[];
}
}
}
#endif
