#pragma once
/*
    This defines the type ElementFormControlInput in the Lua globla namespace, refered to in this documentation by EFCInput

    It inherits from ELementFormControl which inherits from Element

    all of the properties are read and write
    bool EFCInput.checked
    int EFCInput.maxlength
    int EFCInput.size
    int EFCInput.max
    int EFCInput.min
    int EFCInput.step
    
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/ElementFormControlInput.h>

using Rocket::Controls::ElementFormControlInput;
namespace Rocket {
namespace Core {
namespace Lua {
//inherits from ElementFormControl which inherits from Element
template<> void LuaType<ElementFormControlInput>::extra_init(lua_State* L, int metatable_index);

//getters
int ElementFormControlInputGetAttrchecked(lua_State* L);
int ElementFormControlInputGetAttrmaxlength(lua_State* L);
int ElementFormControlInputGetAttrsize(lua_State* L);
int ElementFormControlInputGetAttrmax(lua_State* L);
int ElementFormControlInputGetAttrmin(lua_State* L);
int ElementFormControlInputGetAttrstep(lua_State* L);

//setters
int ElementFormControlInputSetAttrchecked(lua_State* L);
int ElementFormControlInputSetAttrmaxlength(lua_State* L);
int ElementFormControlInputSetAttrsize(lua_State* L);
int ElementFormControlInputSetAttrmax(lua_State* L);
int ElementFormControlInputSetAttrmin(lua_State* L);
int ElementFormControlInputSetAttrstep(lua_State* L);

RegType<ElementFormControlInput> ElementFormControlInputMethods[];
luaL_reg ElementFormControlInputGetters[];
luaL_reg ElementFormControlInputSetters[];

template<> const char* GetTClassName<ElementFormControlInput>();
template<> RegType<ElementFormControlInput>* GetMethodTable<ElementFormControlInput>();
template<> luaL_reg* GetAttrTable<ElementFormControlInput>();
template<> luaL_reg* SetAttrTable<ElementFormControlInput>();

}
}
}