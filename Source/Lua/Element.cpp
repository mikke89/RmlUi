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

#include "Element.h"
#include "ElementAttributesProxy.h"
#include "ElementChildNodesProxy.h"
#include "ElementStyleProxy.h"
#include "LuaEventListener.h"
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Lua {
typedef ElementDocument Document;

template <>
void ExtraInit<Element>(lua_State* L, int metatable_index)
{
	int top = lua_gettop(L);
	// guarantee the "Element.As" table exists
	lua_getfield(L, metatable_index - 1, "As");
	if (lua_isnoneornil(L, -1)) // if it doesn't exist, create it
	{
		lua_newtable(L);
		lua_setfield(L, metatable_index - 1, "As");
	}
	lua_pop(L, 1); // pop the result of lua_getfield
	lua_pushcfunction(L, Elementnew);
	lua_setfield(L, metatable_index - 1, "new");
	lua_settop(L, top);
}

int Elementnew(lua_State* L)
{
	const char* tag = luaL_checkstring(L, 1);
	Element* ele = new Element(tag);
	LuaType<Element>::push(L, ele, true);
	return 1;
}

// methods
int ElementAddEventListener(lua_State* L, Element* obj)
{
	int top = lua_gettop(L);
	bool capture = false;
	// default false if they didn't pass it in
	if (top > 2)
		capture = RMLUI_CHECK_BOOL(L, 3);

	const char* event = luaL_checkstring(L, 1);

	LuaEventListener* listener = nullptr;
	int type = lua_type(L, 2);
	if (type == LUA_TFUNCTION)
	{
		listener = new LuaEventListener(L, 2, obj);
	}
	else if (type == LUA_TSTRING)
	{
		const char* code = luaL_checkstring(L, 2);
		listener = new LuaEventListener(code, obj);
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Lua Context:AddEventLisener's 2nd argument can only be a Lua function or a string, you passed in a %s",
			lua_typename(L, type));
	}

	if (listener != nullptr)
	{
		obj->AddEventListener(event, listener, capture);
	}
	return 0;
}

int ElementAppendChild(lua_State* L, Element* obj)
{
	ElementPtr* element = LuaType<ElementPtr>::check(L, 1);
	if (*element)
	{
		Element* child = obj->AppendChild(std::move(*element));
		LuaType<Element>::push(L, child, false);
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Could not append child to element '%s', as the child was null. Was it already moved from?",
			obj->GetAddress().c_str());
		lua_pushnil(L);
	}
	return 1;
}

int ElementBlur(lua_State* /*L*/, Element* obj)
{
	obj->Blur();
	return 0;
}

int ElementClick(lua_State* /*L*/, Element* obj)
{
	obj->Click();
	return 0;
}

int ElementDispatchEvent(lua_State* L, Element* obj)
{
	const char* event = luaL_checkstring(L, 1);
	Dictionary params;
	lua_pushnil(L); // becauase lua_next pops a key from the stack first, we don't want to pop the table
	while (lua_next(L, 2) != 0)
	{
		//[-1] is value, [-2] is key
		int type = lua_type(L, -1);
		const char* key = luaL_checkstring(L, -2); // key HAS to be a string, or things will go bad
		switch (type)
		{
		case LUA_TNUMBER: params[key] = (float)lua_tonumber(L, -1); break;
		case LUA_TBOOLEAN: params[key] = RMLUI_CHECK_BOOL(L, -1); break;
		case LUA_TSTRING: params[key] = luaL_checkstring(L, -1); break;
		case LUA_TUSERDATA:
		case LUA_TLIGHTUSERDATA: params[key] = lua_touserdata(L, -1); break;
		default: break;
		}
		lua_pop(L, 1); // pops value, leaves key for next iteration
	}
	obj->DispatchEvent(event, params);
	return 0;
}

int ElementFocus(lua_State* /*L*/, Element* obj)
{
	obj->Focus();
	return 0;
}

int ElementGetAttribute(lua_State* L, Element* obj)
{
	const char* name = luaL_checkstring(L, 1);
	Variant* var = obj->GetAttribute(name);
	PushVariant(L, var);
	return 1;
}

int ElementGetElementById(lua_State* L, Element* obj)
{
	const char* id = luaL_checkstring(L, 1);
	Element* ele = obj->GetElementById(id);
	LuaType<Element>::push(L, ele, false);
	return 1;
}

