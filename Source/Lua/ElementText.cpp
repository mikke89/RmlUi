#include "ElementText.h"
#include "Element.h"
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {
template <>
void ExtraInit<ElementText>(lua_State* L, int metatable_index)
{
	// inherit from Element
	ExtraInit<Element>(L, metatable_index);
	LuaType<Element>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<ElementText>(L);
}

int ElementTextGetAttrtext(lua_State* L)
{
	ElementText* obj = LuaType<ElementText>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushstring(L, obj->GetText().c_str());
	return 1;
}

int ElementTextSetAttrtext(lua_State* L)
{
	ElementText* obj = LuaType<ElementText>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	const char* text = luaL_checkstring(L, 2);
	obj->SetText(text);
	return 0;
}

RegType<ElementText> ElementTextMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementTextGetters[] = {
	RMLUI_LUAGETTER(ElementText, text),
	{nullptr, nullptr},
};

luaL_Reg ElementTextSetters[] = {
	RMLUI_LUASETTER(ElementText, text),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(ElementText)
} // namespace Lua
} // namespace Rml
