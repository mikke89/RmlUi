#pragma once
/*
    This defines the Document type in the Lua global namespace

    It inherits from Element, so check the documentation for Element.h to
    see what other functions you can call from a Document object. Document
    specific things are below

    //methods
    noreturn Document:PullToFront()
    noreturn Document:PushToBack()
    noreturn Document:Show(int flag)
    noreturn Document:Hide()
    noreturn Document:Close()
    Element Document:CreateElement(string tag)
    ElementText Document:CreateTextNode --NYI
    
    //getters
    string Document.title
    Context Document.context

    //setter
    Document.title = string
*/
#include "lua.hpp"
#include "LuaType.h"
#include <Rocket/Core/ElementDocument.h>


namespace Rocket {
namespace Core {
namespace Lua {
typedef ElementDocument Document;

template<> void LuaType<Document>::extra_init(lua_State* L, int metatable_index);

//methods
int DocumentPullToFront(lua_State* L, Document* obj);
int DocumentPushToBack(lua_State* L, Document* obj);
int DocumentShow(lua_State* L, Document* obj);
int DocumentHide(lua_State* L, Document* obj);
int DocumentClose(lua_State* L, Document* obj);
int DocumentCreateElement(lua_State* L, Document* obj);
int DocumentCreateTextNode(lua_State* L, Document* obj);

//getters
int DocumentGetAttrtitle(lua_State* L);
int DocumentGetAttrcontext(lua_State* L);

//setters
int DocumentSetAttrtitle(lua_State* L);

RegType<Document> DocumentMethods[];
luaL_reg DocumentGetters[];
luaL_reg DocumentSetters[];

/*
template<> const char* GetTClassName<Document>();
template<> RegType<Document>* GetMethodTable<Document>();
template<> luaL_reg* GetAttrTable<Document>();
template<> luaL_reg* SetAttrTable<Document>();
*/
}
}
}