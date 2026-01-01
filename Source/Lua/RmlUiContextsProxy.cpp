#include "RmlUiContextsProxy.h"
#include "Pairs.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>

namespace Rml {
namespace Lua {

template <>
void ExtraInit<RmlUiContextsProxy>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, RmlUiContextsProxy__index);
	lua_setfield(L, metatable_index, "__index");
	lua_pushcfunction(L, RmlUiContextsProxy__pairs);
	lua_setfield(L, metatable_index, "__pairs");
}

int RmlUiContextsProxy__index(lua_State* L)
{
	/*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
	int keytype = lua_type(L, 2);
	if (keytype == LUA_TSTRING || keytype == LUA_TNUMBER) // only valid key types
	{
		RmlUiContextsProxy* obj = LuaType<RmlUiContextsProxy>::check(L, 1);
		RMLUI_CHECK_OBJ(obj);
		if (keytype == LUA_TSTRING)
		{
			const char* key = lua_tostring(L, 2);
			LuaType<Context>::push(L, GetContext(key));
		}
		else
		{
			int key = (int)luaL_checkinteger(L, 2);
			LuaType<Context>::push(L, GetContext(key - 1));
		}
		return 1;
	}
	else
		return LuaType<RmlUiContextsProxy>::index(L);
}

int RmlUiContextsProxy__pairs(lua_State* L)
{
	return MakeIntPairs(L);
}

RegType<RmlUiContextsProxy> RmlUiContextsProxyMethods[] = {
	{nullptr, nullptr},
};
luaL_Reg RmlUiContextsProxyGetters[] = {
	{nullptr, nullptr},
};
luaL_Reg RmlUiContextsProxySetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(RmlUiContextsProxy)
} // namespace Lua
} // namespace Rml
