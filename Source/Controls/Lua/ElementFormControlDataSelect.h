#pragma once
/*
    This defines the ElementFormControlDataSelect type in the Lua global namespace. I think it is the longest
    type name.

    It inherits from ElementFormControlSelect, which inherits from ElementFormControl, which inherits from Element

    //method
    noreturn ElementFormControlDataSelect:SetDataSource(string source)
*/


#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/ElementFormControlDataSelect.h>

using Rocket::Controls::ElementFormControlDataSelect;
namespace Rocket {
namespace Core {
namespace Lua {
//inherits from ElementFormControl which inherits from Element
template<> void LuaType<ElementFormControlDataSelect>::extra_init(lua_State* L, int metatable_index);
template<> bool LuaType<ElementFormControlDataSelect>::is_reference_counted();

//method
int ElementFormControlDataSelectSetDataSource(lua_State* L, ElementFormControlDataSelect* obj);

RegType<ElementFormControlDataSelect> ElementFormControlDataSelectMethods[];
luaL_reg ElementFormControlDataSelectGetters[];
luaL_reg ElementFormControlDataSelectSetters[];

/*
template<> const char* GetTClassName<ElementFormControlDataSelect>();
template<> RegType<ElementFormControlDataSelect>* GetMethodTable<ElementFormControlDataSelect>();
template<> luaL_reg* GetAttrTable<ElementFormControlDataSelect>();
template<> luaL_reg* SetAttrTable<ElementFormControlDataSelect>();
*/

}
}
}