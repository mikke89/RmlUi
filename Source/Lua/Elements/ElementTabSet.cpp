#include "ElementTabSet.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

// methods
int ElementTabSetSetPanel(lua_State* L, ElementTabSet* obj)
{
	RMLUI_CHECK_OBJ(obj);
	int index = GetIndex(L, 1);
	const char* rml = luaL_checkstring(L, 2);

	obj->SetPanel(index, rml);
	return 0;
}

int ElementTabSetSetTab(lua_State* L, ElementTabSet* obj)
{
	RMLUI_CHECK_OBJ(obj);
	int index = GetIndex(L, 1);
	const char* rml = luaL_checkstring(L, 2);

	obj->SetTab(index, rml);
	return 0;
}

// getters
int ElementTabSetGetAttractive_tab(lua_State* L)
{
	ElementTabSet* obj = LuaType<ElementTabSet>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int tab = obj->GetActiveTab();
	PushIndex(L, tab);
	return 1;
}

int ElementTabSetGetAttrnum_tabs(lua_State* L)
{
	ElementTabSet* obj = LuaType<ElementTabSet>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int num = obj->GetNumTabs();
	lua_pushinteger(L, num);
	return 1;
}

// setter
int ElementTabSetSetAttractive_tab(lua_State* L)
{
	ElementTabSet* obj = LuaType<ElementTabSet>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int tab = GetIndex(L, 2);
	obj->SetActiveTab(tab);
	return 0;
}

RegType<ElementTabSet> ElementTabSetMethods[] = {
	RMLUI_LUAMETHOD(ElementTabSet, SetPanel),
	RMLUI_LUAMETHOD(ElementTabSet, SetTab),
	{nullptr, nullptr},
};

luaL_Reg ElementTabSetGetters[] = {
	RMLUI_LUAGETTER(ElementTabSet, active_tab),
	RMLUI_LUAGETTER(ElementTabSet, num_tabs),
	{nullptr, nullptr},
};

luaL_Reg ElementTabSetSetters[] = {
	RMLUI_LUASETTER(ElementTabSet, active_tab),
	{nullptr, nullptr},
};

// this will be used to "inherit" from Element
template <>
void ExtraInit<ElementTabSet>(lua_State* L, int metatable_index)
{
	ExtraInit<Element>(L, metatable_index);
	LuaType<Element>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<ElementTabSet>(L);
}

RMLUI_LUATYPE_DEFINE(ElementTabSet)
} // namespace Lua
} // namespace Rml
