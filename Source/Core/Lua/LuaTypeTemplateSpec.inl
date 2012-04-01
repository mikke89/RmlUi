/*
    Because all of the template specializations have to be compiled in the same translation unit, they have to 
    exist in this file. It is included by LuaType.inl
*/
#include "precompiled.h"
#include <Rocket/Core/Core.h>
#include <Rocket/Controls/Controls.h>
#include "LuaType.h"
#include "Colourb.h"
#include "Colourf.h"
#include "Vector2f.h"
#include "Vector2i.h"
#include "ElementStyle.h"
#include "Log.h"
#include "Rocket.h"
#include "Element.h"
#include "Document.h"
#include "Event.h"
#include "Context.h"
#include "ElementForm.h"
#include "ElementFormControl.h"
#include "ElementFormControlDataSelect.h"
#include "ElementFormControlSelect.h"
#include "ElementFormControlInput.h"
#include "ElementFormControlTextArea.h"
#include "ElementDataGrid.h"
#include "ElementDataGridRow.h"
#include "ElementTabSet.h"



namespace Rocket {
namespace Core {
namespace Lua {


LUATYPEDEFINE(Colourb)
LUATYPEDEFINE(Colourf)
LUATYPEDEFINE(Vector2f)
LUATYPEDEFINE(Vector2i)
LUATYPEDEFINE(Log)
LUATYPEDEFINE(rocket)
LUATYPEDEFINE(Element)
LUATYPEDEFINE(Document)
LUATYPEDEFINE(ElementStyle)
LUATYPEDEFINE(Event)
LUATYPEDEFINE(Context)
LUATYPEDEFINE(ElementForm)
LUATYPEDEFINE(ElementFormControl)
LUATYPEDEFINE(ElementFormControlDataSelect)
LUATYPEDEFINE(ElementFormControlSelect)
LUATYPEDEFINE(ElementFormControlInput)
LUATYPEDEFINE(ElementFormControlTextArea)
LUATYPEDEFINE(ElementDataGrid)
LUATYPEDEFINE(ElementDataGridRow)
LUATYPEDEFINE(ElementTabSet)


template class LuaType<Colourb>;
template class LuaType<Colourf>;
template class LuaType<Vector2f>;
template class LuaType<Vector2i>;
template class LuaType<Log>;
template class LuaType<rocket>;
template class LuaType<Element>;
template class LuaType<Document>;
template class LuaType<ElementStyle>;
template class LuaType<Event>;
template class LuaType<Context>;
template class LuaType<ElementForm>;
template class LuaType<ElementFormControl>;
template class LuaType<ElementFormControlDataSelect>;
template class LuaType<ElementFormControlSelect>;
template class LuaType<ElementFormControlInput>;
template class LuaType<ElementFormControlTextArea>;
template class LuaType<ElementDataGrid>;
template class LuaType<ElementDataGridRow>;
template class LuaType<ElementTabSet>;



template<> void LuaType<rocket>::extra_init(lua_State* L, int metatable_index)
{
    //because of the way LuaType::Register is done, we know that the methods table is directly
    //before the metatable 
    int method_index = metatable_index - 1;

    lua_pushcfunction(L,rocketCreateContext);
    lua_setfield(L,method_index,"CreateContext");

    lua_pushcfunction(L,rocketLoadFontFace);
    lua_setfield(L,method_index,"LoadFontFace");

    lua_pushcfunction(L,rocketRegisterTag);
    lua_setfield(L,method_index,"RegisterTag");

    rocketEnumkey_identifier(L);
    lua_setfield(L,method_index,"key_identifier");

    lua_pushcfunction(L,rocketGetAttrcontexts);
    lua_setfield(L,method_index,"contexts");

    return;
}

/*
    
        Basic Types

*/

template<> void LuaType<Colourb>::extra_init(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,Colourb__call);
    lua_setfield(L,metatable_index,"__call");

    lua_pushcfunction(L,Colourb__eq);
    lua_setfield(L,metatable_index,"__eq");

    lua_pushcfunction(L,Colourb__add);
    lua_setfield(L,metatable_index,"__add");

    lua_pushcfunction(L,Colourb__mul);
    lua_setfield(L,metatable_index,"__mul");

    return;
}


template<> void LuaType<Colourf>::extra_init(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,Colourf__call);
    lua_setfield(L,metatable_index,"__call");

    lua_pushcfunction(L,Colourf__eq);
    lua_setfield(L,metatable_index,"__eq");

    return;
}


template<> 
void LuaType<Vector2f>::extra_init(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,Vector2f__call);
    lua_setfield(L,metatable_index,"__call");

