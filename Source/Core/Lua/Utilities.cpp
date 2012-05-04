#include "precompiled.h"
#include "Utilities.h"

namespace Rocket {
namespace Core {
namespace Lua {

void PushVariant(lua_State* L, Variant* var)
{
    if(var == NULL)
    {
        lua_pushnil(L);
        return;
    }
    Variant::Type vartype = var->GetType();
    switch(vartype)
    {
    case Variant::BYTE:
    case Variant::CHAR:
    case Variant::INT:
        lua_pushinteger(L,var->Get<int>());
        break;
    case Variant::FLOAT:
        lua_pushnumber(L,var->Get<float>());
        break;
    case Variant::COLOURB:
        LuaType<Colourb>::push(L,new Colourb(var->Get<Colourb>()),true);
        break;
    case Variant::COLOURF:
        LuaType<Colourf>::push(L,new Colourf(var->Get<Colourf>()),true);
        break;
    case Variant::STRING:
        lua_pushstring(L,var->Get<String>().CString());
        break;
    case Variant::VECTOR2:
        //according to Variant.inl, it is going to be a Vector2f
        LuaType<Vector2f>::push(L,new Vector2f(var->Get<Vector2f>()),true);
        break;
    case Variant::VOIDPTR:
        lua_pushlightuserdata(L,var->Get<void*>());
        break;
    default:
        lua_pushnil(L);
        break;
    }
}

}
}
}
