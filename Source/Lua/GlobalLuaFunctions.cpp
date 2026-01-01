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

void OverrideLuaGlobalFunctions(lua_State* L)
{
	lua_getglobal(L, "_G");

	lua_pushcfunction(L, LuaPrint);
	lua_setfield(L, -2, "print");

	lua_pop(L, 1); // pop _G
}

} // namespace Lua
} // namespace Rml
