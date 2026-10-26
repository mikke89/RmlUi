#include "GlobalLuaFunctions.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {
/*
Below here are global functions and their helper functions that help overwrite the Lua global functions
*/

// Based off of the luaB_print function from Lua's lbaselib.c
int LuaPrint(lua_State* L)
{
	int n = lua_gettop(L); /* number of arguments */
	int i;
	lua_getglobal(L, "tostring");
	StringList string_list = StringList();
	String output = "";
	for (i = 1; i <= n; i++)
	{
		const char* s;
		lua_pushvalue(L, -1);    /* function to be called */
		lua_pushvalue(L, i);     /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1); /* get result */
		if (s == nullptr)
			return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1)
			output += "\t";
		output += String(s);
		lua_pop(L, 1); /* pop result */
	}
	output += "\n";
	Log::Message(Log::LT_INFO, "%s", output.c_str());
	return 0;
}

#if LUA_VERSION_NUM < 502
// Lua 5.1 and LuaJIT ignore the __pairs and __ipairs metamethods, which the plugin's objects rely on to be iterable.
// Honor them like Lua 5.2 does, and otherwise call the original function, stored as the first upvalue.
static int CallPairsMetamethodOrOriginal(lua_State* L, const char* metamethod)
{
	if (luaL_getmetafield(L, 1, metamethod))
	{
		lua_pushvalue(L, 1);
		lua_call(L, 1, 3);
		return 3;
	}

	lua_pushvalue(L, lua_upvalueindex(1));
	lua_insert(L, 1);
	lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
	return lua_gettop(L);
}

static int LuaPairs(lua_State* L)
{
	return CallPairsMetamethodOrOriginal(L, "__pairs");
}

static int LuaIpairs(lua_State* L)
{
	return CallPairsMetamethodOrOriginal(L, "__ipairs");
}

static void WrapPairsFunction(lua_State* L, const char* name, lua_CFunction wrapper)
{
	lua_getfield(L, -1, name);
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushcclosure(L, wrapper, 1);
	lua_setfield(L, -2, name);
}
#endif

void OverrideLuaGlobalFunctions(lua_State* L)
{
	lua_getglobal(L, "_G");

	lua_pushcfunction(L, LuaPrint);
	lua_setfield(L, -2, "print");

#if LUA_VERSION_NUM < 502
	WrapPairsFunction(L, "pairs", LuaPairs);
	WrapPairsFunction(L, "ipairs", LuaIpairs);
#endif

	lua_pop(L, 1); // pop _G
}

} // namespace Lua
} // namespace Rml
