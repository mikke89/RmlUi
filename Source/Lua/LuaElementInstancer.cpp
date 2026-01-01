#include "LuaElementInstancer.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Lua/Interpreter.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {
// This will be called from ElementInstancernew
LuaElementInstancer::LuaElementInstancer(lua_State* L) : ElementInstancer(), ref_InstanceElement(LUA_NOREF)
{
	if (lua_type(L, 1) != LUA_TFUNCTION && !lua_isnoneornil(L, 1))
	{
		Log::Message(Log::LT_ERROR, "The argument to ElementInstancer.new has to be a function or nil. You passed in a %s.", luaL_typename(L, 1));
		return;
	}
	PushFunctionsTable(L); // top of the table is now ELEMENTINSTANCERFUNCTIONS table
	lua_pushvalue(L, 1);   // copy of the function
	ref_InstanceElement = luaL_ref(L, -2);
	lua_pop(L, 1);         // pop the ELEMENTINSTANCERFUNCTIONS table
}

ElementPtr LuaElementInstancer::InstanceElement(Element* /*parent*/, const String& tag, const XMLAttributes& /*attributes*/)
{
	lua_State* L = Interpreter::GetLuaState();
	int top = lua_gettop(L);
	ElementPtr ret = nullptr;
	if (ref_InstanceElement != LUA_REFNIL && ref_InstanceElement != LUA_NOREF)
	{
		PushFunctionsTable(L);
		lua_rawgeti(L, -1, ref_InstanceElement); // push the function
		lua_pushstring(L, tag.c_str());          // push the tag
		Interpreter::ExecuteCall(1, 1);          // we pass in a string, and we want to get an Element back
		ret = std::move(*LuaType<ElementPtr>::check(L, -1));
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Attempt to call the function for ElementInstancer.InstanceElement, the function does not exist.");
	}
	lua_settop(L, top);
	return ret;
}

void LuaElementInstancer::ReleaseElement(Element* element)
{
	delete element;
}

void LuaElementInstancer::PushFunctionsTable(lua_State* L)
{
	// make sure there is an area to save the function
	lua_getglobal(L, "ELEMENTINSTANCERFUNCTIONS");
	if (lua_isnoneornil(L, -1))
	{
		lua_newtable(L);
		lua_setglobal(L, "ELEMENTINSTANCERFUNCTIONS");
		lua_pop(L, 1); // pop the unsucessful getglobal
		lua_getglobal(L, "ELEMENTINSTANCERFUNCTIONS");
	}
}

} // namespace Lua
} // namespace Rml
