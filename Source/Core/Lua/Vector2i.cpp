#include "precompiled.h"
#include "Vector2i.h"


namespace Rocket {
namespace Core {
namespace Lua {


int Vector2i__call(lua_State* L)
{
    int x = luaL_checkint(L,1);
    int y = luaL_checkint(L,2);

    Vector2i* vect = new Vector2i(x,y);

    LuaType<Vector2i>::push(L,vect,true); //true means it will be deleted when it is garbage collected
    return 1;
}

int Vector2i__mul(lua_State* L)
{
    Vector2i* lhs = LuaType<Vector2i>::check(L,1);
    int rhs = luaL_checkint(L,2);

    Vector2i* res = new Vector2i(*lhs);
    (*res) *= rhs;

    LuaType<Vector2i>::push(L,res,true);
    return 1;
}

int Vector2i__div(lua_State* L)
{
    Vector2i* lhs = LuaType<Vector2i>::check(L,1);
    int rhs = luaL_checkint(L,2);

    Vector2i* res = new Vector2i(*lhs);
    (*res) /= rhs;

    LuaType<Vector2i>::push(L,res,true);
    return 1;
}

int Vector2i__add(lua_State* L)
{
    Vector2i* lhs = LuaType<Vector2i>::check(L,1);
    Vector2i* rhs = LuaType<Vector2i>::check(L,1);

    Vector2i* res = new Vector2i(*lhs);
    (*res) += (*rhs);

    LuaType<Vector2i>::push(L,res,true);
    return 1;
}

int Vector2i__sub(lua_State* L)
{
    Vector2i* lhs = LuaType<Vector2i>::check(L,1);
    Vector2i* rhs = LuaType<Vector2i>::check(L,1);

    Vector2i* res = new Vector2i(*lhs);
    (*res) -= (*rhs);

    LuaType<Vector2i>::push(L,res,true);
    return 1;
}

int Vector2i__eq(lua_State* L)
{
    Vector2i* lhs = LuaType<Vector2i>::check(L,1);
    Vector2i* rhs = LuaType<Vector2i>::check(L,1);

    lua_pushboolean(L, (*lhs) == (*rhs) ? 1 : 0);
    return 1;
}

int Vector2iGetAttrx(lua_State*L)
{
    Vector2i* self = LuaType<Vector2i>::check(L,1);

    lua_pushinteger(L,self->x);
    return 1;
}

int Vector2iGetAttry(lua_State*L)
{
    Vector2i* self = LuaType<Vector2i>::check(L,1);

    lua_pushinteger(L,self->y);
    return 1;
}

int Vector2iGetAttrmagnitude(lua_State*L)
{
    Vector2i* self = LuaType<Vector2i>::check(L,1);

    lua_pushnumber(L,self->Magnitude());
    return 1;
}

int Vector2iSetAttrx(lua_State*L)
{
    Vector2i* self = LuaType<Vector2i>::check(L,1);
    int value = luaL_checkint(L,2);

    self->x = value;
    return 0;
}

int Vector2iSetAttry(lua_State*L)
{
    Vector2i* self = LuaType<Vector2i>::check(L,1);
    int value = luaL_checkint(L,2);

    self->y = value;
    return 0;
}



RegType<Vector2i> Vector2iMethods[] = 
{
    { NULL, NULL },
};

luaL_reg Vector2iGetters[]= 
{
    LUAGETTER(Vector2i,x)
    LUAGETTER(Vector2i,y)
    LUAGETTER(Vector2i,magnitude)
    { NULL, NULL },
};

luaL_reg Vector2iSetters[]= 
{
    LUASETTER(Vector2i,x)
    LUASETTER(Vector2i,y)
    { NULL, NULL },
};

}
}
}