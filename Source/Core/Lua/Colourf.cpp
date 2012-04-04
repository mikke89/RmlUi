#include "precompiled.h"
#include "Colourf.h"


namespace Rocket {
namespace Core {
namespace Lua {

//metamethods


int Colourfnew(lua_State* L)
{
    float red = (float)luaL_checknumber(L,1);
    float green = (float)luaL_checknumber(L,2);
    float blue =  (float)luaL_checknumber(L,3);
    float alpha = (float)luaL_checknumber(L,4);

    Colourf* col = new Colourf(red,green,blue,alpha);

    LuaType<Colourf>::push(L,col,true);
    return 1;
}

int Colourf__eq(lua_State* L)
{
    Colourf* lhs = LuaType<Colourf>::check(L,1);
    Colourf* rhs = LuaType<Colourf>::check(L,2);

    lua_pushboolean(L, (*lhs) == (*rhs) ? 1 : 0);
    return 1;
}


//getters
int ColourfGetAttrred(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    lua_pushnumber(L,obj->red);
    return 1;
}

int ColourfGetAttrgreen(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    lua_pushnumber(L,obj->green);
    return 1;
}

int ColourfGetAttrblue(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    lua_pushnumber(L,obj->blue);
    return 1;
}

int ColourfGetAttralpha(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    lua_pushnumber(L,obj->alpha);
    return 1;
}

int ColourfGetAttrrgba(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    lua_pushnumber(L,obj->red);
    lua_pushnumber(L,obj->green);
    lua_pushnumber(L,obj->blue);
    lua_pushnumber(L,obj->alpha);
    return 4;
}


//setters
int ColourfSetAttrred(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    float red = (float)luaL_checknumber(L,2);
    obj->red = red;
    return 0;
}

int ColourfSetAttrgreen(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    float green = (float)luaL_checknumber(L,2);
    obj->green = green;
    return 0;
}

int ColourfSetAttrblue(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    float blue = (float)luaL_checknumber(L,2);
    obj->blue;
    return 0;
}

int ColourfSetAttralpha(lua_State* L)
{
    Colourf* obj = LuaType<Colourf>::check(L,1);
    float alpha = (float)luaL_checknumber(L,2);
    obj->alpha;
    return 0;
}


RegType<Colourf> ColourfMethods[] =
{
    { NULL, NULL },
};

luaL_reg ColourfGetters[] =
{
    LUAGETTER(Colourf,red)
    LUAGETTER(Colourf,green)
    LUAGETTER(Colourf,blue)
    LUAGETTER(Colourf,alpha)
    LUAGETTER(Colourf,rgba)
    { NULL, NULL },
};

luaL_reg ColourfSetters[] =
{
    LUASETTER(Colourf,red)
    LUASETTER(Colourf,green)
    LUASETTER(Colourf,blue)
    LUASETTER(Colourf,alpha)
    { NULL, NULL },
};




}
}
}