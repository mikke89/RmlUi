#pragma once

#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<Vector2f>(lua_State* L, int metatable_index);

int Vector2fnew(lua_State* L);
int Vector2f__mul(lua_State* L);
int Vector2f__div(lua_State* L);
int Vector2f__add(lua_State* L);
int Vector2f__sub(lua_State* L);
int Vector2f__eq(lua_State* L);

int Vector2fDotProduct(lua_State* L, Vector2f* obj);
int Vector2fNormalise(lua_State* L, Vector2f* obj);
int Vector2fRotate(lua_State* L, Vector2f* obj);

int Vector2fGetAttrx(lua_State* L);
int Vector2fGetAttry(lua_State* L);
int Vector2fGetAttrmagnitude(lua_State* L);

int Vector2fSetAttrx(lua_State* L);
int Vector2fSetAttry(lua_State* L);

extern RegType<Vector2f> Vector2fMethods[];
extern luaL_Reg Vector2fGetters[];
extern luaL_Reg Vector2fSetters[];

RMLUI_LUATYPE_DECLARE(Vector2f)
} // namespace Lua
} // namespace Rml
