#include "ElementInstancer.h"

namespace Rml {
namespace Lua {
template <>
void ExtraInit<ElementInstancer>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, ElementInstancernew);
	lua_setfield(L, metatable_index - 1, "new");
}

// method
int ElementInstancernew(lua_State* L)
{
	LuaElementInstancer* lei = new LuaElementInstancer(L);
	LuaType<ElementInstancer>::push(L, lei, true);
	return 1;
}

// setter
int ElementInstancerSetAttrInstanceElement(lua_State* L)
{
	LuaElementInstancer* lei = (LuaElementInstancer*)LuaType<ElementInstancer>::check(L, 1);
	RMLUI_CHECK_OBJ(lei);

	if (lua_type(L, 2) != LUA_TFUNCTION)
	{
		Log::Message(Log::LT_ERROR, "The argument to ElementInstancer.InstanceElement must be a function. You passed in a %s.", luaL_typename(L, 2));
		return 0;
	}
	lei->PushFunctionsTable(L); // top of the stack is now ELEMENTINSTANCERFUNCTIONS table
	lua_pushvalue(L, 2);        // copy of the function
	lei->ref_InstanceElement = luaL_ref(L, -2);
	lua_pop(L, 1);              // pop the ELEMENTINSTANCERFUNCTIONS table
	return 0;
}

RegType<ElementInstancer> ElementInstancerMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementInstancerGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementInstancerSetters[] = {
	RMLUI_LUASETTER(ElementInstancer, InstanceElement),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(ElementInstancer)
} // namespace Lua
} // namespace Rml
