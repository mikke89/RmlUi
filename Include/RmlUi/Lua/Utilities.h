#pragma once
/*
    This file is for free-floating functions that are used across more than one file.
*/
#include "Header.h"
#include "IncludeLua.h"
#include "LuaType.h"
#include <RmlUi/Core/Types.h>

namespace Rml {
namespace Lua {

#if LUA_VERSION_NUM < 502
	#define lua_setuservalue(L, i) (luaL_checktype((L), -1, LUA_TTABLE), lua_setfenv((L), (i)))

	inline int lua_absindex(lua_State* L, int idx)
	{
		if (idx > LUA_REGISTRYINDEX && idx < 0)
			return lua_gettop(L) + idx + 1;
		else
			return idx;
	}

	void lua_len(lua_State* L, int i);
	lua_Integer luaL_len(lua_State* L, int i);
#endif

	/** casts the variant to its specific type before pushing it to the stack
	@relates LuaType */
	void RMLUILUA_API PushVariant(lua_State* L, const Variant* var);

	/** Populate the variant based on the Lua value at the given index */
	void RMLUILUA_API GetVariant(lua_State* L, int index, Variant* variant);

	// Converts index from 0-based to 1-based before pushing it to the stack
	void RMLUILUA_API PushIndex(lua_State* L, int index);

	// Returns 0-based index after retrieving from stack and converting from 1-based
	// The index parameter refers to the position on the stack and is not affected
	// by the conversion
	int RMLUILUA_API GetIndex(lua_State* L, int index);

	// Helper function, so that the types don't have to define individual functions themselves
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
	void AddTypeToElementAsTable(lua_State* L)
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
