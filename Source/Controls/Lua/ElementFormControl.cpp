#include "precompiled.h"
#include "ElementFormControl.h"
#include <Rocket/Controls/ElementFormControl.h>
#include <Rocket/Core/Element.h>

using Rocket::Controls::ElementFormControl;
namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element
template<> void LuaType<ElementFormControl>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

//getters
int ElementFormControlGetAttrdisabled(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    LUACHECKOBJ(efc);
    lua_pushboolean(L,efc->IsDisabled());
    return 1;
}

int ElementFormControlGetAttrname(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    LUACHECKOBJ(efc);
    lua_pushstring(L,efc->GetName().CString());
    return 1;
}

int ElementFormControlGetAttrvalue(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    LUACHECKOBJ(efc);
    lua_pushstring(L,efc->GetValue().CString());
    return 1;
}


//setters
int ElementFormControlSetAttrdisabled(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    LUACHECKOBJ(efc);
    efc->SetDisabled(CHECK_BOOL(L,2));
    return 0;
}

int ElementFormControlSetAttrname(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    LUACHECKOBJ(efc);
    const char* name = luaL_checkstring(L,2);
    efc->SetName(name);
    return 0;
}

int ElementFormControlSetAttrvalue(lua_State* L)
{
    ElementFormControl* efc = LuaType<ElementFormControl>::check(L,1);
    LUACHECKOBJ(efc);
    const char* value = luaL_checkstring(L,2);
    efc->SetValue(value);
    return 0;
}


RegType<ElementFormControl> ElementFormControlMethods[] = 
{
    { NULL, NULL },
};

luaL_reg ElementFormControlGetters[] = 
{
    LUAGETTER(ElementFormControl,disabled)
    LUAGETTER(ElementFormControl,name)
    LUAGETTER(ElementFormControl,value)
    { NULL, NULL },
};

luaL_reg ElementFormControlSetters[] = 
{
    LUASETTER(ElementFormControl,disabled)
    LUASETTER(ElementFormControl,name)
    LUASETTER(ElementFormControl,value)
    { NULL, NULL },
};


template<> const char* GetTClassName<ElementFormControl>() { return "ElementFormControl"; }
template<> RegType<ElementFormControl>* GetMethodTable<ElementFormControl>() { return ElementFormControlMethods; }
template<> luaL_reg* GetAttrTable<ElementFormControl>() { return ElementFormControlGetters; }
template<> luaL_reg* SetAttrTable<ElementFormControl>() { return ElementFormControlSetters; }

}
}
}