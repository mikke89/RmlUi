#pragma once
/*
    This defines the ElementFormControlTextArea type in the Lua global namespace, refered in this documentation by EFCTextArea

    It inherits from ElementFormControl,which inherits from Element

    All properties are read/write
    int EFCTextArea.cols
    int EFCTextArea.maxlength
    int EFCTextArea.rows
    bool EFCTextArea.wordwrap
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/ElementFormControlTextArea.h>

using Rocket::Controls::ElementFormControlTextArea;
namespace Rocket {
namespace Core {
namespace Lua {
//inherits from ElementFormControl which inherits from Element
template<> void LuaType<ElementFormControlTextArea>::extra_init(lua_State* L, int metatable_index);

//getters
int ElementFormControlTextAreaGetAttrcols(lua_State* L);
int ElementFormControlTextAreaGetAttrmaxlength(lua_State* L);
int ElementFormControlTextAreaGetAttrrows(lua_State* L);
int ElementFormControlTextAreaGetAttrwordwrap(lua_State* L);

//setters
int ElementFormControlTextAreaSetAttrcols(lua_State* L);
int ElementFormControlTextAreaSetAttrmaxlength(lua_State* L);
int ElementFormControlTextAreaSetAttrrows(lua_State* L);
int ElementFormControlTextAreaSetAttrwordwrap(lua_State* L);

RegType<ElementFormControlTextArea> ElementFormControlTextAreaMethods[];
luaL_reg ElementFormControlTextAreaGetters[];
luaL_reg ElementFormControlTextAreaSetters[];

/*
template<> const char* GetTClassName<ElementFormControlTextArea>();
template<> RegType<ElementFormControlTextArea>* GetMethodTable<ElementFormControlTextArea>();
template<> luaL_reg* GetAttrTable<ElementFormControlTextArea>();
template<> luaL_reg* SetAttrTable<ElementFormControlTextArea>();
*/
}
}
}