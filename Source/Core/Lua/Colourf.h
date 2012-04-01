#pragma once
/*
    Declares Colourf in the Lua global namespace. It implements the below (examples using Lua syntax) :

    Colourf(float red,float green,float blue,float alpha) creates a new Colourf (values must be bounded between 0 and 1 inclusive), 
    and gets deleted when Lua garbage collects
    
    everything after this will assume that you have a local variable named 'col', declared something similar to
    local col = Colourf(0.1,0.5,0.25,1.0)

    operators (the types that it can operate on are on the right):
    col == Colourf

    no methods

    get and set attributes:
    col.red
    col.green
    col.blue
    col.alpha

    get attributes:
    local red,green,blue,alpha = col.rgba    
*/

#include "lua.hpp"
#include "LuaType.h"
#include <Rocket/Core/Types.h>

using Rocket::Core::Colourf;
namespace Rocket {
namespace Core {
namespace Lua {
template<> void LuaType<Colourf>::extra_init(lua_State* L, int metatable_index);
//metamethods
int Colourf__call(lua_State* L);
int Colourf__eq(lua_State* L);

//getters
int ColourfGetAttrred(lua_State* L);
int ColourfGetAttrgreen(lua_State* L);
int ColourfGetAttrblue(lua_State* L);
int ColourfGetAttralpha(lua_State* L);
int ColourfGetAttrrgba(lua_State* L);

//setters
int ColourfSetAttrred(lua_State* L);
int ColourfSetAttrgreen(lua_State* L);
int ColourfSetAttrblue(lua_State* L);
int ColourfSetAttralpha(lua_State* L);

RegType<Colourf> ColourfMethods[];
luaL_reg ColourfGetters[];
luaL_reg ColourfSetters[];

/*
template<> const char* GetTClassName<Colourf>() { return "Colourf"; }
template<> RegType<Colourf>* GetMethodTable<Colourf>() { return ColourfMethods; }
template<> luaL_reg* GetAttrTable<Colourf>() { return ColourfGetters; }
template<> luaL_reg* SetAttrTable<Colourf>() { return ColourfSetters; }
*/
}
}
}


