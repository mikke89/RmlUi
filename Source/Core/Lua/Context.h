#pragma once
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
template<> bool LuaType<Context>::is_reference_counted();

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

/*
template<> const char* GetTClassName<Context>();
template<> RegType<Context>* GetMethodTable<Context>();
template<> luaL_reg* GetAttrTable<Context>();
template<> luaL_reg* SetAttrTable<Context>();
*/
}
}
}