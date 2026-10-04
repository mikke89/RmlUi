#include "ElementInstancer.h"

namespace Rml {
namespace Lua {
template <>
void ExtraInit<LuaElementInstancer>(lua_State* L, int metatable_index)
{
	lua_pushcfunction(L, ElementInstancernew);
	lua_setfield(L, metatable_index - 1, "new");
}

// method
int ElementInstancernew(lua_State* L)
{
	LuaType<LuaElementInstancer>::emplace(L, L);
	return 1;
}

// setter
int ElementInstancerSetAttrInstanceElement(lua_State* L)
{
	auto lei = LuaType<LuaElementInstancer>::check(L, 1);
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

RegType<LuaElementInstancer> ElementInstancerMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementInstancerGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementInstancerSetters[] = {
	RMLUI_LUASETTER(ElementInstancer, InstanceElement),
	{nullptr, nullptr},
};

template <>
const char* GetTClassName<LuaElementInstancer>()
{
	return "ElementInstancer";
}

template <>
RegType<LuaElementInstancer>* GetMethodTable<LuaElementInstancer>()
{
	return ElementInstancerMethods;
}

template <>
luaL_Reg* GetAttrTable<LuaElementInstancer>()
{
	return ElementInstancerGetters;
}

template <>
luaL_Reg* SetAttrTable<LuaElementInstancer>()
{
	return ElementInstancerSetters;
}

} // namespace Lua
} // namespace Rml