int ElementGetElementsByTagName(lua_State* L, Element* obj)
{
	const char* tag = luaL_checkstring(L, 1);
	ElementList list;
	obj->GetElementsByTagName(list, tag);
	lua_newtable(L);
	for (unsigned int i = 0; i < list.size(); i++)
	{
		PushIndex(L, i);
		LuaType<Element>::push(L, list[i], false);
		lua_settable(L, -3); //-3 is the table
	}
	return 1;
}

int ElementQuerySelector(lua_State* L, Element* obj)
{
	const char* sel = luaL_checkstring(L, 1);
	Element* ele = obj->QuerySelector(sel);
	LuaType<Element>::push(L, ele, false);
	return 1;
}

int ElementQuerySelectorAll(lua_State* L, Element* obj)
{
	const char* tag = luaL_checkstring(L, 1);
	ElementList list;
	obj->QuerySelectorAll(list, tag);
	lua_newtable(L);
	for (unsigned int i = 0; i < list.size(); i++)
	{
		PushIndex(L, i);
		LuaType<Element>::push(L, list[i], false);
		lua_settable(L, -3); //-3 is the table
	}
	return 1;
}

int ElementMatches(lua_State* L, Element* obj)
{
	const char* tag = luaL_checkstring(L, 1);
	lua_pushboolean(L, obj->Matches(tag));
	return 1;
}

int ElementHasAttribute(lua_State* L, Element* obj)
{
	const char* name = luaL_checkstring(L, 1);
	lua_pushboolean(L, obj->HasAttribute(name));
	return 1;
}

int ElementHasChildNodes(lua_State* L, Element* obj)
{
	lua_pushboolean(L, obj->HasChildNodes());
	return 1;
}

int ElementInsertBefore(lua_State* L, Element* obj)
{
	ElementPtr* element = LuaType<ElementPtr>::check(L, 1);
	Element* adjacent = LuaType<Element>::check(L, 2);
	if (*element)
	{
		Element* inserted = obj->InsertBefore(std::move(*element), adjacent);
		LuaType<Element>::push(L, inserted, false);
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Could not insert child to element '%s', as the child was null. Was it already moved from?",
			obj->GetAddress().c_str());
		lua_pushnil(L);
	}
	return 1;
}

int ElementIsClassSet(lua_State* L, Element* obj)
{
	const char* name = luaL_checkstring(L, 1);
	lua_pushboolean(L, obj->IsClassSet(name));
	return 1;
}

int ElementRemoveAttribute(lua_State* L, Element* obj)
{
	const char* name = luaL_checkstring(L, 1);
	obj->RemoveAttribute(name);
	return 0;
}

int ElementRemoveChild(lua_State* L, Element* obj)
{
	Element* element = LuaType<Element>::check(L, 1);
	lua_pushboolean(L, static_cast<bool>(obj->RemoveChild(element)));
	return 1;
}

int ElementReplaceChild(lua_State* L, Element* obj)
{
	ElementPtr* inserted = LuaType<ElementPtr>::check(L, 1);
	Element* replaced = LuaType<Element>::check(L, 2);
	if (*inserted)
		lua_pushboolean(L, static_cast<bool>(obj->ReplaceChild(std::move(*inserted), replaced)));
	else
		Log::Message(Log::LT_WARNING, "Could not replace child in element '%s', as the child was null. Was it already moved from?",
			obj->GetAddress().c_str());
	return 1;
}

int ElementScrollIntoView(lua_State* L, Element* obj)
{
	bool align = RMLUI_CHECK_BOOL(L, 1);
	obj->ScrollIntoView(align);
	return 0;
}

int ElementSetAttribute(lua_State* L, Element* obj)
{
	const char* name = luaL_checkstring(L, 1);
	const char* value = luaL_checkstring(L, 2);
	obj->SetAttribute(name, String(value));
	return 0;
}

int ElementSetClass(lua_State* L, Element* obj)
{
	const char* name = luaL_checkstring(L, 1);
	bool value = RMLUI_CHECK_BOOL(L, 2);
	obj->SetClass(name, value);
	return 0;
}

