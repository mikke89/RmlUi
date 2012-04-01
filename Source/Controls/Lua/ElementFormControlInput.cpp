#include "precompiled.h"
#include "ElementFormControlInput.h"
#include <Rocket/Controls/ElementFormControl.h>

using Rocket::Controls::ElementFormControl;
namespace Rocket {
namespace Core {
namespace Lua {
//getters
int ElementFormControlInputGetAttrchecked(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushboolean(L,obj->HasAttribute("checked"));
    return 1;
}

int ElementFormControlInputGetAttrmaxlength(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetAttribute<int>("maxlength",-1));
    return 1;
}

int ElementFormControlInputGetAttrsize(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetAttribute<int>("size",20));
    return 1;
}

int ElementFormControlInputGetAttrmax(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetAttribute<int>("max",100));
    return 1;
}

int ElementFormControlInputGetAttrmin(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetAttribute<int>("min",0));
    return 1;
}

int ElementFormControlInputGetAttrstep(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetAttribute<int>("step",1));
    return 1;
}


//setters
int ElementFormControlInputSetAttrchecked(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    bool checked = CHECK_BOOL(L,2);
    if(checked)
        obj->SetAttribute("checked",true);
    else
        obj->RemoveAttribute("checked");
    return 0;
}

int ElementFormControlInputSetAttrmaxlength(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    int maxlength = luaL_checkint(L,2);
    obj->SetAttribute("maxlength",maxlength);
    return 0;
}

int ElementFormControlInputSetAttrsize(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    int size = luaL_checkint(L,2);
    obj->SetAttribute("size",size);
    return 0;
}

int ElementFormControlInputSetAttrmax(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    int max = luaL_checkint(L,2);
    obj->SetAttribute("max",max);
    return 0;
}

int ElementFormControlInputSetAttrmin(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    int min = luaL_checkint(L,2);
    obj->SetAttribute("min",min);
    return 0;
}

int ElementFormControlInputSetAttrstep(lua_State* L)
{
    ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L,1);
    LUACHECKOBJ(obj);
    int step = luaL_checkint(L,2);
    obj->SetAttribute("step",step);
    return 0;
}


RegType<ElementFormControlInput> ElementFormControlInputMethods[] = 
{
    {NULL,NULL},
};

luaL_reg ElementFormControlInputGetters[] = 
{
    LUAGETTER(ElementFormControlInput,checked)
    LUAGETTER(ElementFormControlInput,maxlength)
    LUAGETTER(ElementFormControlInput,size)
    LUAGETTER(ElementFormControlInput,max)
    LUAGETTER(ElementFormControlInput,min)
    LUAGETTER(ElementFormControlInput,step)
    {NULL,NULL},
};

luaL_reg ElementFormControlInputSetters[] = 
{
    LUASETTER(ElementFormControlInput,checked)
    LUASETTER(ElementFormControlInput,maxlength)
    LUASETTER(ElementFormControlInput,size)
    LUASETTER(ElementFormControlInput,max)
    LUASETTER(ElementFormControlInput,min)
    LUASETTER(ElementFormControlInput,step)
    {NULL,NULL},
};

/*
template<> const char* GetTClassName<ElementFormControlInput>() { return "ElementFormControlInput"; }
template<> RegType<ElementFormControlInput>* GetMethodTable<ElementFormControlInput>() { return ElementFormControlInputMethods; }
template<> luaL_reg* GetAttrTable<ElementFormControlInput>() { return ElementFormControlInputGetters; }
template<> luaL_reg* SetAttrTable<ElementFormControlInput>() { return ElementFormControlInputSetters; }
*/
}
}
}