#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// where owner is the context that we should look information from
struct ContextDocumentsProxy {
	Context* owner;
};

template <>
void ExtraInit<ContextDocumentsProxy>(lua_State* L, int metatable_index);
int ContextDocumentsProxy__index(lua_State* L);
int ContextDocumentsProxy__pairs(lua_State* L);

extern RegType<ContextDocumentsProxy> ContextDocumentsProxyMethods[];
extern luaL_Reg ContextDocumentsProxyGetters[];
extern luaL_Reg ContextDocumentsProxySetters[];

RMLUI_LUATYPE_DECLARE(ContextDocumentsProxy)
} // namespace Lua
} // namespace Rml
