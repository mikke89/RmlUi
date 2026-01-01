#include "Log.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/StringUtilities.h>

namespace Rml {
namespace Lua {

template <>
void ExtraInit<Log>(lua_State* L, int metatable_index)
{
	// due to they way that LuaType::Register is made, we know that the method table is at the index
	// directly below the metatable
	int method_index = metatable_index - 1;

	lua_pushcfunction(L, LogMessage);
	lua_setfield(L, method_index, "Message");

	// construct the "logtype" table, so that we can use the Log::Type enum like Log.logtype.always in Lua for Log::LT_ALWAYS
	lua_newtable(L);
	int logtype = lua_gettop(L);
	lua_pushvalue(L, -1); // copy of the new table, so that the logtype index will stay valid
	lua_setfield(L, method_index, "logtype");

	lua_pushinteger(L, (int)Log::LT_ALWAYS);
	lua_setfield(L, logtype, "always");

	lua_pushinteger(L, (int)Log::LT_ERROR);
	lua_setfield(L, logtype, "error");

	lua_pushinteger(L, (int)Log::LT_WARNING);
	lua_setfield(L, logtype, "warning");

	lua_pushinteger(L, (int)Log::LT_INFO);
	lua_setfield(L, logtype, "info");

	lua_pushinteger(L, (int)Log::LT_DEBUG);
	lua_setfield(L, logtype, "debug");

	lua_pop(L, 1); // pop the logtype table
	return;
}

int LogMessage(lua_State* L)
{
	Log::Type type = Log::Type((int)luaL_checkinteger(L, 1));
	const char* str = luaL_checkstring(L, 2);

	Log::Message(type, "%s", str);
	return 0;
}

RegType<Log> LogMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg LogGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg LogSetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(Log)
} // namespace Lua
} // namespace Rml
