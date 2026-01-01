#pragma once

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
typedef ElementDocument Document;

template <>
void ExtraInit<Document>(lua_State* L, int metatable_index);

// methods
int DocumentPullToFront(lua_State* L, Document* obj);
int DocumentPushToBack(lua_State* L, Document* obj);
int DocumentShow(lua_State* L, Document* obj);
int DocumentHide(lua_State* L, Document* obj);
int DocumentClose(lua_State* L, Document* obj);
int DocumentCreateElement(lua_State* L, Document* obj);
int DocumentCreateTextNode(lua_State* L, Document* obj);

// getters
int DocumentGetAttrtitle(lua_State* L);
int DocumentGetAttrcontext(lua_State* L);

// setters
int DocumentSetAttrtitle(lua_State* L);

extern RegType<Document> DocumentMethods[];
extern luaL_Reg DocumentGetters[];
extern luaL_Reg DocumentSetters[];

RMLUI_LUATYPE_DECLARE(Document)
} // namespace Lua
} // namespace Rml
