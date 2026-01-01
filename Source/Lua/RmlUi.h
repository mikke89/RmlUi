#pragma once

#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

class LuaRmlUi {
public:
	// reference to the table defined in LuaRmlUiEnumkey_identifier
	int key_identifier_ref;
	// reference to the table defined in LuaRmlUiEnumkey_modifier
	int key_modifier_ref;
};

void LuaRmlUiPushrmluiGlobal(lua_State* L);

template <>
void ExtraInit<LuaRmlUi>(lua_State* L, int metatable_index);
int LuaRmlUiCreateContext(lua_State* L, LuaRmlUi* obj);
int LuaRmlUiLoadFontFace(lua_State* L, LuaRmlUi* obj);
int LuaRmlUiRegisterTag(lua_State* L, LuaRmlUi* obj);

int LuaRmlUiGetAttrcontexts(lua_State* L);
int LuaRmlUiGetAttrkey_identifier(lua_State* L);
int LuaRmlUiGetAttrkey_modifier(lua_State* L);

void LuaRmlUiEnumkey_identifier(lua_State* L);
void LuaRmlUiEnumkey_modifier(lua_State* L);

extern RegType<LuaRmlUi> LuaRmlUiMethods[];
extern luaL_Reg LuaRmlUiGetters[];
extern luaL_Reg LuaRmlUiSetters[];

RMLUI_LUATYPE_DECLARE(LuaRmlUi)
} // namespace Lua
} // namespace Rml
