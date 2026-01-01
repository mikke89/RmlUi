#include "ElementChildNodesProxy.h"
#include "Element.h"
#include "Pairs.h"

namespace Rml {
namespace Lua {

template <>
void ExtraInit<ElementChildNodesProxy>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, ElementChildNodesProxy__index);
	lua_setfield(L, metatable_index, "__index");
	lua_pushcfunction(L, ElementChildNodesProxy__pairs);
	lua_setfield(L, metatable_index, "__pairs");
	lua_pushcfunction(L, ElementChildNodesProxy__len);
	lua_setfield(L, metatable_index, "__len");
}

int ElementChildNodesProxy__index(lua_State* L)
{
	/*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
	int keytype = lua_type(L, 2);
	if (keytype == LUA_TNUMBER) // only valid key types
	{
		ElementChildNodesProxy* obj = LuaType<ElementChildNodesProxy>::check(L, 1);
		RMLUI_CHECK_OBJ(obj);
		int key = (int)luaL_checkinteger(L, 2);
		Element* child = obj->owner->GetChild(key - 1);
		LuaType<Element>::push(L, child, false);
		return 1;
	}
	else
		return LuaType<ElementChildNodesProxy>::index(L);
}

int ElementChildNodesProxy__pairs(lua_State* L)
{
	return MakeIntPairs(L);
}

int ElementChildNodesProxy__len(lua_State* L)
{
	ElementChildNodesProxy* obj = LuaType<ElementChildNodesProxy>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->owner->GetNumChildren());
	;
	return 1;
}

RegType<ElementChildNodesProxy> ElementChildNodesProxyMethods[] = {
	{nullptr, nullptr},
};
luaL_Reg ElementChildNodesProxyGetters[] = {
	{nullptr, nullptr},
};
luaL_Reg ElementChildNodesProxySetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(ElementChildNodesProxy)
} // namespace Lua
} // namespace Rml
