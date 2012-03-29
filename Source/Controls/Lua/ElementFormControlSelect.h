#pragma once

/*
    This defines the ElementFormControlSelect type in the Lua global namespace, for this documentation will be
    named EFCSelect

    It has one extra method than the Python api, which is GetOption

    //methods
    int EFCSelect:Add(string rml, string value, [int before]) --where 'before' is optional, and is an index
    noreturn EFCSelect:Remove(int index)
    {"element"=Element,"value"=string} EFCSelect:GetOption(int index) --this is a more efficient way to get an option if you know the index beforehand

    //getters
    {[int index]={"element"=Element,"value"=string}} EFCSelect.options --used to access options as a Lua table. Comparatively expensive operation, use GetOption when 
                                                                       --you only need one option and you know the index
    int EFCSelect.selection

    //setter
    EFCSelect.selection = int
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/ElementFormControlSelect.h>

using Rocket::Controls::ElementFormControlSelect;
namespace Rocket {
namespace Core {
namespace Lua {
//inherits from ElementFormControl which inherits from Element
template<> void LuaType<ElementFormControlSelect>::extra_init(lua_State* L, int metatable_index);

//methods
int ElementFormControlSelectAdd(lua_State* L, ElementFormControlSelect* obj);
int ElementFormControlSelectRemove(lua_State* L, ElementFormControlSelect* obj);
int ElementFormControlSelectGetOption(lua_State* L, ElementFormControlSelect* obj);

//getters
int ElementFormControlSelectGetAttroptions(lua_State* L);
int ElementFormControlSelectGetAttrselection(lua_State* L);

//setter
int ElementFormControlSelectSetAttrselection(lua_State* L);

RegType<ElementFormControlSelect> ElementFormControlSelectMethods[];
luaL_reg ElementFormControlSelectGetters[];
luaL_reg ElementFormControlSelectSetters[];

template<> const char* GetTClassName<ElementFormControlSelect>();
template<> RegType<ElementFormControlSelect>* GetMethodTable<ElementFormControlSelect>();
template<> luaL_reg* GetAttrTable<ElementFormControlSelect>();
template<> luaL_reg* SetAttrTable<ElementFormControlSelect>();

}
}
}