/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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
 
#ifndef ROCKETCORELUACONTEXT_H
#define ROCKETCORELUACONTEXT_H
/*
    This defines a Context type in the Lua global namespace

    //methods
    noreturn Context:AddEventListener(string event, function | string listener, [Element element, bool capture]) --see note at the bottom
    noreturn Context:AddMouseCursor(Document cursor_document)
    Document Context:CreateDocument([string tag]) --tag defaults to "body"
    Document Context:LoadDocument(string path)
    Document Context:LoadMouseCursor(string path)
    bool Context:Render()
    noreturn Context:ShowMouseCursor(bool show)
    noreturn Context:UnloadAllDocuments()
    noreturn Context:UnloadAllMouseCursors()
    noreturn Context:UnloadDocument(Document doc)
    noreturn Context:UnloadMouseCursor(string name)
    bool Context:Update()

    //getters
    Vector2i Context.dimensions
    {} where key=string id,value=Document Context.documents
    Element Context.focus_element
    Element Context.hover_element
    string Context.name
    Element Context.root_element

    //setters
    Context.dimensions = Vector2i

	--note 1
	--[[
	Context:AddEventListener has 2 'unusuals'. The first is that the 2nd argument can be either a string or a function;
	see footnote 1 in Element.h for more info
	The second is the optional parameters. If you pass in an element (anything not nil), then it will actually call 
	element->AddEventListener and will call context->AddEventListener otherwise. capture will default to false
	]]

*/
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Context.h>


namespace Rocket {
namespace Core {
namespace Lua {

//methods
int ContextAddEventListener(lua_State* L, Context* obj);
int ContextAddMouseCursor(lua_State* L, Context* obj);
int ContextCreateDocument(lua_State* L, Context* obj);
int ContextLoadDocument(lua_State* L, Context* obj);
int ContextLoadMouseCursor(lua_State* L, Context* obj);
int ContextRender(lua_State* L, Context* obj);
int ContextShowMouseCursor(lua_State* L, Context* obj);
int ContextUnloadAllDocuments(lua_State* L, Context* obj);
int ContextUnloadAllMouseCursors(lua_State* L, Context* obj);
int ContextUnloadDocument(lua_State* L, Context* obj);
int ContextUnloadMouseCursor(lua_State* L, Context* obj);
int ContextUpdate(lua_State* L, Context* obj);

//getters
int ContextGetAttrdimensions(lua_State* L);
int ContextGetAttrdocuments(lua_State* L);
int ContextGetAttrfocus_element(lua_State* L);
int ContextGetAttrhover_element(lua_State* L);
int ContextGetAttrname(lua_State* L);
int ContextGetAttrroot_element(lua_State* L);

//setters
int ContextSetAttrdimensions(lua_State* L);


RegType<Context> ContextMethods[];
luaL_reg ContextGetters[];
luaL_reg ContextSetters[];

LUATYPEDECLARE(Context)
}
}
}
#endif
