#include "precompiled.h"
#include "ElementForm.h"
#include <Rocket/Core/Element.h>
#include <Rocket/Controls/ElementForm.h>

namespace Rocket {
namespace Core {
namespace Lua {

template<> void LuaType<ElementForm>::extra_init(lua_State* L, int metatable_index)
{
    //inherit from Element
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

//method
int ElementFormSubmit(lua_State* L, ElementForm* obj)
{
    LUACHECKOBJ(obj);
    const char* name = luaL_checkstring(L,1);
    const char* value = luaL_checkstring(L,2);
    obj->Submit(name,value);
    return 0;
}

RegType<ElementForm> ElementFormMethods[] =
{
    LUAMETHOD(ElementForm,Submit)
    { NULL, NULL },
};

luaL_reg ElementFormGetters[] =
{
    { NULL, NULL },
};

luaL_reg ElementFormSetters[] =
{
    { NULL, NULL },
};

template<> const char* GetTClassName<ElementForm>() { return "ElementForm"; }
template<> RegType<ElementForm>* GetMethodTable<ElementForm>() { return ElementFormMethods; }
template<> luaL_reg* GetAttrTable<ElementForm>() { return ElementFormGetters; }
template<> luaL_reg* SetAttrTable<ElementForm>() { return ElementFormSetters; }

}
}
}