    lua_pushcfunction(L,Vector2f__mul);
    lua_setfield(L,metatable_index,"__mul");

    lua_pushcfunction(L,Vector2f__div);
    lua_setfield(L,metatable_index,"__div");

    lua_pushcfunction(L,Vector2f__add);
    lua_setfield(L,metatable_index,"__add");

    lua_pushcfunction(L,Vector2f__sub);
    lua_setfield(L,metatable_index,"__sub");

    lua_pushcfunction(L,Vector2f__eq);
    lua_setfield(L,metatable_index,"__eq");

    //stack is in the same state as it was before it entered this function
    return;
}

template<> 
void LuaType<Vector2i>::extra_init(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,Vector2i__call);
    lua_setfield(L,metatable_index,"__call");

    lua_pushcfunction(L,Vector2i__mul);
    lua_setfield(L,metatable_index,"__mul");

    lua_pushcfunction(L,Vector2i__div);
    lua_setfield(L,metatable_index,"__div");

    lua_pushcfunction(L,Vector2i__add);
    lua_setfield(L,metatable_index,"__add");

    lua_pushcfunction(L,Vector2i__sub);
    lua_setfield(L,metatable_index,"__sub");

    lua_pushcfunction(L,Vector2i__eq);
    lua_setfield(L,metatable_index,"__eq");

    //stack is in the same state as it was before it entered this function
    return;
}


/*

    Elements

*/


template<> void LuaType<Document>::extra_init(lua_State* L, int metatable_index)
{
    //we will inherit from Element
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index - 1);
}


template<> void LuaType<ElementStyle>::extra_init(lua_State* L, int metatable_index)
{
    lua_pushcfunction(L,ElementStyle__index);
    lua_setfield(L,metatable_index,"__index");

    lua_pushcfunction(L,ElementStyle__newindex);
    lua_setfield(L,metatable_index,"__newindex");
}


template<> void LuaType<ElementForm>::extra_init(lua_State* L, int metatable_index)
{
    //inherit from Element
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementFormControl>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementFormControlDataSelect>::extra_init(lua_State* L, int metatable_index)
{
    //do whatever ElementFormControlSelect did as far as inheritance
    LuaType<ElementFormControlSelect>::extra_init(L,metatable_index);
    //then inherit from ElementFromControlSelect
    LuaType<ElementFormControlSelect>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementFormControlInput>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<ElementFormControl>::extra_init(L,metatable_index);
    LuaType<ElementFormControl>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementFormControlSelect>::extra_init(lua_State* L, int metatable_index)
{
    //init whatever elementformcontrol did extra, like inheritance
    LuaType<ElementFormControl>::extra_init(L,metatable_index);
    //then inherit from elementformcontrol
    LuaType<ElementFormControl>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementFormControlTextArea>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<ElementFormControl>::extra_init(L,metatable_index);
    LuaType<ElementFormControl>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementDataGrid>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementDataGridRow>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}

template<> void LuaType<ElementTabSet>::extra_init(lua_State* L, int metatable_index)
{
    LuaType<Element>::_regfunctions(L,metatable_index,metatable_index-1);
}


template<> void LuaType<Log>::extra_init(lua_State* L, int metatable_index)
{
    //due to they way that LuaType::Register is made, we know that the method table is at the index
    //directly below the metatable
    int method_index = metatable_index - 1;

    lua_pushcfunction(L,Log__call);
    lua_setfield(L,metatable_index, "__call");

    //construct the "logtype" table, so that we can use the Rocket::Core::Log::Type enum like Log.logtype.always in Lua for Log::LT_ALWAYS
    lua_newtable(L);
    int logtype = lua_gettop(L);
    lua_pushvalue(L,-1); //copy of the new table, so that the logtype index will stay valid
    lua_setfield(L,method_index,"logtype");

    lua_pushinteger(L,(int)Log::LT_ALWAYS);
    lua_setfield(L,logtype,"always");

    lua_pushinteger(L,(int)Log::LT_ERROR);
    lua_setfield(L,logtype,"error");

    lua_pushinteger(L,(int)Log::LT_WARNING);
    lua_setfield(L,logtype,"warning");

    lua_pushinteger(L,(int)Log::LT_INFO);
    lua_setfield(L,logtype,"info");

    lua_pushinteger(L,(int)Log::LT_DEBUG);
    lua_setfield(L,logtype,"debug");

    lua_pop(L,1); //pop the logtype table

    return;
}

}
}
}