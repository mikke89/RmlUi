#include "SelectOptionsProxy.h"
#include "../Pairs.h"
#include <RmlUi/Core/Element.h>

namespace Rml {
namespace Lua {

int SelectOptionsProxy__index(lua_State* L)
{
	/*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
	int keytype = lua_type(L, 2);
	if (keytype == LUA_TNUMBER) // only valid key types
	{
		SelectOptionsProxy* proxy = LuaType<SelectOptionsProxy>::check(L, 1);
		RMLUI_CHECK_OBJ(proxy);
		int index = (int)luaL_checkinteger(L, 2);
		Element* opt = proxy->owner->GetOption(index - 1);
		RMLUI_CHECK_OBJ(opt);
		lua_newtable(L);
		LuaType<Element>::push(L, opt, false);
		lua_setfield(L, -2, "element");
		lua_pushstring(L, opt->GetAttribute("value", String()).c_str());
		lua_setfield(L, -2, "value");
		return 1;
	}
	else
		return LuaType<SelectOptionsProxy>::index(L);
}

int SelectOptionsProxy__pairs(lua_State* L)
{
	return MakeIntPairs(L);
}

RegType<SelectOptionsProxy> SelectOptionsProxyMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg SelectOptionsProxyGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg SelectOptionsProxySetters[] = {
	{nullptr, nullptr},
};

template <>
void ExtraInit<SelectOptionsProxy>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, SelectOptionsProxy__index);
	lua_setfield(L, metatable_index, "__index");
	lua_pushcfunction(L, SelectOptionsProxy__pairs);
	lua_setfield(L, metatable_index, "__pairs");
}

RMLUI_LUATYPE_DEFINE(SelectOptionsProxy)
} // namespace Lua
} // namespace Rml
