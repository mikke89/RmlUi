#include "precompiled.h"
#include "Element.h"
#include <ElementStyle.h>
#include "LuaEventListener.h"


namespace Rocket {
namespace Core {
namespace Lua {
typedef ElementDocument Document;

//methods
int ElementAddEventListener(lua_State* L, Element* obj)
{
    int top = lua_gettop(L);
    bool capture;
    //default false if they didn't pass it in
    if (top < 3) capture = false;
    else capture = CHECK_BOOL(L,3);

    const char* event = luaL_checkstring(L,1);

    LuaEventListener* listener = NULL;
    int type = lua_type(L,2);
    if(type == LUA_TFUNCTION)
    {
        lua_pushvalue(L,2);
        int ref = lua_ref(L,true);
        listener = new LuaEventListener(ref,obj);
    }
    else if(type == LUA_TSTRING)
    {
        const char* code = luaL_checkstring(L,2);
        listener = new LuaEventListener(code,obj);
    }

    if(listener != NULL)
    {
        obj->AddEventListener(event,listener,capture);
    }
    return 0;
}

int ElementAppendChild(lua_State* L, Element* obj)
{
    Element* ele = LuaType<Element>::check(L,1);
    obj->AppendChild(ele);
    return 0;
}

int ElementBlur(lua_State* L, Element* obj)
{
    obj->Blur();
    return 0;
}

int ElementClick(lua_State* L, Element* obj)
{
    obj->Click();
    return 0;
}

int ElementDispatchEvent(lua_State* L, Element* obj)
{
    const char* event = luaL_checkstring(L,1);
    Dictionary params;
    lua_pushnil(L); //becauase lua_next pops a key from the stack first, we don't want to pop the table
    while(lua_next(L,2) != 0)
    {
        //[-1] is value, [-2] is key
        int type = lua_type(L,-1);
        const char* key = luaL_checkstring(L,-2); //key HAS to be a string, or things will go bad
        switch(type)
        {
		case LUA_TNUMBER:
            params.Set(key,(float)lua_tonumber(L,-1));
            break;
		case LUA_TBOOLEAN: 
            params.Set(key,CHECK_BOOL(L,-1));
            break;
		case LUA_TSTRING:
            params.Set(key,luaL_checkstring(L,-1));
            break;
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
            params.Set(key,lua_touserdata(L,-1));
            break;
        default:
            break;
        }
    }
    obj->DispatchEvent(event,params,false);
    return 0;
}

int ElementFocus(lua_State* L, Element* obj)
{
    obj->Focus();
    return 0;
}

int ElementGetAttribute(lua_State* L, Element* obj)
{
    const char* name = luaL_checkstring(L,1);
    Variant* var = obj->GetAttribute(name);
    Variant::Type type = var->GetType();
    switch(type)
    {
    case Variant::BYTE:
    case Variant::CHAR:
    case Variant::INT:
        lua_pushinteger(L,*(int*)var);
        break;
    case Variant::FLOAT:
        lua_pushnumber(L,*(float*)var);
        break;
    case Variant::COLOURB:
        LuaType<Colourb>::push(L,(Colourb*)var,false);
        break;
    case Variant::COLOURF:
        LuaType<Colourf>::push(L,(Colourf*)var,false);
        break;
    case Variant::STRING:
        lua_pushstring(L,((String*)var)->CString());
        break;
    case Variant::VECTOR2:
        //according to Variant.inl, it is going to be a Vector2f
        LuaType<Vector2f>::push(L,((Vector2f*)var),false);
        break;
    case Variant::VOIDPTR:
        lua_pushlightuserdata(L,(void*)var);
        break;
    default:
        lua_pushnil(L);
        break;
    }
    return 1;
}

int ElementGetElementById(lua_State* L, Element* obj)
{
    const char* id = luaL_checkstring(L,1);
    Element* ele = obj->GetElementById(id);
    LuaType<Element>::push(L,ele,false);
    return 1;
}

int ElementGetElementsByTagName(lua_State* L, Element* obj)
{
    const char* tag = luaL_checkstring(L,1);
    ElementList list;
    obj->GetElementsByTagName(list,tag);
    lua_newtable(L);
    for(unsigned int i = 0; i < list.size(); i++)
    {
        lua_pushinteger(L,i);
        LuaType<Element>::push(L,list[i],false);
        lua_settable(L,-3); //-3 is the table
    }
    return 1;
}

int ElementHasAttribute(lua_State* L, Element* obj)
{
    const char* name = luaL_checkstring(L,1);
    lua_pushboolean(L,obj->HasAttribute(name));
    return 1;
}

int ElementHasChildNodes(lua_State* L, Element* obj)
{
    lua_pushboolean(L,obj->HasChildNodes());
    return 1;
}

int ElementInsertBefore(lua_State* L, Element* obj)
{
    Element* element = LuaType<Element>::check(L,1);
    Element* adjacent = LuaType<Element>::check(L,2);
    obj->InsertBefore(element,adjacent);
    return 0;
}

int ElementIsClassSet(lua_State* L, Element* obj)
{
    const char* name = luaL_checkstring(L,1);
    lua_pushboolean(L,obj->IsClassSet(name));
    return 1;
}

int ElementRemoveAttribute(lua_State* L, Element* obj)
{
    const char* name = luaL_checkstring(L,1);
    obj->RemoveAttribute(name);
    return 0;
}

int ElementRemoveChild(lua_State* L, Element* obj)
{
    Element* element = LuaType<Element>::check(L,1);
    lua_pushboolean(L,obj->RemoveChild(element));
    return 1;
}

int ElementReplaceChild(lua_State* L, Element* obj)
{
    Element* inserted = LuaType<Element>::check(L,1);
    Element* replaced = LuaType<Element>::check(L,2);
    lua_pushboolean(L,obj->ReplaceChild(inserted,replaced));
    return 1;
}

int ElementScrollIntoView(lua_State* L, Element* obj)
{
    bool align = CHECK_BOOL(L,1);
    obj->ScrollIntoView(align);
    return 0;
}

int ElementSetAttribute(lua_State* L, Element* obj)
{
    LUACHECKOBJ(obj);
    const char* name = luaL_checkstring(L,1);
    const char* value = luaL_checkstring(L,2);
    obj->SetAttribute(name,String(value));
    return 0;
}

int ElementSetClass(lua_State* L, Element* obj)
{
    const char* name = luaL_checkstring(L,1);
    bool value = CHECK_BOOL(L,2);
    obj->SetClass(name,value);
    return 0;
}


//getters
int ElementGetAttrattributes(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    
    int index;
    String key;
    Variant* var;
    Variant::Type type;

    lua_newtable(L);
    while(ele->IterateAttributes(index,key,var))
    {
        lua_pushstring(L,key.CString());
        type = var->GetType();
        switch(type)
        {
        case Variant::BYTE:
        case Variant::CHAR:
        case Variant::INT:
            lua_pushinteger(L,*(int*)var);
            break;
        case Variant::FLOAT:
            lua_pushnumber(L,*(float*)var);
            break;
        case Variant::COLOURB:
            LuaType<Colourb>::push(L,(Colourb*)var,false);
            break;
        case Variant::COLOURF:
            LuaType<Colourf>::push(L,(Colourf*)var,false);
            break;
        case Variant::STRING:
            lua_pushstring(L,((String*)var)->CString());
            break;
        case Variant::VECTOR2:
            //according to Variant.inl, it is going to be a Vector2f
            LuaType<Vector2f>::push(L,((Vector2f*)var),false);
            break;
        case Variant::VOIDPTR:
            lua_pushlightuserdata(L,(void*)var);
            break;
        default:
            lua_pushnil(L);
            break;
        }   
        lua_settable(L,-3);
    }
    
    return 1;
}

int ElementGetAttrchild_nodes(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    if(!ele->HasChildNodes())
        lua_pushnil(L);
    else
    {
        lua_newtable(L);
        int index = 0;
        int num_of_children = ele->GetNumChildren();
        while(index < num_of_children)
        {
            lua_pushinteger(L,index);
            LuaType<Element>::push(L,ele->GetChild(index),false);
            lua_settable(L,-3);
            ++index;
        }
    }
    return 1;
}

int ElementGetAttrclass_name(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    const char* classnames = ele->GetClassNames().CString();
    lua_pushstring(L,classnames);
    return 1;
}

int ElementGetAttrclient_left(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetClientLeft());
    return 1;
}

int ElementGetAttrclient_height(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetClientHeight());
    return 1;
}

int ElementGetAttrclient_top(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetClientTop());
    return 1;
}

