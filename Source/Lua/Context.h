#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Context>(lua_State* L, int metatable_index);

// methods
int ContextAddEventListener(lua_State* L, Context* obj);
int ContextCreateDocument(lua_State* L, Context* obj);
int ContextLoadDocument(lua_State* L, Context* obj);
int ContextRender(lua_State* L, Context* obj);
int ContextUnloadAllDocuments(lua_State* L, Context* obj);
int ContextUnloadDocument(lua_State* L, Context* obj);
int ContextUpdate(lua_State* L, Context* obj);

// getters
int ContextGetAttrdimensions(lua_State* L);
int ContextGetAttrdocuments(lua_State* L);
int ContextGetAttrdp_ratio(lua_State* L);
int ContextGetAttrfocus_element(lua_State* L);
int ContextGetAttrhover_element(lua_State* L);
int ContextGetAttrname(lua_State* L);
int ContextGetAttrroot_element(lua_State* L);

// setters
int ContextSetAttrdimensions(lua_State* L);
int ContextSetAttrdp_ratio(lua_State* L);

extern RegType<Context> ContextMethods[];
extern luaL_Reg ContextGetters[];
extern luaL_Reg ContextSetters[];

RMLUI_LUATYPE_DECLARE(Context)
} // namespace Lua
} // namespace Rml
