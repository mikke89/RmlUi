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

#include "Document.h"
//#include "Element.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Lua/Utilities.h>

namespace Rml {
namespace Qjs {

/*
template <>
void ExtraInit<Document>(lua_State* L, int metatable_index)
{
	// we will inherit from Element
	ExtraInit<Element>(L, metatable_index);
	LuaType<Element>::_regfunctions(L, metatable_index, metatable_index - 1);
	AddTypeToElementAsTable<Document>(L);

	// create the DocumentFocus table
	lua_getglobal(L, "DocumentModal");
	if (lua_isnoneornil(L, -1))
	{
		lua_pop(L, 1);   // pop unsucessful getglobal
		lua_newtable(L); // create a table for holding the enum
		lua_pushinteger(L, (int)ModalFlag::None);
		lua_setfield(L, -2, "None");
		lua_pushinteger(L, (int)ModalFlag::Modal);
		lua_setfield(L, -2, "Modal");
		lua_pushinteger(L, (int)ModalFlag::Keep);
		lua_setfield(L, -2, "Keep");
		lua_setglobal(L, "DocumentModal");
	}

	// create the DocumentFocus table
	lua_getglobal(L, "DocumentFocus");
	if (lua_isnoneornil(L, -1))
	{
		lua_pop(L, 1);   // pop unsucessful getglobal
		lua_newtable(L); // create a table for holding the enum
		lua_pushinteger(L, (int)FocusFlag::None);
		lua_setfield(L, -2, "None");
		lua_pushinteger(L, (int)FocusFlag::Document);
		lua_setfield(L, -2, "Document");
		lua_pushinteger(L, (int)FocusFlag::Keep);
		lua_setfield(L, -2, "Keep");
		lua_pushinteger(L, (int)FocusFlag::Auto);
		lua_setfield(L, -2, "Auto");
		lua_setglobal(L, "DocumentFocus");
	}
}
*/

// methods
JSValue pullToFront( JSContext* ctx, JSValue _this, int argc, JSValue* argv )
{
	obj->PullToFront();
	return 0;
}

JSValue pushToBack( JSContext* ctx, JSValue _this, int argc, JSValue* argv )
{
	obj->PushToBack();
	return 0;
}

JSValue show( JSContext* ctx, JSValue _this, int argc, JSValue* argv )
{
	int top = lua_gettop(L);
	if (top == 0)
		obj->Show();
	else if (top == 1)
	{
		ModalFlag modal = (ModalFlag)luaL_checkinteger(L, 1);
		obj->Show(modal);
	}
	else
	{
		ModalFlag modal = (ModalFlag)luaL_checkinteger(L, 1);
		FocusFlag focus = (FocusFlag)luaL_checkinteger(L, 2);
		obj->Show(modal, focus);
	}
	return 0;
}

JSValue hide( JSContext* ctx, JSValue _this, int argc, JSValue* argv )
{
	obj->Hide();
	return 0;
}

JSValue close( JSContext* ctx, JSValue _this, int argc, JSValue* argv )
{
	obj->Close();
	return 0;
}

JSValue createElement( JSContext* ctx, JSValue _this, int argc, JSValue* argv )
{
	const char* tag = luaL_checkstring(L, 1);
	ElementPtr* ele = new ElementPtr(obj->CreateElement(tag));
	LuaType<ElementPtr>::push(L, ele, true);
	return 1;
}

JSValue createTextNode( JSContext* ctx, JSValue _this, int argc, JSValue* argv )
{
	// need ElementText object first
	const char* text = luaL_checkstring(L, 1);
	ElementPtr* et = new ElementPtr(obj->CreateTextNode(text));
	LuaType<ElementPtr>::push(L, et, true);
	return 1;
}

// getters
int DocumentGetAttrtitle(lua_State* L)
{
	Document* doc = LuaType<Document>::check(L, 1);
	RMLUI_CHECK_OBJ(doc);
	lua_pushstring(L, doc->GetTitle().c_str());
	return 1;
}

int DocumentGetAttrcontext(lua_State* L)
{
	Document* doc = LuaType<Document>::check(L, 1);
	RMLUI_CHECK_OBJ(doc);
	LuaType<Context>::push(L, doc->GetContext(), false);
	return 1;
}

// setters
int DocumentSetAttrtitle(lua_State* L)
{
	Document* doc = LuaType<Document>::check(L, 1);
	RMLUI_CHECK_OBJ(doc);
	const char* title = luaL_checkstring(L, 2);
	doc->SetTitle(title);
	return 0;
}


static const JSCFunctionListEntry _functions[] = {
	JS_CFUNC_DEF( "pullToFront", 0, nullptr ),			// name: string, width: number, height: number
	JS_CFUNC_DEF( "pushToBack", 0, nullptr ),		// const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face
	JS_CFUNC_DEF( "show", 0, nullptr ),		// name: string, callback: Function
	JS_CFUNC_DEF( "hide", 0, nullptr ),		// name: string, callback: Function
	JS_CFUNC_DEF( "close", 0, nullptr ),		// name: string, callback: Function
	JS_CFUNC_DEF( "createElement", 0, nullptr ),		// name: string, callback: Function
	JS_CFUNC_DEF( "createTextNode", 0, nullptr ),		// name: string, callback: Function
	
	JS_CGETSET_DEF( "title", nullptr, NULL ),
	JS_CGETSET_DEF( "context",  nullptr, NULL ),
};

static const JSCFunctionListEntry _document_obj[] = {
	JS_OBJECT_DEF( "document", _functions, countof( _functions ), 0 ),
};


int setupDocument( JSContext* ctx ) 
{
	//JS_SetModuleLoaderFunc( JS_GetRuntime( ctx ), NULL, js_moduleLoader, NULL );

	//init_list_head( &__timers );
	//JS_NewClassID( &timer_class_id );

	//JS_NewClass( JS_GetRuntime( ctx ), timer_class_id, &timer_class );
	//JS_SetPropertyFunctionList( ctx, JS_GetGlobalObject( ctx ), _global_obj, countof( _global_obj ) );
	JS_SetPropertyFunctionList( ctx, JS_GetGlobalObject( ctx ), _document_obj, countof( _document_obj ) );
	return 0;
}



} // namespace Lua
} // namespace Rml