int ElementGetAttrclient_width(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetClientWidth());
    return 1;
}

int ElementGetAttrfirst_child(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    Element* child = ele->GetFirstChild();
    if(child == NULL)
        lua_pushnil(L);
    else
        LuaType<Element>::push(L,child,false);
    return 1;
}

int ElementGetAttrid(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushstring(L,ele->GetId().CString());
    return 1;
}

int ElementGetAttrinner_rml(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushstring(L,ele->GetInnerRML().CString());
    return 1;
}

int ElementGetAttrlast_child(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    Element* child = ele->GetLastChild();
    if(child == NULL)
        lua_pushnil(L);
    else
        LuaType<Element>::push(L,child,false);
    return 1;
}

int ElementGetAttrnext_sibling(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    Element* sibling = ele->GetNextSibling();
    if(sibling == NULL)
        lua_pushnil(L);
    else
        LuaType<Element>::push(L,sibling,false);
    return 1;
}

int ElementGetAttroffset_height(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetOffsetHeight());
    return 1;
}

int ElementGetAttroffset_left(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetOffsetLeft());
    return 1;
}

int ElementGetAttroffset_parent(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    Element* parent = ele->GetOffsetParent();
    LuaType<Element>::push(L,parent,false);
    return 1;
}

int ElementGetAttroffset_top(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L, ele->GetOffsetTop());
    return 1;
}

int ElementGetAttroffset_width(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetOffsetWidth());
    return 1;
}