// getters
int ElementGetAttrattributes(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	ElementAttributesProxy* proxy = new ElementAttributesProxy();
	proxy->owner = ele;
	LuaType<ElementAttributesProxy>::push(L, proxy, true);
	return 1;
}

int ElementGetAttrchild_nodes(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	ElementChildNodesProxy* ecnp = new ElementChildNodesProxy();
	ecnp->owner = ele;
	LuaType<ElementChildNodesProxy>::push(L, ecnp, true);
	return 1;
}

int ElementGetAttrclass_name(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	String classnames = ele->GetClassNames();
	lua_pushstring(L, classnames.c_str());
	return 1;
}

int ElementGetAttrclient_left(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetClientLeft());
	return 1;
}

int ElementGetAttrclient_height(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetClientHeight());
	return 1;
}

int ElementGetAttrclient_top(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetClientTop());
	return 1;
}

int ElementGetAttrclient_width(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetClientWidth());
	return 1;
}

int ElementGetAttrfirst_child(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	Element* child = ele->GetFirstChild();
	if (child == nullptr)
		lua_pushnil(L);
	else
		LuaType<Element>::push(L, child, false);
	return 1;
}

int ElementGetAttrid(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushstring(L, ele->GetId().c_str());
	return 1;
}

int ElementGetAttrinner_rml(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushstring(L, ele->GetInnerRML().c_str());
	return 1;
}

int ElementGetAttrlast_child(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	Element* child = ele->GetLastChild();
	if (child == nullptr)
		lua_pushnil(L);
	else
		LuaType<Element>::push(L, child, false);
	return 1;
}

int ElementGetAttrnext_sibling(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	Element* sibling = ele->GetNextSibling();
	if (sibling == nullptr)
		lua_pushnil(L);
	else
		LuaType<Element>::push(L, sibling, false);
	return 1;
}

int ElementGetAttroffset_height(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetOffsetHeight());
	return 1;
}

int ElementGetAttroffset_left(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetOffsetLeft());
	return 1;
}

int ElementGetAttroffset_parent(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	Element* parent = ele->GetOffsetParent();
	LuaType<Element>::push(L, parent, false);
	return 1;
}

int ElementGetAttroffset_top(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetOffsetTop());
	return 1;
}

int ElementGetAttroffset_width(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetOffsetWidth());
	return 1;
}

int ElementGetAttrowner_document(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	Document* doc = ele->GetOwnerDocument();
	LuaType<Document>::push(L, doc, false);
	return 1;
}

int ElementGetAttrparent_node(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	Element* parent = ele->GetParentNode();
	if (parent == nullptr)
		lua_pushnil(L);
	else
		LuaType<Element>::push(L, parent, false);
	return 1;
}

int ElementGetAttrprevious_sibling(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	Element* sibling = ele->GetPreviousSibling();
	if (sibling == nullptr)
		lua_pushnil(L);
	else
		LuaType<Element>::push(L, sibling, false);
	return 1;
}

int ElementGetAttrscroll_height(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetScrollHeight());
	return 1;
}

int ElementGetAttrscroll_left(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetScrollLeft());
	return 1;
}

int ElementGetAttrscroll_top(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetScrollTop());
	return 1;
}

int ElementGetAttrscroll_width(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushnumber(L, ele->GetScrollWidth());
	return 1;
}

int ElementGetAttrstyle(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	ElementStyleProxy* prox = new ElementStyleProxy();
	prox->owner = ele;
	LuaType<ElementStyleProxy>::push(L, prox, true);
	return 1;
}

int ElementGetAttrtag_name(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	lua_pushstring(L, ele->GetTagName().c_str());
	return 1;
}

// setters
int ElementSetAttrclass_name(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	const char* name = luaL_checkstring(L, 2);
	ele->SetClassNames(name);
	return 0;
}

int ElementSetAttrid(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	const char* id = luaL_checkstring(L, 2);
	ele->SetId(id);
	return 0;
}

int ElementSetAttrinner_rml(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	const char* rml = luaL_checkstring(L, 2);
	ele->SetInnerRML(rml);
	return 0;
}

int ElementSetAttrscroll_left(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	float scroll = (float)luaL_checknumber(L, 2);
	ele->SetScrollLeft(scroll);
	return 0;
}

