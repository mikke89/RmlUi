#pragma once
/*
    Declares Vector2f in the Lua global namespace. It implements the below (examples using Lua syntax) :

    Vector2f(float,float) creates a new Vector2f, and gets deleted when Lua garbage collects
    
    everything after this will assume that you have a local variable named 'vect', declared something similar to
    local vect = Vector2f(3.5,2.3)
    operators (the types that it can operate on are on the right):
    vect * float
    vect / float
    vect + Vector2f
    vect - Vector2f
    vect == Vector2f

    methods:
    float var = vect:DotProduct(Vector2f)
    Vector2f var = vect:Normalise()
    Vector2f var = vect:Rotate(float)

    get and set attributes:
    vect.x
    vect.y
    
    get attributes:
    vect.magnitude

*/
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Types.h>

using Rocket::Core::Vector2f;
namespace Rocket {
namespace Core {
namespace Lua {
template<> void LuaType<Vector2f>::extra_init(lua_State* L, int metatable_index);
int Vector2fnew(lua_State* L);
int Vector2f__mul(lua_State* L);
int Vector2f__div(lua_State* L);
int Vector2f__add(lua_State* L);
int Vector2f__sub(lua_State* L);
int Vector2f__eq(lua_State* L);

int Vector2fDotProduct(lua_State* L, Vector2f* obj);
int Vector2fNormalise(lua_State* L, Vector2f* obj);
int Vector2fRotate(lua_State* L, Vector2f* obj);

int Vector2fGetAttrx(lua_State*L);
int Vector2fGetAttry(lua_State*L);
int Vector2fGetAttrmagnitude(lua_State*L);

int Vector2fSetAttrx(lua_State*L);
int Vector2fSetAttry(lua_State*L);


RegType<Vector2f> Vector2fMethods[];
luaL_reg Vector2fGetters[];
luaL_reg Vector2fSetters[];

/*
template<> const char* GetTClassName<Vector2f>() { return "Vector2f"; }
template<> RegType<Vector2f>* GetMethodTable<Vector2f>() { return Vector2fMethods; }
template<> luaL_reg* GetAttrTable<Vector2f>() { return Vector2fGetters; }
template<> luaL_reg* SetAttrTable<Vector2f>() { return Vector2fSetters; }
*/
}
}
}
