#include "precompiled.h"
#include "ElementFormControlTextArea.h"
#include <Rocket/Controls/ElementFormControl.h>

using Rocket::Controls::ElementFormControl;
namespace Rocket {
namespace Core {
namespace Lua {
//inherits from ElementFormControl which inherits from Element
template<> void LuaType<ElementFormControlTextArea>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<ElementFormControl>::extra_init(L,metatable_index);
    LuaType<ElementFormControl>::_regfunctions(L,metatable_index,metatable_index-1);
}

//getters
int ElementFormControlTextAreaGetAttrcols(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetNumColumns());
    return 1;
}

int ElementFormControlTextAreaGetAttrmaxlength(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetMaxLength());
    return 1;
}

int ElementFormControlTextAreaGetAttrrows(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushinteger(L,obj->GetNumRows());
    return 1;
}

int ElementFormControlTextAreaGetAttrwordwrap(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    lua_pushboolean(L,obj->GetWordWrap());
    return 1;
}


//setters
int ElementFormControlTextAreaSetAttrcols(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    int cols = luaL_checkint(L,2);
    obj->SetNumColumns(cols);
    return 0;
}

int ElementFormControlTextAreaSetAttrmaxlength(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    int ml = luaL_checkint(L,2);
    obj->SetMaxLength(ml);
    return 0;
}

int ElementFormControlTextAreaSetAttrrows(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    int rows = luaL_checkint(L,2);
    obj->SetNumRows(rows);
    return 0;
}

int ElementFormControlTextAreaSetAttrwordwrap(lua_State* L)
{
    ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L,1);
    LUACHECKOBJ(obj);
    bool ww = CHECK_BOOL(L,2);
    obj->SetWordWrap(ww);
    return 0;
}


RegType<ElementFormControlTextArea> ElementFormControlTextAreaMethods[] =
{
    { NULL, NULL },
};

luaL_reg ElementFormControlTextAreaGetters[] =
{
    LUAGETTER(ElementFormControlTextArea,cols)
    LUAGETTER(ElementFormControlTextArea,maxlength)
    LUAGETTER(ElementFormControlTextArea,rows)
    LUAGETTER(ElementFormControlTextArea,wordwrap)
    { NULL, NULL },
};

luaL_reg ElementFormControlTextAreaSetters[] =
{
    LUASETTER(ElementFormControlTextArea,cols)
    LUASETTER(ElementFormControlTextArea,maxlength)
    LUASETTER(ElementFormControlTextArea,rows)
    LUASETTER(ElementFormControlTextArea,wordwrap)
    { NULL, NULL },
};


template<> const char* GetTClassName<ElementFormControlTextArea>() { return "ElementFormControlTextArea"; }
template<> RegType<ElementFormControlTextArea>* GetMethodTable<ElementFormControlTextArea>() { return ElementFormControlTextAreaMethods; }
template<> luaL_reg* GetAttrTable<ElementFormControlTextArea>() { return ElementFormControlTextAreaGetters; }
template<> luaL_reg* SetAttrTable<ElementFormControlTextArea>() { return ElementFormControlTextAreaSetters; }

}
}
}