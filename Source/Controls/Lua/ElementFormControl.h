#pragma once
/*
    This defines the ElementFormControl type in the Lua global namespace

    it has no methods, and all of the attributes are read and write

    bool    ElementFormControl.disabled
    string  ElementFormControl.name
    string  ElementFormControl.value
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/ElementFormControl.h>

using Rocket::Controls::ElementFormControl;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementFormControl>::extra_init(lua_State* L, int metatable_index);

//getters
int ElementFormControlGetAttrdisabled(lua_State* L);
int ElementFormControlGetAttrname(lua_State* L);
int ElementFormControlGetAttrvalue(lua_State* L);

//setters
int ElementFormControlSetAttrdisabled(lua_State* L);
int ElementFormControlSetAttrname(lua_State* L);
int ElementFormControlSetAttrvalue(lua_State* L);

RegType<ElementFormControl> ElementFormControlMethods[];
luaL_reg ElementFormControlGetters[];
luaL_reg ElementFormControlSetters[];

template<> const char* GetTClassName<ElementFormControl>();
template<> RegType<ElementFormControl>* GetMethodTable<ElementFormControl>();
template<> luaL_reg* GetAttrTable<ElementFormControl>();
template<> luaL_reg* SetAttrTable<ElementFormControl>();

}
}
}