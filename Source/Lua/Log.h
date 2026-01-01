#pragma once

#include <RmlUi/Core/Log.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

template <>
void ExtraInit<Log>(lua_State* L, int metatable_index);
int LogMessage(lua_State* L);

extern RegType<Log> LogMethods[];
extern luaL_Reg LogGetters[];
extern luaL_Reg LogSetters[];

RMLUI_LUATYPE_DECLARE(Log)
} // namespace Lua
} // namespace Rml
