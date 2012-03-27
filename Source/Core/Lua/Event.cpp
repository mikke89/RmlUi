#include "precompiled.h"
#include "LuaType.h"
#include "lua.hpp"
#include <Rocket/Core/Event.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/Dictionary.h>


namespace Rocket {
namespace Core {
namespace Lua {

//method
int EventStopPropagation(lua_State* L, Event* obj)
{
    LUACHECKOBJ(obj);
    obj->StopPropagation();
    return 0;
}

//getters
int EventGetAttrcurrent_element(lua_State* L)
{
    Event* evt = LuaType<Event>::check(L,1);
    LUACHECKOBJ(evt);
    Element* ele = evt->GetCurrentElement();
    LuaType<Element>::push(L,ele,false);
    return 1;
}

int EventGetAttrtype(lua_State* L)
{
    Event* evt = LuaType<Event>::check(L,1);
    LUACHECKOBJ(evt);
    String type = evt->GetType();
    lua_pushstring(L,type.CString());
    return 1;
}

int EventGetAttrtarget_element(lua_State* L)
{
    Event* evt = LuaType<Event>::check(L,1);
    LUACHECKOBJ(evt);
    Element* target = evt->GetTargetElement();
    LuaType<Element>::push(L,target,false);
    return 1;
}

int EventGetAttrparameters(lua_State* L)
{
    Event* evt = LuaType<Event>::check(L,1);
    LUACHECKOBJ(evt);
    const Dictionary* params = evt->GetParameters();
    int index;
    String key;
    Variant* value;

    lua_newtable(L);
    int tableindex = lua_gettop(L);
    while(params->Iterate(index,key,value))
    {
        lua_pushstring(L,key.CString());
        Variant::Type type = value->GetType();
        switch(type)
        {
        case Variant::BYTE:
        case Variant::CHAR:
        case Variant::INT:
            lua_pushinteger(L,*(int*)value);
            break;
        case Variant::FLOAT:
            lua_pushnumber(L,*(float*)value);
            break;
        case Variant::COLOURB:
            LuaType<Colourb>::push(L,(Colourb*)value,false);
            break;
        case Variant::COLOURF:
            LuaType<Colourf>::push(L,(Colourf*)value,false);
            break;
        case Variant::STRING:
            lua_pushstring(L,((String*)value)->CString());
            break;
        case Variant::VECTOR2:
            //according to Variant.inl, it is going to be a Vector2f
            LuaType<Vector2f>::push(L,((Vector2f*)value),false);
            break;
        case Variant::VOIDPTR:
            lua_pushlightuserdata(L,(void*)value);
            break;
        default:
            lua_pushnil(L);
            break;
        }
        lua_settable(L,tableindex);
    }
    return 1;
}



//setters
/*
//Apparently, the dictionary returned is constant, and shouldn't be modified.
//I am keeping this function in case that isn't the case
int EventSetAttrparameters(lua_State* L)
{
    Event* evt = LuaType<Event>::check(L,1);
    LUACHECKOBJ(evt);
    const Dictionary* params = evt->GetParameters();
    int valtype = lua_type(L,2);
    if(valtype == LUA_TTABLE) //if the user gives a table, then go through the table and set everything
    {
        lua_pushnil(L); //becauase lua_next pops a key from the stack first, we don't want to pop the table
        while(lua_next(L,2) != 0)
        {
            //[-1] is value, [-2] is key
            int type = lua_type(L,-1);
            const char* key = luaL_checkstring(L,-2); //key HAS to be a string, or things will go bad
            switch(type)
            {
		    case LUA_TNUMBER:
                params->Set(key,(float)lua_tonumber(L,-1));
                break;
		    case LUA_TBOOLEAN: 
                params->Set(key,CHECK_BOOL(L,-1));
                break;
		    case LUA_TSTRING:
                params->Set(key,luaL_checkstring(L,-1));
                break;
            case LUA_TUSERDATA:
            case LUA_TLIGHTUSERDATA:
                params->Set(key,lua_touserdata(L,-1));
                break;
            default:
                break;
            }
        }
    }
    else if(valtype == LUA_TNIL)
    {
        params->Clear();
    }
    return 0;
}
*/


RegType<Event> EventMethods[] =
{
    LUAMETHOD(Event,StopPropagation)
    { NULL, NULL },
};

luaL_reg EventGetters[] =
{
    LUAGETTER(Event,current_element)
    LUAGETTER(Event,type)
    LUAGETTER(Event,target_element)
    LUAGETTER(Event,parameters)
    { NULL, NULL },
};

luaL_reg EventSetters[] =
{
    //LUASETTER(Event,parameters)
    { NULL, NULL },
};


template<> const char* GetTClassName<Event>() { return "Event"; }
template<> RegType<Event>* GetMethodTable<Event>() { return EventMethods; }
template<> luaL_reg* GetAttrTable<Event>() { return EventGetters; }
template<> luaL_reg* SetAttrTable<Event>() { return EventSetters; }

}
}
}