#pragma once
/*
    These are helper functions to fill up the Element.As table with types that are able to be casted
*/

#include <RmlUi/Core/Element.h>
#include <RmlUi/Lua/IncludeLua.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

// Helper function for the controls, so that the types don't have to define individual functions themselves
//  to fill the Elements.As table
template <typename ToType>
int CastFromElementTo(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	LuaType<ToType>::push(L, (ToType*)ele, false);
	return 1;
}

// Adds to the Element.As table the name of the type, and the function to use to cast
template <typename T>
void AddCastFunctionToElementAsTable(lua_State* L)
{
	int top = lua_gettop(L);
	lua_getglobal(L, "Element");
	lua_getfield(L, -1, "As");
	if (!lua_isnoneornil(L, -1))
	{
		lua_pushcfunction(L, CastFromElementTo<T>);
		lua_setfield(L, -2, GetTClassName<T>());
	}
	lua_settop(L, top); // pop "As" and "Element"
}

} // namespace Lua
} // namespace Rml
