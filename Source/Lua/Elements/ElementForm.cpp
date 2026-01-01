#include "ElementForm.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementForm.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

// method
int ElementFormSubmit(lua_State* L, ElementForm* obj)
{
	int top = lua_gettop(L);
	const char* name = "";
	const char* value = "";
	if (top > 0)
	{
		name = luaL_checkstring(L, 1);
		if (top > 1)
			value = luaL_checkstring(L, 2);
	}
	obj->Submit(name, value);
	return 0;
}

RegType<ElementForm> ElementFormMethods[] = {
	RMLUI_LUAMETHOD(ElementForm, Submit),
	{nullptr, nullptr},
};

luaL_Reg ElementFormGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementFormSetters[] = {
	{nullptr, nullptr},
};

template <>
void ExtraInit<ElementForm>(lua_State* L, int metatable_index)
{
	// inherit from Element
	ExtraInit<Element>(L, metatable_index);
	LuaType<Element>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<ElementForm>(L);
}
RMLUI_LUATYPE_DEFINE(ElementForm)
} // namespace Lua
} // namespace Rml
