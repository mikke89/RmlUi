#include "NodeInstancer.h"

namespace Rml {
namespace Lua {
template <>
void ExtraInit<NodeInstancer>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, NodeInstancernew);
	lua_setfield(L, metatable_index - 1, "new");
}

// method
int NodeInstancernew(lua_State* L)
{
	LuaNodeInstancer* lei = new LuaNodeInstancer(L);
	LuaType<NodeInstancer>::push(L, lei, true);
	return 1;
}

// setter
int NodeInstancerSetAttrInstanceNode(lua_State* L)
{
	LuaNodeInstancer* lei = (LuaNodeInstancer*)LuaType<NodeInstancer>::check(L, 1);
	RMLUI_CHECK_OBJ(lei);

	if (lua_type(L, 2) != LUA_TFUNCTION)
	{
		Log::Message(Log::LT_ERROR, "The argument to NodeInstancer.InstanceNode must be a function. You passed in a %s.", luaL_typename(L, 2));
		return 0;
	}
	lei->PushFunctionsTable(L); // top of the stack is now NODEINSTANCERFUNCTIONS table
	lua_pushvalue(L, 2);        // copy of the function
	lei->ref_InstanceNode = luaL_ref(L, -2);
	lua_pop(L, 1);              // pop the NODEINSTANCERFUNCTIONS table
	return 0;
}

RegType<NodeInstancer> NodeInstancerMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg NodeInstancerGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg NodeInstancerSetters[] = {
	RMLUI_LUASETTER(NodeInstancer, InstanceNode),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(NodeInstancer)
} // namespace Lua
} // namespace Rml
