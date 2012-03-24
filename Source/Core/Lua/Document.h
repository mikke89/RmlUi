#pragma once
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

template<> const char* GetTClassName<Document>();
template<> RegType<Document>* GetMethodTable<Document>();
template<> luaL_reg* GetAttrTable<Document>();
template<> luaL_reg* SetAttrTable<Document>();
}
}
}