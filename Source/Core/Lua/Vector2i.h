#pragma once
/*
    Declares Vector2i in the Lua global namespace. It implements the below (examples using Lua syntax) :

    Vector2i(int,int) creates a new Vector2i, and gets deleted when Lua garbage collects
    
    everything after this will assume that you have a local variable named 'vect', declared something similar to
    local vect = Vector2i(50,90)
    operators (the types that it can operate on are on the right):
    vect * int
    vect / int
    vect + Vector2i
    vect - Vector2i
    vect == Vector2i

    no methods

    get and set attributes:
    vect.x
    vect.y
*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Types.h>

using Rocket::Core::Vector2i;
namespace Rocket {
namespace Core {
namespace Lua {
template<> void LuaType<Vector2i>::extra_init(lua_State* L, int metatable_index);
int Vector2inew(lua_State* L);
int Vector2i__mul(lua_State* L);
int Vector2i__div(lua_State* L);
int Vector2i__add(lua_State* L);
int Vector2i__sub(lua_State* L);
int Vector2i__eq(lua_State* L);

//getters
int Vector2iGetAttrx(lua_State*L);
int Vector2iGetAttry(lua_State*L);
int Vector2iGetAttrmagnitude(lua_State*L);

//setters
int Vector2iSetAttrx(lua_State*L);
int Vector2iSetAttry(lua_State*L);


RegType<Vector2i> Vector2iMethods[];
luaL_reg Vector2iGetters[];
luaL_reg Vector2iSetters[];

/*
template<> const char* GetTClassName<Vector2i>() { return "Vector2i"; }
template<> RegType<Vector2i>* GetMethodTable<Vector2i>() { return Vector2iMethods; }
template<> luaL_reg* GetAttrTable<Vector2i>() { return Vector2iGetters; }
template<> luaL_reg* SetAttrTable<Vector2i>() { return Vector2iSetters; }
*/
}
}
}

