#pragma once

#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Colourb>(lua_State* L, int metatable_index);
int Colourbnew(lua_State* L);
int Colourb__eq(lua_State* L);
int Colourb__add(lua_State* L);
int Colourb__mul(lua_State* L);

// getters
int ColourbGetAttrred(lua_State* L);
int ColourbGetAttrgreen(lua_State* L);
int ColourbGetAttrblue(lua_State* L);
int ColourbGetAttralpha(lua_State* L);
int ColourbGetAttrrgba(lua_State* L);

// setters
int ColourbSetAttrred(lua_State* L);
int ColourbSetAttrgreen(lua_State* L);
int ColourbSetAttrblue(lua_State* L);
int ColourbSetAttralpha(lua_State* L);
int ColourbSetAttrrgba(lua_State* L);

extern RegType<Colourb> ColourbMethods[];
extern luaL_Reg ColourbGetters[];
extern luaL_Reg ColourbSetters[];

RMLUI_LUATYPE_DECLARE(Colourb)
} // namespace Lua
} // namespace Rml
