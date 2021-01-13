/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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
 
#include "Context.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include "LuaEventListener.h"
#include "ContextDocumentsProxy.h"
#include "LuaDataModel.h"

namespace Rml {
namespace Lua {
typedef ElementDocument Document;
template<> void ExtraInit<Context>(lua_State* /*L*/, int /*metatable_index*/) { return; }

//methods
int ContextAddEventListener(lua_State* L, Context* obj)
{
   //need to make an EventListener for Lua before I can do anything else
	const char* evt = luaL_checkstring(L,1); //event
	Element* element = nullptr;
	bool capturephase = false;
	//get the rest of the stuff needed to construct the listener
	if(lua_gettop(L) > 2)
	{
		if(!lua_isnoneornil(L,3))
			element = LuaType<Element>::check(L,3);
		if(!lua_isnoneornil(L,4))
			capturephase = RMLUI_CHECK_BOOL(L,4);

	}
	int type = lua_type(L,2);
	if(type == LUA_TFUNCTION)
	{
		if(element)
			element->AddEventListener(evt, new LuaEventListener(L,2,element), capturephase);
		else
			obj->AddEventListener(evt, new LuaEventListener(L,2,nullptr), capturephase);
	}
	else if(type == LUA_TSTRING)
	{
		if(element)
			element->AddEventListener(evt, new LuaEventListener(luaL_checkstring(L,2),element), capturephase);
		else
			obj->AddEventListener(evt, new LuaEventListener(luaL_checkstring(L,2),nullptr), capturephase);
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Lua Context:AddEventLisener's 2nd argument can only be a Lua function or a string, you passed in a %s", lua_typename(L,type));
	}
    return 0;
}

int ContextCreateDocument(lua_State* L, Context* obj)
{
    const char* tag;
    if(lua_gettop(L) < 1)
        tag = "body";
    else
        tag = luaL_checkstring(L,1);
    Document* doc = obj->CreateDocument(tag);
    LuaType<Document>::push(L,doc,false);
    return 1;
}

int ContextLoadDocument(lua_State* L, Context* obj)
{
    const char* path = luaL_checkstring(L,1);
    Document* doc = obj->LoadDocument(path);
    LuaType<Document>::push(L,doc,false);
    return 1;
}

int ContextRender(lua_State* L, Context* obj)
{
    lua_pushboolean(L,obj->Render());
    return 1;
}

int ContextUnloadAllDocuments(lua_State* /*L*/, Context* obj)
{
    obj->UnloadAllDocuments();
    return 0;
}

int ContextUnloadDocument(lua_State* L, Context* obj)
{
    Document* doc = LuaType<Document>::check(L,1);
    obj->UnloadDocument(doc);
    return 0;
}

int ContextUpdate(lua_State* L, Context* obj)
{
    lua_pushboolean(L,obj->Update());
    return 1;
}

int ContextOpenDataModel(lua_State *L, Context *obj)
{
	if (!OpenLuaDataModel(L, obj, 1, 2)) {
		// Open fails
		lua_pushboolean(L, false);
	}
	return 1;
}

//getters
int ContextGetAttrdimensions(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    Vector2i* dim = new Vector2i(cont->GetDimensions());
    LuaType<Vector2i>::push(L,dim,true);
    return 1;
}

//returns a table of everything
int ContextGetAttrdocuments(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    RMLUI_CHECK_OBJ(cont);
    ContextDocumentsProxy* cdp = new ContextDocumentsProxy();
    cdp->owner = cont;
    LuaType<ContextDocumentsProxy>::push(L,cdp,true); //does get garbage collected (deleted)
    return 1;
}

int ContextGetAttrfocus_element(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    RMLUI_CHECK_OBJ(cont);
    LuaType<Element>::push(L,cont->GetFocusElement());
    return 1;
}

int ContextGetAttrhover_element(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    RMLUI_CHECK_OBJ(cont);
    LuaType<Element>::push(L,cont->GetHoverElement());
    return 1;
}

int ContextGetAttrname(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    RMLUI_CHECK_OBJ(cont);
    lua_pushstring(L,cont->GetName().c_str());
    return 1;
}

int ContextGetAttrroot_element(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    RMLUI_CHECK_OBJ(cont);
    LuaType<Element>::push(L,cont->GetRootElement());
    return 1;
}


//setters
int ContextSetAttrdimensions(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    RMLUI_CHECK_OBJ(cont);
    Vector2i* dim = LuaType<Vector2i>::check(L,2);
    cont->SetDimensions(*dim);
    return 0;
}


RegType<Context> ContextMethods[] =
{
    RMLUI_LUAMETHOD(Context,AddEventListener)
    RMLUI_LUAMETHOD(Context,CreateDocument)
    RMLUI_LUAMETHOD(Context,LoadDocument)
    RMLUI_LUAMETHOD(Context,Render)
    RMLUI_LUAMETHOD(Context,UnloadAllDocuments)
    RMLUI_LUAMETHOD(Context,UnloadDocument)
    RMLUI_LUAMETHOD(Context,Update)
	RMLUI_LUAMETHOD(Context,OpenDataModel)
	// todo: CloseDataModel
    { nullptr, nullptr },
};

luaL_Reg ContextGetters[] =
{
    RMLUI_LUAGETTER(Context,dimensions)
    RMLUI_LUAGETTER(Context,documents)
    RMLUI_LUAGETTER(Context,focus_element)
    RMLUI_LUAGETTER(Context,hover_element)
    RMLUI_LUAGETTER(Context,name)
    RMLUI_LUAGETTER(Context,root_element)
    { nullptr, nullptr },
};

luaL_Reg ContextSetters[] =
{
    RMLUI_LUASETTER(Context,dimensions)
    { nullptr, nullptr },
};

RMLUI_LUATYPE_DEFINE(Context)
} // namespace Lua
} // namespace Rml
