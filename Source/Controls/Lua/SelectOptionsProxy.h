#ifndef ROCKETCORELUASELECTOPTIONSPROXY_H
#define ROCKETCORELUASELECTOPTIONSPROXY_H
/*
    Proxy table for ElementFormControlSelect.options
    read-only, key must be a number
    Each object in this proxy is a table with two items:
    Element element and String value
    Usage:
    ElementFormControlSelect.options[2].element or ElementFormControlSelect.options[2].value
    OR, as usual you can store the object in a variable like
    local opt = ElementFormControlSelect.options
    opt[2].element or opt[2].value
    and you can store the returned table as a variable, of course
    local item = opt[2]
    item.element or item.value
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/ElementFormControlSelect.h>

namespace Rocket {
namespace Core {
namespace Lua {
//where owner is the ElementFormControlSelect that we should look up information from
struct SelectOptionsProxy { Rocket::Controls::ElementFormControlSelect* owner;  };

template<> void LuaType<SelectOptionsProxy>::extra_init(lua_State* L, int metatable_index);
int SelectOptionsProxy__index(lua_State* L);

//method
int SelectOptionsProxyGetTable(lua_State* L, SelectOptionsProxy* obj);

RegType<SelectOptionsProxy> SelectOptionsProxyMethods[];
luaL_reg SelectOptionsProxyGetters[];
luaL_reg SelectOptionsProxySetters[];
}
}
}
#endif
