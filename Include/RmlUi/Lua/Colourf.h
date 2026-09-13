#pragma once

#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Colourf>(lua_State* L, int metatable_index);
// metamethods
int Colourfnew(lua_State* L);
int Colourf__eq(lua_State* L);

// getters
int ColourfGetAttrred(lua_State* L);
int ColourfGetAttrgreen(lua_State* L);
int ColourfGetAttrblue(lua_State* L);
int ColourfGetAttralpha(lua_State* L);
int ColourfGetAttrrgba(lua_State* L);

// setters
int ColourfSetAttrred(lua_State* L);
int ColourfSetAttrgreen(lua_State* L);
int ColourfSetAttrblue(lua_State* L);
int ColourfSetAttralpha(lua_State* L);
int ColourfSetAttrrgba(lua_State* L);

extern RegType<Colourf> ColourfMethods[];
extern luaL_Reg ColourfGetters[];
extern luaL_Reg ColourfSetters[];

RMLUI_LUATYPE_DECLARE(Colourf)
} // namespace Lua
} // namespace Rml
