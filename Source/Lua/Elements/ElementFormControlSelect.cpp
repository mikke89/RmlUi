#include "ElementFormControlSelect.h"
#include "ElementFormControl.h"
#include "SelectOptionsProxy.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

// methods
int ElementFormControlSelectAdd(lua_State* L, ElementFormControlSelect* obj)
{
	const char* rml = luaL_checkstring(L, 1);
	const char* value = luaL_checkstring(L, 2);
	int before = -1; // default
	if (lua_gettop(L) >= 3)
		before = GetIndex(L, 3);

	int index = obj->Add(rml, value, before);
	lua_pushinteger(L, index);
	return 1;
}

int ElementFormControlSelectRemove(lua_State* L, ElementFormControlSelect* obj)
{
	int index = GetIndex(L, 1);
	obj->Remove(index);
	return 0;
}

int ElementFormControlSelectRemoveAll(lua_State* /*L*/, ElementFormControlSelect* obj)
{
	obj->RemoveAll();
	return 0;
}

// getters
int ElementFormControlSelectGetAttroptions(lua_State* L)
{
	ElementFormControlSelect* obj = LuaType<ElementFormControlSelect>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	SelectOptionsProxy* proxy = new SelectOptionsProxy();
	proxy->owner = obj;
	LuaType<SelectOptionsProxy>::push(L, proxy, true);
	return 1;
}

int ElementFormControlSelectGetAttrselection(lua_State* L)
{
	ElementFormControlSelect* obj = LuaType<ElementFormControlSelect>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int selection = obj->GetSelection();
	PushIndex(L, selection);
	return 1;
}

// setter
int ElementFormControlSelectSetAttrselection(lua_State* L)
{
	ElementFormControlSelect* obj = LuaType<ElementFormControlSelect>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int selection = GetIndex(L, 2);
	obj->SetSelection(selection);
	return 0;
}

RegType<ElementFormControlSelect> ElementFormControlSelectMethods[] = {
	RMLUI_LUAMETHOD(ElementFormControlSelect, Add),
	RMLUI_LUAMETHOD(ElementFormControlSelect, Remove),
	RMLUI_LUAMETHOD(ElementFormControlSelect, RemoveAll),
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlSelectGetters[] = {
	RMLUI_LUAGETTER(ElementFormControlSelect, options),
	RMLUI_LUAGETTER(ElementFormControlSelect, selection),
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlSelectSetters[] = {
	RMLUI_LUASETTER(ElementFormControlSelect, selection),
	{nullptr, nullptr},
};

// inherits from ElementFormControl which inherits from Element
template <>
void ExtraInit<ElementFormControlSelect>(lua_State* L, int metatable_index)
{
	// init whatever elementformcontrol did extra, like inheritance
	ExtraInit<ElementFormControl>(L, metatable_index);
	// then inherit from elementformcontrol
	LuaType<ElementFormControl>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<ElementFormControlSelect>(L);
}
RMLUI_LUATYPE_DEFINE(ElementFormControlSelect)
} // namespace Lua
} // namespace Rml
