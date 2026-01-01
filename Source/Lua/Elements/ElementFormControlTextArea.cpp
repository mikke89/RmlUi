#include "ElementFormControlTextArea.h"
#include "ElementFormControl.h"
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

// methods
int ElementFormControlTextAreaSelect(lua_State* /*L*/, ElementFormControlTextArea* obj)
{
	obj->Select();
	return 0;
}

int ElementFormControlTextAreaSetSelection(lua_State* L, ElementFormControlTextArea* obj)
{
	int start = (int)GetIndex(L, 1);
	int end = (int)GetIndex(L, 2);
	obj->SetSelectionRange(start, end);
	return 0;
}

int ElementFormControlTextAreaGetSelection(lua_State* L, ElementFormControlTextArea* obj)
{
	int selection_start = 0, selection_end = 0;
	String selected_text;
	obj->GetSelection(&selection_start, &selection_end, &selected_text);
	PushIndex(L, selection_start);
	PushIndex(L, selection_end);
	lua_pushstring(L, selected_text.c_str());
	return 3;
}

// getters
int ElementFormControlTextAreaGetAttrcols(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetNumColumns());
	return 1;
}

int ElementFormControlTextAreaGetAttrmaxlength(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetMaxLength());
	return 1;
}

int ElementFormControlTextAreaGetAttrrows(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetNumRows());
	return 1;
}

int ElementFormControlTextAreaGetAttrwordwrap(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushboolean(L, obj->GetWordWrap());
	return 1;
}

// setters
int ElementFormControlTextAreaSetAttrcols(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int cols = (int)luaL_checkinteger(L, 2);
	obj->SetNumColumns(cols);
	return 0;
}

int ElementFormControlTextAreaSetAttrmaxlength(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int ml = (int)luaL_checkinteger(L, 2);
	obj->SetMaxLength(ml);
	return 0;
}

int ElementFormControlTextAreaSetAttrrows(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int rows = (int)luaL_checkinteger(L, 2);
	obj->SetNumRows(rows);
	return 0;
}

int ElementFormControlTextAreaSetAttrwordwrap(lua_State* L)
{
	ElementFormControlTextArea* obj = LuaType<ElementFormControlTextArea>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	bool ww = RMLUI_CHECK_BOOL(L, 2);
	obj->SetWordWrap(ww);
	return 0;
}

RegType<ElementFormControlTextArea> ElementFormControlTextAreaMethods[] = {
	RMLUI_LUAMETHOD(ElementFormControlTextArea, Select),
	RMLUI_LUAMETHOD(ElementFormControlTextArea, SetSelection),
	RMLUI_LUAMETHOD(ElementFormControlTextArea, GetSelection),
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlTextAreaGetters[] = {
	RMLUI_LUAGETTER(ElementFormControlTextArea, cols),
	RMLUI_LUAGETTER(ElementFormControlTextArea, maxlength),
	RMLUI_LUAGETTER(ElementFormControlTextArea, rows),
	RMLUI_LUAGETTER(ElementFormControlTextArea, wordwrap),
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlTextAreaSetters[] = {
	RMLUI_LUASETTER(ElementFormControlTextArea, cols),
	RMLUI_LUASETTER(ElementFormControlTextArea, maxlength),
	RMLUI_LUASETTER(ElementFormControlTextArea, rows),
	RMLUI_LUASETTER(ElementFormControlTextArea, wordwrap),
	{nullptr, nullptr},
};

template <>
void ExtraInit<ElementFormControlTextArea>(lua_State* L, int metatable_index)
{
	ExtraInit<ElementFormControl>(L, metatable_index);
	LuaType<ElementFormControl>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<ElementFormControlTextArea>(L);
}

RMLUI_LUATYPE_DEFINE(ElementFormControlTextArea)
} // namespace Lua
} // namespace Rml
