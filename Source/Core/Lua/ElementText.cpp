#include "precompiled.h"
#include "ElementText.h"

namespace Rocket {
namespace Core {
namespace Lua {

int ElementTextGetAttrtext(lua_State* L)
{
    ElementText* obj = LuaType<ElementText>::check(L, 1);
    LUACHECKOBJ(obj);
    lua_pushstring(L,obj->GetText().ToUTF8(String()).CString());
    return 1;
}

int ElementTextSetAttrtext(lua_State* L)
{
    ElementText* obj = LuaType<ElementText>::check(L, 1);
    LUACHECKOBJ(obj);
    const char* text = luaL_checkstring(L,2);
    obj->SetText(text);
    return 0;
}

RegType<ElementText> ElementTextMethods[] =
{
    { NULL, NULL },
};

luaL_reg ElementTextGetters[] =
{
    LUAGETTER(ElementText,text)
    { NULL, NULL },
};

luaL_reg ElementTextSetters[] =
{
    LUASETTER(ElementText,text)
    { NULL, NULL },
};


}
}
}