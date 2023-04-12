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
