#pragma once
/*
    This defines the ElementForm type in the Lua global namespace

    methods:
    ElementForm:Submit(string name,string value)

    for everything else, see the documentation for "Element"
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Controls/ElementForm.h>

using Rocket::Controls::ElementForm;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementForm>::extra_init(lua_State* L, int metatable_index);
template<> bool LuaType<ElementForm>::is_reference_counted();

//method
int ElementFormSubmit(lua_State* L, ElementForm* obj);

RegType<ElementForm> ElementFormMethods[];
luaL_reg ElementFormGetters[];
luaL_reg ElementFormSetters[];


/*
template<> const char* GetTClassName<ElementForm>() { return "ElementForm"; }
template<> RegType<ElementForm>* GetMethodTable<ElementForm>() { return ElementFormMethods; }
template<> luaL_reg* GetAttrTable<ElementForm>() { return ElementFormGetters; }
template<> luaL_reg* SetAttrTable<ElementForm>() { return ElementFormSetters; }

*/
}
}
}
