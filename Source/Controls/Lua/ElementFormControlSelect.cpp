#include "precompiled.h"
#include "ElementFormControlSelect.h"
#include <Rocket/Controls/ElementFormControlSelect.h>
#include <Rocket/Controls/ElementFormControl.h>
#include <Rocket/Core/Element.h>

using Rocket::Controls::ElementFormControlSelect;
using Rocket::Controls::ElementFormControl;
namespace Rocket {
namespace Core {
namespace Lua {
//inherits from ElementFormControl which inherits from Element


//methods
int ElementFormControlSelectAdd(lua_State* L, ElementFormControlSelect* obj)
{
    const char* rml = luaL_checkstring(L,1);
    const char* value = luaL_checkstring(L,2);
    int before = -1; //default
    if(lua_gettop(L) >= 3)
        before = luaL_checkint(L,3);

    int index = obj->Add(rml,value,before);
    lua_pushinteger(L,index);
    return 1;
}

int ElementFormControlSelectRemove(lua_State* L, ElementFormControlSelect* obj)
{
    int index = luaL_checkint(L,1);
    obj->Remove(index);
    return 0;
}

int ElementFormControlSelectGetOption(lua_State* L, ElementFormControlSelect* obj)
{
    int index = luaL_checkint(L,1);
    Rocket::Controls::SelectOption* opt = obj->GetOption(index);
    lua_newtable(L);
    LuaType<Element>::push(L,opt->GetElement(),false);
    lua_setfield(L,-2,"element");
    lua_pushstring(L,opt->GetValue().CString());
    lua_setfield(L,-2,"value");
    return 1;
}


//getters
int ElementFormControlSelectGetAttroptions(lua_State* L)
{
    ElementFormControlSelect* obj = LuaType<ElementFormControlSelect>::check(L,1);
    LUACHECKOBJ(obj);
    int numOptions = obj->GetNumOptions();

    //local variables for the loop
    Rocket::Controls::SelectOption* opt; 
    Element* ele;
    String value;
    lua_newtable(L);
    int retindex = lua_gettop(L);
    for(int index = 0; index < numOptions; index++)
    {
        opt = obj->GetOption(index);
        if(opt == NULL) continue;
    
        ele = opt->GetElement();
        value = opt->GetValue();

        lua_newtable(L);
        LuaType<Element>::push(L,ele,false);
        lua_setfield(L,-2,"element");
        lua_pushstring(L,value.CString());
        lua_setfield(L,-2,"value");
        lua_rawseti(L,retindex,index); //sets the table that is being returned's 'index' to be the table with element and value
        lua_pop(L,1); //pops the table with element,value from the stack
    }
    return 1;
}

int ElementFormControlSelectGetAttrselection(lua_State* L)
{
    ElementFormControlSelect* obj = LuaType<ElementFormControlSelect>::check(L,1);
    LUACHECKOBJ(obj);

    int selection = obj->GetSelection();
    lua_pushinteger(L,selection);
    return 1;
}


//setter
int ElementFormControlSelectSetAttrselection(lua_State* L)
{
    ElementFormControlSelect* obj = LuaType<ElementFormControlSelect>::check(L,1);
    LUACHECKOBJ(obj);
    int selection = luaL_checkint(L,2);
    obj->SetSelection(selection);
    return 0;
}


RegType<ElementFormControlSelect> ElementFormControlSelectMethods[] =
{
    LUAMETHOD(ElementFormControlSelect,Add)
    LUAMETHOD(ElementFormControlSelect,Remove)
    LUAMETHOD(ElementFormControlSelect,GetOption)
    { NULL, NULL },
};

luaL_reg ElementFormControlSelectGetters[] =
{
    LUAGETTER(ElementFormControlSelect,options)
    LUAGETTER(ElementFormControlSelect,selection)
    { NULL, NULL },
};

luaL_reg ElementFormControlSelectSetters[] =
{
    LUASETTER(ElementFormControlSelect,selection)
    { NULL, NULL },
};

/*
template<> const char* GetTClassName<ElementFormControlSelect>() { return "ElementFormControlSelect"; }
template<> RegType<ElementFormControlSelect>* GetMethodTable<ElementFormControlSelect>() { return ElementFormControlSelectMethods; }
template<> luaL_reg* GetAttrTable<ElementFormControlSelect>() { return ElementFormControlSelectGetters; }
template<> luaL_reg* SetAttrTable<ElementFormControlSelect>() { return ElementFormControlSelectSetters; }
*/
}
}
}