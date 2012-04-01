#include "precompiled.h"
#include "Document.h"
#include <Rocket/Core/ElementDocument.h>
#include <Rocket/Core/Context.h>

namespace Rocket {
namespace Core {
namespace Lua {

//methods
int DocumentPullToFront(lua_State* L, Document* obj)
{
    obj->PullToFront();
    return 0;
}

int DocumentPushToBack(lua_State* L, Document* obj)
{
    obj->PushToBack();
    return 0;
}

int DocumentShow(lua_State* L, Document* obj)
{
    int top = lua_gettop(L);
    if(top == 0)
        obj->Show();
    else
    {
        int flag = luaL_checkinteger(L,1);
        obj->Show(flag);
    }
    return 0;
}

int DocumentHide(lua_State* L, Document* obj)
{
    obj->Hide();
    return 0;
}

int DocumentClose(lua_State* L, Document* obj)
{
    obj->Close();
    return 0;
}

int DocumentCreateElement(lua_State* L, Document* obj)
{
    const char* tag = luaL_checkstring(L,1);
    Element* ele = obj->CreateElement(tag);
    LuaType<Element>::push(L,ele,false);
    return 1;
}

int DocumentCreateTextNode(lua_State* L, Document* obj)
{
    //need ElementText object first
	return 0;
}


//getters
int DocumentGetAttrtitle(lua_State* L)
{
    Document* doc = LuaType<Document>::check(L,1);
    lua_pushstring(L,doc->GetTitle().CString());
    return 1;
}

int DocumentGetAttrcontext(lua_State* L)
{
    Document* doc = LuaType<Document>::check(L,1);
    LuaType<Context>::push(L,doc->GetContext(),false);
    return 1;
}


//setters
int DocumentSetAttrtitle(lua_State* L)
{
    Document* doc = LuaType<Document>::check(L,1);
    const char* title = luaL_checkstring(L,2);
    doc->SetTitle(title);
    return 0;
}


RegType<Document> DocumentMethods[] =
{
    LUAMETHOD(Document,PullToFront)
    LUAMETHOD(Document,PushToBack)
    LUAMETHOD(Document,Show)
    LUAMETHOD(Document,Hide)
    LUAMETHOD(Document,Close)
    LUAMETHOD(Document,CreateElement)
    LUAMETHOD(Document,CreateTextNode)
    { NULL, NULL },
};

luaL_reg DocumentGetters[] =
{
    LUAGETTER(Document,title)
    LUAGETTER(Document,context)
    { NULL, NULL },
};

luaL_reg DocumentSetters[] =
{
    LUASETTER(Document,title)
    { NULL, NULL },
};

/*
template<> const char* GetTClassName<Document>() { return "Document"; }
template<> RegType<Document>* GetMethodTable<Document>() { return DocumentMethods; }
template<> luaL_reg* GetAttrTable<Document>() { return DocumentGetters; }
template<> luaL_reg* SetAttrTable<Document>() { return DocumentSetters; }
*/
}
}
}