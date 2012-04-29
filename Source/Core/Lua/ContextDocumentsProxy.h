#pragma once
/*
    A proxy table with key of string and int and a value of Document
    Read only
*/
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Context.h>

namespace Rocket {
namespace Core {
namespace Lua {
//where owner is the context that we should look information from
struct ContextDocumentsProxy { Context* owner;  };

template<> void LuaType<ContextDocumentsProxy>::extra_init(lua_State* L, int metatable_index);
int ContextDocumentsProxy__index(lua_State* L);

//method
int ContextDocumentsProxyGetTable(lua_State* L, ContextDocumentsProxy* obj);

RegType<ContextDocumentsProxy> ContextDocumentsProxyMethods[];
luaL_reg ContextDocumentsProxyGetters[];
luaL_reg ContextDocumentsProxySetters[];
}
}
}