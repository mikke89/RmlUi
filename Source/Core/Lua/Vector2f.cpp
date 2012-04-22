#include "precompiled.h"
#include "Vector2f.h"
#include <Rocket/Core/Vector2.h>

namespace Rocket {
namespace Core {
namespace Lua {



int Vector2fnew(lua_State* L)
{
    float x = (float)luaL_checknumber(L,1);
    float y = (float)luaL_checknumber(L,2);

    Vector2f* vect = new Vector2f(x,y);

    LuaType<Vector2f>::push(L,vect,true); //true means it will be deleted when it is garbage collected
    return 1;
}

int Vector2f__mul(lua_State* L)
{
    Vector2f* lhs = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(lhs);
    float rhs = (float)luaL_checknumber(L,2);

    Vector2f* res = new Vector2f(*lhs);
    (*res) *= rhs;

    LuaType<Vector2f>::push(L,res,true);
    return 1;
}

int Vector2f__div(lua_State* L)
{
    Vector2f* lhs = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(lhs);
    float rhs = (float)luaL_checknumber(L,2);

    Vector2f* res = new Vector2f(*lhs);
    (*res) /= rhs;

    LuaType<Vector2f>::push(L,res,true);
    return 1;
}

int Vector2f__add(lua_State* L)
{
    Vector2f* lhs = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(lhs);
    Vector2f* rhs = LuaType<Vector2f>::check(L,2);
    LUACHECKOBJ(rhs);

    Vector2f* res = new Vector2f(*lhs);
    (*res) += (*rhs);

    LuaType<Vector2f>::push(L,res,true);
    return 1;
}

int Vector2f__sub(lua_State* L)
{
    Vector2f* lhs = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(lhs);
    Vector2f* rhs = LuaType<Vector2f>::check(L,2);
    LUACHECKOBJ(rhs);

    Vector2f* res = new Vector2f(*lhs);
    (*res) -= (*rhs);

    LuaType<Vector2f>::push(L,res,true);
    return 1;
}

int Vector2f__eq(lua_State* L)
{
    Vector2f* lhs = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(lhs);
    Vector2f* rhs = LuaType<Vector2f>::check(L,2);
    LUACHECKOBJ(rhs);

    lua_pushboolean(L, (*lhs) == (*rhs) ? 1 : 0);
    return 1;
}


int Vector2fDotProduct(lua_State* L, Vector2f* obj)
{
    Vector2f* rhs = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(rhs);
    
    float res = obj->DotProduct(*rhs);

    lua_pushnumber(L,res);
    return 1;
}

int Vector2fNormalise(lua_State* L, Vector2f* obj)
{
    Vector2f* res = new Vector2f();
    (*res) = obj->Normalise();

    LuaType<Vector2f>::push(L,res,true);
    return 1;
}

int Vector2fRotate(lua_State* L, Vector2f* obj)
{
    float num = (float)luaL_checknumber(L,1);

    Vector2f* res = new Vector2f();
    (*res) = obj->Rotate(num);

    LuaType<Vector2f>::push(L,res,true);
    return 1;
}

int Vector2fGetAttrx(lua_State*L)
{
    Vector2f* self = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(self);

    lua_pushnumber(L,self->x);
    return 1;
}

int Vector2fGetAttry(lua_State*L)
{
    Vector2f* self = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(self);

    lua_pushnumber(L,self->y);
    return 1;
}

int Vector2fGetAttrmagnitude(lua_State*L)
{
    Vector2f* self = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(self);

    lua_pushnumber(L,self->Magnitude());
    return 1;
}

int Vector2fSetAttrx(lua_State*L)
{
    Vector2f* self = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(self);
    float value = (float)luaL_checknumber(L,2);

    self->x = value;
    return 0;
}

int Vector2fSetAttry(lua_State*L)
{
    Vector2f* self = LuaType<Vector2f>::check(L,1);
    LUACHECKOBJ(self);
    float value = (float)luaL_checknumber(L,2);

    self->y = value;
    return 0;
}



RegType<Vector2f> Vector2fMethods[] = 
{
    LUAMETHOD(Vector2f,DotProduct)
    LUAMETHOD(Vector2f,Normalise)
    LUAMETHOD(Vector2f,Rotate)
    { NULL, NULL },
};

luaL_reg Vector2fGetters[]= 
{
    LUAGETTER(Vector2f,x)
    LUAGETTER(Vector2f,y)
    LUAGETTER(Vector2f,magnitude)
    { NULL, NULL },
};

luaL_reg Vector2fSetters[]= 
{
    LUASETTER(Vector2f,x)
    LUASETTER(Vector2f,y)
    { NULL, NULL },
};

}
}
}