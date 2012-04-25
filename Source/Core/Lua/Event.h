#pragma once
/*
    This defines an Event type name in the global Lua namespace

    Generally, you won't create an event yourself, just receive it.

    //method
    noreturn Event:StopPropagation()
    
    //getters
    Element Event.current_element
    string Event.type
    Element Event.target_element
    {}key=string,value=int,float,Colourb/f,string,Vector2f,userdata Event.parameters
*/

#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>

namespace Rocket {
namespace Core {
namespace Lua {
template<> bool LuaType<Event>::is_reference_counted();

//method
int EventStopPropagation(lua_State* L, Event* obj);

//getters
int EventGetAttrcurrent_element(lua_State* L);
int EventGetAttrtype(lua_State* L);
int EventGetAttrtarget_element(lua_State* L);
int EventGetAttrparameters(lua_State* L);

//setters
//int EventSetAttrparameters(lua_State* L);

RegType<Event> EventMethods[];
luaL_reg EventGetters[];
luaL_reg EventSetters[];

/*
template<> const char* GetTClassName<Event>();
template<> RegType<Event>* GetMethodTable<Event>();
template<> luaL_reg* GetAttrTable<Event>();
template<> luaL_reg* SetAttrTable<Event>();
*/
}
}
}