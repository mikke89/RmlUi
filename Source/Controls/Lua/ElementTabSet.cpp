#include "precompiled.h"
#include "ElementTabSet.h"
#include <Rocket/Core/Element.h>


namespace Rocket {
namespace Core {
namespace Lua {
//this will be used to "inherit" from Element

//methods
int ElementTabSetSetPanel(lua_State* L, ElementTabSet* obj)
{
    LUACHECKOBJ(obj);
    int index = luaL_checkint(L,1);
    const char* rml = luaL_checkstring(L,2);

    obj->SetPanel(index,rml);
    return 0;
}

int ElementTabSetSetTab(lua_State* L, ElementTabSet* obj)
{
    LUACHECKOBJ(obj);
    int index = luaL_checkint(L,1);
    const char* rml = luaL_checkstring(L,2);

    obj->SetTab(index,rml);
    return 0;
}


//getters
int ElementTabSetGetAttractive_tab(lua_State* L)
{
    ElementTabSet* obj = LuaType<ElementTabSet>::check(L,1);
    LUACHECKOBJ(obj);
    int tab = obj->GetActiveTab();
    lua_pushinteger(L,tab);
    return 1;
}

int ElementTabSetGetAttrnum_tabs(lua_State* L)
{
    ElementTabSet* obj = LuaType<ElementTabSet>::check(L,1);
    LUACHECKOBJ(obj);
    int num = obj->GetNumTabs();
    lua_pushinteger(L,num);
    return 1;
}


//setter
int ElementTabSetSetAttractive_tab(lua_State* L)
{
    ElementTabSet* obj = LuaType<ElementTabSet>::check(L,1);
    LUACHECKOBJ(obj);
    int tab = luaL_checkint(L,2);
    obj->SetActiveTab(tab);
    return 0;
}


RegType<ElementTabSet> ElementTabSetMethods[] =
{
    LUAMETHOD(ElementTabSet,SetPanel)
    LUAMETHOD(ElementTabSet,SetTab)
    { NULL, NULL },
};

luaL_reg ElementTabSetGetters[] =
{
    LUAGETTER(ElementTabSet,active_tab)
    LUAGETTER(ElementTabSet,num_tabs)
    { NULL, NULL },
};

luaL_reg ElementTabSetSetters[] =
{
    LUASETTER(ElementTabSet,active_tab)
    { NULL, NULL },
};

/*
template<> const char* GetTClassName<ElementTabSet>() { return "ElementTabSet"; }
template<> RegType<ElementTabSet>* GetMethodTable<ElementTabSet>() { return ElementTabSetMethods; }
template<> luaL_reg* GetAttrTable<ElementTabSet>() { return ElementTabSetGetters; }
template<> luaL_reg* SetAttrTable<ElementTabSet>() { return ElementTabSetSetters; }
*/

}
}
}