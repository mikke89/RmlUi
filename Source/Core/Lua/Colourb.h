#pragma once
/*
    Declares Colourb in the Lua global namespace. It implements the below (examples using Lua syntax) :

    Colourb(int red,int green,int blue,int alpha) creates a new Colourf (values must be bounded between 0 and 255 inclusive), 
    and gets deleted when Lua garbage collects
    
    everything after this will assume that you have a local variable named 'col', declared something similar to
    local col = Colourb(0,15,3,255)

    operators (the types that it can operate on are on the right):
    col == Colourb
    col + Colourb
    col * float

    no methods

    get and set attributes:
    col.red
    col.green
    col.blue
    col.alpha

    get attributes:
    local red,green,blue,alpha = col.rgba    
*/

#include "LuaType.h"
#include "lua.hpp"
#include <Rocket/Core/Types.h>

namespace Rocket {
namespace Core {
namespace Lua {

template<> void LuaType<Colourb>::extra_init(lua_State* L, int metatable_index);
int Colourb__call(lua_State* L);
int Colourb__eq(lua_State* L);
int Colourb__add(lua_State* L);
int Colourb__mul(lua_State* L);


//getters
int ColourbGetAttrred(lua_State* L);
int ColourbGetAttrgreen(lua_State* L);
int ColourbGetAttrblue(lua_State* L);
int ColourbGetAttralpha(lua_State* L);
int ColourbGetAttrrgba(lua_State* L);

//setters
int ColourbSetAttrred(lua_State* L);
int ColourbSetAttrgreen(lua_State* L);
int ColourbSetAttrblue(lua_State* L);
int ColourbSetAttralpha(lua_State* L);

RegType<Colourb> ColourbMethods[];
luaL_reg ColourbGetters[];
luaL_reg ColourbSetters[];

template<> const char* GetTClassName<Colourb>();
template<> RegType<Colourb>* GetMethodTable<Colourb>();
template<> luaL_reg* GetAttrTable<Colourb>();
template<> luaL_reg* SetAttrTable<Colourb>();

}
}
}