/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "ElementFormControlInput.h"
#include "ElementFormControl.h"
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {

// methods
int ElementFormControlInputSelect(lua_State* /*L*/, ElementFormControlInput* obj)
{
	obj->Select();
	return 0;
}

int ElementFormControlInputSetSelection(lua_State* L, ElementFormControlInput* obj)
{
	int start = (int)GetIndex(L, 1);
	int end = (int)GetIndex(L, 2);
	obj->SetSelectionRange(start, end);
	return 0;
}

int ElementFormControlInputGetSelection(lua_State* L, ElementFormControlInput* obj)
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
int ElementFormControlInputGetAttrchecked(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushboolean(L, obj->HasAttribute("checked"));
	return 1;
}

int ElementFormControlInputGetAttrmaxlength(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetAttribute<int>("maxlength", -1));
	return 1;
}

int ElementFormControlInputGetAttrsize(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetAttribute<int>("size", 20));
	return 1;
}

int ElementFormControlInputGetAttrmax(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetAttribute<int>("max", 100));
	return 1;
}

int ElementFormControlInputGetAttrmin(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetAttribute<int>("min", 0));
	return 1;
}

int ElementFormControlInputGetAttrstep(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	lua_pushinteger(L, obj->GetAttribute<int>("step", 1));
	return 1;
}

// setters
int ElementFormControlInputSetAttrchecked(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	bool checked = RMLUI_CHECK_BOOL(L, 2);
	if (checked)
		obj->SetAttribute("checked", true);
	else
		obj->RemoveAttribute("checked");
	return 0;
}

int ElementFormControlInputSetAttrmaxlength(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int maxlength = (int)luaL_checkinteger(L, 2);
	obj->SetAttribute("maxlength", maxlength);
	return 0;
}

int ElementFormControlInputSetAttrsize(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int size = (int)luaL_checkinteger(L, 2);
	obj->SetAttribute("size", size);
	return 0;
}

int ElementFormControlInputSetAttrmax(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int max = (int)luaL_checkinteger(L, 2);
	obj->SetAttribute("max", max);
	return 0;
}

int ElementFormControlInputSetAttrmin(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int min = (int)luaL_checkinteger(L, 2);
	obj->SetAttribute("min", min);
	return 0;
}

int ElementFormControlInputSetAttrstep(lua_State* L)
{
	ElementFormControlInput* obj = LuaType<ElementFormControlInput>::check(L, 1);
	RMLUI_CHECK_OBJ(obj);
	int step = (int)luaL_checkinteger(L, 2);
	obj->SetAttribute("step", step);
	return 0;
}

RegType<ElementFormControlInput> ElementFormControlInputMethods[] = {
	RMLUI_LUAMETHOD(ElementFormControlInput, Select),
	RMLUI_LUAMETHOD(ElementFormControlInput, SetSelection),
	RMLUI_LUAMETHOD(ElementFormControlInput, GetSelection),
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlInputGetters[] = {
	RMLUI_LUAGETTER(ElementFormControlInput, checked),
	RMLUI_LUAGETTER(ElementFormControlInput, maxlength),
	RMLUI_LUAGETTER(ElementFormControlInput, size),
	RMLUI_LUAGETTER(ElementFormControlInput, max),
	RMLUI_LUAGETTER(ElementFormControlInput, min),
	RMLUI_LUAGETTER(ElementFormControlInput, step),
	{nullptr, nullptr},
};

luaL_Reg ElementFormControlInputSetters[] = {
	RMLUI_LUASETTER(ElementFormControlInput, checked),
	RMLUI_LUASETTER(ElementFormControlInput, maxlength),
	RMLUI_LUASETTER(ElementFormControlInput, size),
	RMLUI_LUASETTER(ElementFormControlInput, max),
	RMLUI_LUASETTER(ElementFormControlInput, min),
	RMLUI_LUASETTER(ElementFormControlInput, step),
	{nullptr, nullptr},
};

template <>
void ExtraInit<ElementFormControlInput>(lua_State* L, int metatable_index)
{
	ExtraInit<ElementFormControl>(L, metatable_index);
	LuaType<ElementFormControl>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<ElementFormControlInput>(L);
}
RMLUI_LUATYPE_DEFINE(ElementFormControlInput)
} // namespace Lua
} // namespace Rml
