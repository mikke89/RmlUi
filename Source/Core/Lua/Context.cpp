#include "precompiled.h"
#include "Context.h"
#include <Rocket/Core/Context.h>
#include <Rocket/Core/ElementDocument.h>

namespace Rocket {
namespace Core {
namespace Lua {
typedef Rocket::Core::ElementDocument Document;

//methods
int ContextAddEventListener(lua_State* L, Context* obj)
{
   //need to make an EventListener for Lua before I can do anything else
    return 0;
}

int ContextAddMouseCursor(lua_State* L, Context* obj)
{
    Document* cursor_doc = LuaType<Document>::check(L,1);
    obj->AddMouseCursor(cursor_doc);
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
    LuaType<Document>::push(L,doc,true);
    return 1;
}

int ContextLoadDocument(lua_State* L, Context* obj)
{
    const char* path = luaL_checkstring(L,1);
    Document* doc = obj->LoadDocument(path);
    LuaType<Document>::push(L,doc,true);
    return 1;
}

int ContextLoadMouseCursor(lua_State* L, Context* obj)
{
    const char* path = luaL_checkstring(L,1);
    Document* doc = obj->LoadMouseCursor(path);
    LuaType<Document>::push(L,doc);
    return 1;
}

int ContextRender(lua_State* L, Context* obj)
{
    lua_pushboolean(L,obj->Render());
    return 1;
}

int ContextShowMouseCursor(lua_State* L, Context* obj)
{
    bool show = CHECK_BOOL(L,1);
    obj->ShowMouseCursor(show);
    return 0;
}

int ContextUnloadAllDocuments(lua_State* L, Context* obj)
{
    obj->UnloadAllDocuments();
    return 0;
}

int ContextUnloadAllMouseCursors(lua_State* L, Context* obj)
{
    obj->UnloadAllMouseCursors();
    return 0;
}

int ContextUnloadDocument(lua_State* L, Context* obj)
{
    Document* doc = LuaType<Document>::check(L,1);
    obj->UnloadDocument(doc);
    return 0;
}

int ContextUnloadMouseCursor(lua_State* L, Context* obj)
{
    const char* name = luaL_checkstring(L,1);
    obj->UnloadMouseCursor(name);
    return 0;
}

int ContextUpdate(lua_State* L, Context* obj)
{
    lua_pushboolean(L,obj->Update());
    return 1;
}


//getters
int ContextGetAttrdimensions(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    const Vector2i* dim = &cont->GetDimensions();
    //const_cast-ing so that the user can do dimensions.x = 3 and it will actually change the dimensions
    //of the context
    LuaType<Vector2i>::push(L,const_cast<Vector2i*>(dim));
    return 1;
}

//returns a table of everything
int ContextGetAttrdocuments(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    Element* root = cont->GetRootElement();

    lua_newtable(L);
    int tableindex = lua_gettop(L);
    for(int i = 0; i < root->GetNumChildren(); i++)
    {
        Document* doc = root->GetChild(i)->GetOwnerDocument();
        if(doc == NULL)
            continue;

        LuaType<Document>::push(L,doc);
        lua_setfield(L, tableindex,doc->GetId().CString());

        /* //is this a bad idea?
        lua_pushinteger(L,i);
        lua_setfield(L,tableindex,doc->GetId().CString());
        */
    }

    return 1;
}

int ContextGetAttrfocus_element(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    LuaType<Element>::push(L,cont->GetFocusElement());
    return 1;
}

int ContextGetAttrhover_element(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    LuaType<Element>::push(L,cont->GetHoverElement());
    return 1;
}

int ContextGetAttrname(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);

    lua_pushstring(L,cont->GetName().CString());
    return 1;
}

int ContextGetAttrroot_element(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    LuaType<Element>::push(L,cont->GetRootElement());
    return 1;
}


//setters
int ContextSetAttrdimensions(lua_State* L)
{
    Context* cont = LuaType<Context>::check(L,1);
    Vector2i* dim = LuaType<Vector2i>::check(L,2);
    cont->SetDimensions(*dim);
    return 0;
}



RegType<Context> ContextMethods[] =
{
    LUAMETHOD(Context,AddEventListener)
    LUAMETHOD(Context,AddMouseCursor)
    LUAMETHOD(Context,CreateDocument)
    LUAMETHOD(Context,LoadDocument)
    LUAMETHOD(Context,LoadMouseCursor)
    LUAMETHOD(Context,Render)
    LUAMETHOD(Context,ShowMouseCursor)
    LUAMETHOD(Context,UnloadAllDocuments)
    LUAMETHOD(Context,UnloadAllMouseCursors)
    LUAMETHOD(Context,UnloadDocument)
    LUAMETHOD(Context,UnloadMouseCursor)
    LUAMETHOD(Context,Update)
    { NULL, NULL },
};

luaL_reg ContextGetters[] =
{
    LUAGETTER(Context,dimensions)
    LUAGETTER(Context,documents)
    LUAGETTER(Context,focus_element)
    LUAGETTER(Context,hover_element)
    LUAGETTER(Context,name)
    LUAGETTER(Context,root_element)
    { NULL, NULL },
};

luaL_reg ContextSetters[] =
{
    LUASETTER(Context,dimensions)
    { NULL, NULL },
};

/*
template<> const char* GetTClassName<Context>() { return "Context"; }
template<> RegType<Context>* GetMethodTable<Context>() { return ContextMethods; }
template<> luaL_reg* GetAttrTable<Context>() { return ContextGetters; }
template<> luaL_reg* SetAttrTable<Context>() { return ContextSetters; }
*/
}
}
}