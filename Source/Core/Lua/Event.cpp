#include "precompiled.h"
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Event.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/Dictionary.h>
#include "EventParametersProxy.h"


namespace Rocket {
namespace Core {
namespace Lua {

//method
int EventStopPropagation(lua_State* L, Event* obj)
{
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
    EventParametersProxy* proxy = new EventParametersProxy();
    proxy->owner = evt;
    LuaType<EventParametersProxy>::push(L,proxy,true);
    return 1;
}

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
    { NULL, NULL },
};

/*
template<> const char* GetTClassName<Event>() { return "Event"; }
template<> RegType<Event>* GetMethodTable<Event>() { return EventMethods; }
template<> luaL_reg* GetAttrTable<Event>() { return EventGetters; }
template<> luaL_reg* SetAttrTable<Event>() { return EventSetters; }
*/
}
}
}