#include <RmlUi/Core/Variant.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

#if LUA_VERSION_NUM < 502
void lua_len(lua_State* L, int i)
{
	switch (lua_type(L, i))
	{
	case LUA_TSTRING: lua_pushnumber(L, (lua_Number)lua_objlen(L, i)); break;
	case LUA_TTABLE:
		if (!luaL_callmeta(L, i, "__len"))
			lua_pushnumber(L, (lua_Number)lua_objlen(L, i));
		break;
	case LUA_TUSERDATA:
		if (luaL_callmeta(L, i, "__len"))
			break;
		/* FALLTHROUGH */
	default: luaL_error(L, "attempt to get length of a %s value", lua_typename(L, lua_type(L, i)));
	}
}

lua_Integer luaL_len(lua_State* L, int i)
{
	lua_Integer res = 0;
	int isnum = 0;
	luaL_checkstack(L, 1, "not enough stack slots");
	lua_len(L, i);
	res = lua_tointegerx(L, -1, &isnum);
	lua_pop(L, 1);
	if (!isnum)
		luaL_error(L, "object length is not an integer");
	return res;
}

	#ifdef RMLUI_LUA_51_COMPAT
void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup)
{
	luaL_checkstack(L, nup + 1, "too many upvalues");
	for (; l->name != NULL; l++)
	{
		for (int i = 0; i < nup; i++)
			lua_pushvalue(L, -nup);
		lua_pushcclosure(L, l->func, nup);
		lua_setfield(L, -(nup + 2), l->name);
	}
	lua_pop(L, nup);
}

void* luaL_testudata(lua_State* L, int i, const char* tname)
{
	void* p = lua_touserdata(L, i);
	luaL_checkstack(L, 2, "not enough stack slots");
	if (p == NULL || !lua_getmetatable(L, i))
		return NULL;
	luaL_getmetatable(L, tname);
	if (!lua_rawequal(L, -1, -2))
		p = NULL;
	lua_pop(L, 2);
	return p;
}

void luaL_traceback(lua_State* L, lua_State* L1, const char* msg, int level)
{
	lua_Debug ar;
	int top = lua_gettop(L);
	if (msg)
		lua_pushfstring(L, "%s\n", msg);
	lua_pushliteral(L, "stack traceback:");
	while (lua_getstack(L1, level++, &ar))
	{
		lua_getinfo(L1, "Sln", &ar);
		lua_pushfstring(L, "\n\t%s:", ar.short_src);
		if (ar.currentline > 0)
			lua_pushfstring(L, "%d:", ar.currentline);
		if (*ar.namewhat != '\0')
			lua_pushfstring(L, " in function '%s'", ar.name);
		else if (*ar.what == 'm')
			lua_pushliteral(L, " in main chunk");
		else if (*ar.what == 'C' || *ar.what == 't') // C function or tail call
			lua_pushliteral(L, " ?");
		else
			lua_pushfstring(L, " in function <%s:%d>", ar.short_src, ar.linedefined);
		lua_concat(L, lua_gettop(L) - top);
	}
	lua_concat(L, lua_gettop(L) - top);
}

lua_Integer lua_tointegerx(lua_State* L, int i, int* isnum)
{
	lua_Integer n = lua_tointeger(L, i);
	if (isnum != NULL)
		*isnum = (n != 0 || lua_isnumber(L, i));
	return n;
}
	#endif
#endif

void PushVariant(lua_State* L, const Variant* var)
{
	if (!var)
	{
		lua_pushnil(L);
		return;
	}

	switch (var->GetType())
	{
	case Variant::BOOL: lua_pushboolean(L, var->Get<bool>()); break;
	case Variant::BYTE:
	case Variant::CHAR:
	case Variant::INT: lua_pushinteger(L, var->Get<int>()); break;
	case Variant::INT64: lua_pushinteger(L, var->Get<int64_t>()); break;
	case Variant::UINT: lua_pushinteger(L, var->Get<unsigned int>()); break;
	case Variant::UINT64: lua_pushinteger(L, var->Get<uint64_t>()); break;
	case Variant::FLOAT:
	case Variant::DOUBLE: lua_pushnumber(L, var->Get<double>()); break;
	case Variant::COLOURB: LuaType<Colourb>::push(L, new Colourb(var->Get<Colourb>()), true); break;
	case Variant::COLOURF: LuaType<Colourf>::push(L, new Colourf(var->Get<Colourf>()), true); break;
	case Variant::STRING:
	{
		const String& s = var->GetReference<Rml::String>();
		lua_pushlstring(L, s.c_str(), s.length());
	}
	break;
	case Variant::VECTOR2:
		// according to Variant.inl, it is going to be a Vector2f
		LuaType<Vector2f>::push(L, new Vector2f(var->Get<Vector2f>()), true);
		break;
	case Variant::VOIDPTR: lua_pushlightuserdata(L, var->Get<void*>()); break;
	default: lua_pushnil(L); break;
	}
}

void GetVariant(lua_State* L, int index, Variant* variant)
{
	if (!variant)
		return;

	switch (lua_type(L, index))
	{
	case LUA_TBOOLEAN: *variant = (bool)lua_toboolean(L, index); break;
	case LUA_TNUMBER: *variant = lua_tonumber(L, index); break;
	case LUA_TSTRING: *variant = Rml::String(lua_tostring(L, index)); break;
	case LUA_TLIGHTUSERDATA: *variant = lua_touserdata(L, index); break;
	case LUA_TNIL:
	default: // todo: support other types
		*variant = Variant();
		break;
	}
}

void PushIndex(lua_State* L, int index)
{
	lua_pushinteger(L, index + 1);
}

int GetIndex(lua_State* L, int index)
{
	return (int)luaL_checkinteger(L, index) - 1;
}

} // namespace Lua
} // namespace Rml
