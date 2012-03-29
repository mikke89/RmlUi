#include "precompiled.h"
#include "ElementFormControlDataSelect.h"
#include <Rocket/Controls/ElementFormControlSelect.h>

using Rocket::Controls::ElementFormControlSelect;
namespace Rocket {
namespace Core {
namespace Lua {
//inherits from ElementFormControl which inherits from Element
template<> void LuaType<ElementFormControlDataSelect>::extra_init(lua_State* L, int metatable_index)
{
    //do whatever ElementFormControlSelect did as far as inheritance
    LuaType<ElementFormControlSelect>::extra_init(L,metatable_index);
    //then inherit from ElementFromControlSelect
    LuaType<ElementFormControlSelect>::_regfunctions(L,metatable_index,metatable_index-1);
}

//method
int ElementFormControlDataSelectSetDataSource(lua_State* L, ElementFormControlDataSelect* obj)
{
    LUACHECKOBJ(obj);
    const char* source = luaL_checkstring(L,1);
    obj->SetDataSource(source);
    return 0;
}

RegType<ElementFormControlDataSelect> ElementFormControlDataSelectMethods[] =
{
    LUAMETHOD(ElementFormControlDataSelect,SetDataSource)
    { NULL, NULL },
};

luaL_reg ElementFormControlDataSelectGetters[] =
{
    { NULL, NULL },
};

luaL_reg ElementFormControlDataSelectSetters[] =
{
    { NULL, NULL },
};


template<> const char* GetTClassName<ElementFormControlDataSelect>() { return "ElementFormControlDataSelect"; }
template<> RegType<ElementFormControlDataSelect>* GetMethodTable<ElementFormControlDataSelect>() { return ElementFormControlDataSelectMethods; }
template<> luaL_reg* GetAttrTable<ElementFormControlDataSelect>() { return ElementFormControlDataSelectGetters; }
template<> luaL_reg* SetAttrTable<ElementFormControlDataSelect>() { return ElementFormControlDataSelectSetters; }

}
}
}