int ElementSetAttrscroll_top(lua_State* L)
{
	Element* ele = LuaType<Element>::check(L, 1);
	RMLUI_CHECK_OBJ(ele);
	float scroll = (float)luaL_checknumber(L, 2);
	ele->SetScrollTop(scroll);
	return 0;
}

RegType<Element> ElementMethods[] = {
	RMLUI_LUAMETHOD(Element, AddEventListener),
	RMLUI_LUAMETHOD(Element, AppendChild),
	RMLUI_LUAMETHOD(Element, Blur),
	RMLUI_LUAMETHOD(Element, Click),
	RMLUI_LUAMETHOD(Element, DispatchEvent),
	RMLUI_LUAMETHOD(Element, Focus),
	RMLUI_LUAMETHOD(Element, GetAttribute),
	RMLUI_LUAMETHOD(Element, GetElementById),
	RMLUI_LUAMETHOD(Element, GetElementsByTagName),
	RMLUI_LUAMETHOD(Element, QuerySelector),
	RMLUI_LUAMETHOD(Element, QuerySelectorAll),
	RMLUI_LUAMETHOD(Element, Matches),
	RMLUI_LUAMETHOD(Element, HasAttribute),
	RMLUI_LUAMETHOD(Element, HasChildNodes),
	RMLUI_LUAMETHOD(Element, InsertBefore),
	RMLUI_LUAMETHOD(Element, IsClassSet),
	RMLUI_LUAMETHOD(Element, RemoveAttribute),
	RMLUI_LUAMETHOD(Element, RemoveChild),
	RMLUI_LUAMETHOD(Element, ReplaceChild),
	RMLUI_LUAMETHOD(Element, ScrollIntoView),
	RMLUI_LUAMETHOD(Element, SetAttribute),
	RMLUI_LUAMETHOD(Element, SetClass),
	{nullptr, nullptr},
};

luaL_Reg ElementGetters[] = {
	RMLUI_LUAGETTER(Element, attributes),
	RMLUI_LUAGETTER(Element, child_nodes),
	RMLUI_LUAGETTER(Element, class_name),
	RMLUI_LUAGETTER(Element, client_left),
	RMLUI_LUAGETTER(Element, client_height),
	RMLUI_LUAGETTER(Element, client_top),
	RMLUI_LUAGETTER(Element, client_width),
	RMLUI_LUAGETTER(Element, first_child),
	RMLUI_LUAGETTER(Element, id),
	RMLUI_LUAGETTER(Element, inner_rml),
	RMLUI_LUAGETTER(Element, last_child),
	RMLUI_LUAGETTER(Element, next_sibling),
	RMLUI_LUAGETTER(Element, offset_height),
	RMLUI_LUAGETTER(Element, offset_left),
	RMLUI_LUAGETTER(Element, offset_parent),
	RMLUI_LUAGETTER(Element, offset_top),
	RMLUI_LUAGETTER(Element, offset_width),
	RMLUI_LUAGETTER(Element, owner_document),
	RMLUI_LUAGETTER(Element, parent_node),
	RMLUI_LUAGETTER(Element, previous_sibling),
	RMLUI_LUAGETTER(Element, scroll_height),
	RMLUI_LUAGETTER(Element, scroll_left),
	RMLUI_LUAGETTER(Element, scroll_top),
	RMLUI_LUAGETTER(Element, scroll_width),
	RMLUI_LUAGETTER(Element, style),
	RMLUI_LUAGETTER(Element, tag_name),
	{nullptr, nullptr},
};

luaL_Reg ElementSetters[] = {
	RMLUI_LUASETTER(Element, class_name),
	RMLUI_LUASETTER(Element, id),
	RMLUI_LUASETTER(Element, inner_rml),
	RMLUI_LUASETTER(Element, scroll_left),
	RMLUI_LUASETTER(Element, scroll_top),
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(Element)

template <>
void ExtraInit<ElementPtr>(lua_State* /*L*/, int /*metatable_index*/)
{
	return;
}

RegType<ElementPtr> ElementPtrMethods[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementPtrGetters[] = {
	{nullptr, nullptr},
};

luaL_Reg ElementPtrSetters[] = {
	{nullptr, nullptr},
};

RMLUI_LUATYPE_DEFINE(ElementPtr)

} // namespace Lua
} // namespace Rml