int ElementGetAttrowner_document(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    Document* doc = ele->GetOwnerDocument();
    LuaType<Document>::push(L,doc,false);
    return 1;
}

int ElementGetAttrparent_node(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    Element* parent = ele->GetParentNode();
    if(parent == NULL)
        lua_pushnil(L);
    else
        LuaType<Element>::push(L,parent,false);
    return 1;
}

int ElementGetAttrprevious_sibling(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    Element* sibling = ele->GetPreviousSibling();
    if(sibling == NULL)
        lua_pushnil(L);
    else
        LuaType<Element>::push(L,sibling,false);
    return 1;
}

int ElementGetAttrscroll_height(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetScrollHeight());
    return 1;
}

int ElementGetAttrscroll_left(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetScrollLeft());
    return 1;
}

int ElementGetAttrscroll_top(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetScrollTop());
    return 1;
}

int ElementGetAttrscroll_width(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushnumber(L,ele->GetScrollWidth());
    return 1;
}

int ElementGetAttrstyle(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    LuaType<ElementStyle>::push(L,ele->GetStyle(),false);
    return 1;
}

int ElementGetAttrtag_name(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    lua_pushstring(L,ele->GetTagName().CString());
    return 0;
}


//setters
int ElementSetAttrclass_name(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    const char* name = luaL_checkstring(L,2);
    ele->SetClassNames(name);
    return 0;
}

int ElementSetAttrid(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    const char* id = luaL_checkstring(L,2);
    ele->SetId(id);
    return 0;
}

int ElementSetAttrinner_rml(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    const char* rml = luaL_checkstring(L,2);
    ele->SetInnerRML(rml);
    return 0;
}

int ElementSetAttrscroll_left(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    float scroll = (float)luaL_checknumber(L,2);
    ele->SetScrollLeft(scroll);
    return 0;
}

int ElementSetAttrscroll_top(lua_State* L)
{
    Element* ele = LuaType<Element>::check(L,1);
    float scroll = (float)luaL_checknumber(L,2);
    ele->SetScrollTop(scroll);
    return 0;
}




RegType<Element> ElementMethods[] =
{
    LUAMETHOD(Element,AddEventListener)
    LUAMETHOD(Element,AppendChild)
    LUAMETHOD(Element,Blur)
    LUAMETHOD(Element,Click)
    LUAMETHOD(Element,DispatchEvent)
    LUAMETHOD(Element,Focus)
    LUAMETHOD(Element,GetAttribute)
    LUAMETHOD(Element,GetElementById)
    LUAMETHOD(Element,GetElementsByTagName)
    LUAMETHOD(Element,HasAttribute)
    LUAMETHOD(Element,HasChildNodes)
    LUAMETHOD(Element,InsertBefore)
    LUAMETHOD(Element,IsClassSet)
    LUAMETHOD(Element,RemoveAttribute)
    LUAMETHOD(Element,RemoveChild)
    LUAMETHOD(Element,ReplaceChild)
    LUAMETHOD(Element,ScrollIntoView)
    LUAMETHOD(Element,SetAttribute)
    LUAMETHOD(Element,SetClass)
    { NULL, NULL },
};

luaL_reg ElementGetters[] =
{
    LUAGETTER(Element,attributes)
    LUAGETTER(Element,child_nodes)
    LUAGETTER(Element,class_name)
    LUAGETTER(Element,client_left)
    LUAGETTER(Element,client_height)
    LUAGETTER(Element,client_top)
    LUAGETTER(Element,client_width)
    LUAGETTER(Element,first_child)
    LUAGETTER(Element,id)
    LUAGETTER(Element,inner_rml)
    LUAGETTER(Element,last_child)
    LUAGETTER(Element,next_sibling)
    LUAGETTER(Element,offset_height)
    LUAGETTER(Element,offset_left)
    LUAGETTER(Element,offset_parent)
    LUAGETTER(Element,offset_top)
    LUAGETTER(Element,offset_width)
    LUAGETTER(Element,owner_document)
    LUAGETTER(Element,parent_node)
    LUAGETTER(Element,previous_sibling)
    LUAGETTER(Element,scroll_height)
    LUAGETTER(Element,scroll_left)
    LUAGETTER(Element,scroll_top)
    LUAGETTER(Element,scroll_width)
    LUAGETTER(Element,style)
    LUAGETTER(Element,tag_name)
    { NULL, NULL },
};

luaL_reg ElementSetters[] =
{
    LUASETTER(Element,class_name)
    LUASETTER(Element,id)
    LUASETTER(Element,inner_rml)
    LUASETTER(Element,scroll_left)
    LUASETTER(Element,scroll_top)
    { NULL, NULL },
};

/*
template<> const char* GetTClassName<Element>() { return "Element"; }
template<> RegType<Element>* GetMethodTable<Element>() { return ElementMethods; }
template<> luaL_reg* GetAttrTable<Element>() { return ElementGetters; }
template<> luaL_reg* SetAttrTable<Element>() { return ElementSetters; }
*/
}
}
}