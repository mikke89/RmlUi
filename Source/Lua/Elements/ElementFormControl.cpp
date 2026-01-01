#include "ElementFormControl.h"
#include "../Element.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

// getters
int ElementFormControlGetAttrdisabled(lua_State* L)
{
	ElementFormControl* efc = LuaType<ElementFormControl>::check(L, 1);
	RMLUI_CHECK_OBJ(efc);
	lua_pushboolean(L, efc->IsDisabled());
	return 1;
}

int ElementFormControlGetAttrname(lua_State* L)
{
	ElementFormControl* efc = LuaType<ElementFormControl>::check(L, 1);
	RMLUI_CHECK_OBJ(efc);
	lua_pushstring(L, efc->GetName().c_str());
	return 1;
}

int ElementFormControlGetAttrvalue(lua_State* L)
{
	ElementFormControl* efc = LuaType<ElementFormControl>::check(L, 1);
	RMLUI_CHECK_OBJ(efc);
	lua_pushstring(L, efc->GetValue().c_str());
	return 1;
}

// setters
int ElementFormControlSetAttrdisabled(lua_State* L)
{
	ElementFormControl* efc = LuaType<ElementFormControl>::check(L, 1);
	RMLUI_CHECK_OBJ(efc);
	efc->SetDisabled(RMLUI_CHECK_BOOL(L, 2));
	return 0;
}

int ElementFormControlSetAttrname(lua_State* L)
{
	ElementFormControl* efc = LuaType<ElementFormControl>::check(L, 1);
	RMLUI_CHECK_OBJ(efc);
	const char* name = luaL_checkstring(L, 2);
	efc->SetName(name);
	return 0;
}

int ElementFormControlSetAttrvalue(lua_State* L)
{
	ElementFormControl* efc = LuaType<ElementFormControl>::check(L, 1);
	RMLUI_CHECK_OBJ(efc);
	const char* value = luaL_checkstring(L, 2);
	efc->SetValue(value);
	return 0;
}

RegType<ElementFormControl> ElementFormControlMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlGetters[] = {
	RMLUI_LUAGETTER(ElementFormControl, disabled),
	RMLUI_LUAGETTER(ElementFormControl, name),
	RMLUI_LUAGETTER(ElementFormControl, value),
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlSetters[] = {
	RMLUI_LUASETTER(ElementFormControl, disabled),
	RMLUI_LUASETTER(ElementFormControl, name),
	RMLUI_LUASETTER(ElementFormControl, value),
	{nullptr, nullptr},
};

template <>
void ExtraInit<ElementFormControl>(lua_State* L, int metatable_index)
{
	ExtraInit<Element>(L, metatable_index);
	LuaType<Element>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<ElementFormControl>(L);
}
RMLUI_LUATYPE_DEFINE(ElementFormControl)
} // namespace Lua
} // namespace Rml
