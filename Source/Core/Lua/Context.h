#pragma once
#include "LuaType.h"
#include "lua.hpp"


namespace Rocket {
namespace Core {
namespace Lua {
class Rocket::Core::Context;

//methods
int ContextAddEventListener(lua_State* L, Context* obj);
int ContextAddMouseCursor(lua_State* L, Context* obj);
int ContextCreateDocument(lua_State* L, Context* obj);
int ContextLoadDocument(lua_State* L, Context* obj);
int ContextLoadMouseCursor(lua_State* L, Context* obj);
int ContextRender(lua_State* L, Context* obj);
int ContextShowMouseCursor(lua_State* L, Context* obj);
int ContextUnloadAllDocuments(lua_State* L, Context* obj);
int ContextUnloadAllMouseCursors(lua_State* L, Context* obj);
int ContextUnloadDocument(lua_State* L, Context* obj);
int ContextUnloadMouseCursor(lua_State* L, Context* obj);
int ContextUpdate(lua_State* L, Context* obj);

//getters
int ContextGetAttrdimensions(lua_State* L);
int ContextGetAttrdocuments(lua_State* L);
int ContextGetAttrfocus_element(lua_State* L);
int ContextGetAttrhover_element(lua_State* L);
int ContextGetAttrname(lua_State* L);
int ContextGetAttrroot_element(lua_State* L);

//setters
int ContextSetAttrdimensions(lua_State* L);


RegType<Context> ContextMethods[];
luaL_reg ContextGetters[];
luaL_reg ContextSetters[];

template<> const char* GetTClassName<Context>();
template<> RegType<Context>* GetMethodTable<Context>();
template<> luaL_reg* GetAttrTable<Context>();
template<> luaL_reg* SetAttrTable<Context>();

}
}
}