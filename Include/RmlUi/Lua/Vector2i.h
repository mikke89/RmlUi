#pragma once

#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Vector2i>(lua_State* L, int metatable_index);
int Vector2inew(lua_State* L);
int Vector2i__mul(lua_State* L);
int Vector2i__div(lua_State* L);
int Vector2i__add(lua_State* L);
int Vector2i__sub(lua_State* L);
int Vector2i__eq(lua_State* L);

// getters
int Vector2iGetAttrx(lua_State* L);
int Vector2iGetAttry(lua_State* L);
int Vector2iGetAttrmagnitude(lua_State* L);

// setters
int Vector2iSetAttrx(lua_State* L);
int Vector2iSetAttry(lua_State* L);

extern RegType<Vector2i> Vector2iMethods[];
extern luaL_Reg Vector2iGetters[];
extern luaL_Reg Vector2iSetters[];

RMLUI_LUATYPE_DECLARE(Vector2i)
} // namespace Lua
} // namespace Rml
