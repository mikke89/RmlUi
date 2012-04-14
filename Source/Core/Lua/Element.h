#pragma once
/*
    This defines the Element type in the Lua global namespace

    A few classes "inherit" from Element, such as Document. Document will call
    LuaType<Element>::_regfunctions to put all of these functions in its own table, and
    will be used with the same syntax except it will be from a Document object. It isn't true
    inheritance, but it is a fair enough emulation. Any functions in the child class that have
    the same name as the function in Element will overwrite the one in Element.

    Here, I will be showing usage of the API, and it will show the type names rather than the regular
    local var = foo that is Lua. If you need info on the purpose of the functions, see the python docs
    
    //methods that need to be called from an Element object using the colon syntax
    noreturn Element:AddEventListener(string event, ? see footnote 1, [bool capture])
    noreturn Element:AppendChild(Element child)
    noreturn Element:Blur()
    noreturn Element:Click()
    noreturn Element:DispatchEvent(string event, {} params) --in params, keys have to be a string and value can be number,bool,string,userdata,lightuserdata
    noreturn Element:Focus()
    [int,float,Colourb,Colourf,string,Vector2f,lightuserdata] Element:GetAttribute(string name) --will return one of those types
    Element Element:GetElementById(string id)
    {}of elements Element:GetElementsByTagName(string tag)
    bool Element:HasAttribute(string name)
    bool Element:HasChildNodes()
    noreturn Element:InsertBefore(Element child,Element adjacent)
    bool Element:IsClassSet(string name)
    noreturn Element:RemoveAttribute(string name)
    bool Element:RemoveChild(Element child)
    bool Element:ReplaceChild(Element inserted,Element replaced)
    noreturn Element:ScrollIntoView(bool align_with_top)
    noreturn Element:SetAttribute(string name,string value)
    noreturn Element:SetClass(string name, bool activate)


    //getters accessed by the period syntax from an element object
    --for attributes, if you save it to a local/global variable and try to modify that variable,
    --your changes will not be saved. You will have to use Element:SetAttribute
    {} of [key=string,value=int,float,Colourb,Colourf,string,Vector2f,lightuserdata] Element.attributes
    {} of Element Element.child_nodes
    string Element.class_name
    float Element.client_left
    float Element.client_height
    float Element.client_top
    float Element.client_width
    Element Element.first_child
    string Element.id
    string Element.inner_rml
    Element Element.last_child
    Element Element.next_sibling
    float Element.offset_height
    float Element.offset_left
    Element Element.offset_parent
    float Element.offset_top
    float Element.offset_width
    Document Element.owner_document
    Element Element.parent_nod
    Element Element.previous_sibling
    float Element.scroll_height
    float Element.scroll_left
    float Element.scroll_top
    float Element.scroll_width
    ElementStyle Element.style --see ElementStyle.h documentation
    string Element.tag_name

    //setters to be used with a dot syntax on an Element object
    Element.class_name = string
    Element.id = string
    Element.inner_rml = string
    Element.scroll_left = float
    Element.scroll_top = float  


    footnote 1: for Element:AddEventListener(string,?,bool)
    The ? can be either a string or a function. 
    In the string, you can be guaranteed that you will have the
    named variables 'event','element','document' available to you, and they mean the same as if you were to put
    the string as onclick="string" in a .rml file.
    If you give it a function, the function will be called every time that C++ EventListener::ProcessEvent would
    would be called. In this case, it will call the function, and you can decide the name of the parameters, however
    it is in a specific order. The order is event,element,document. So:
    function foo(l,q,e) end element:AddEventListener("click",foo,true) is the correct syntax, and puts l=event,q=element,e=document
    They are terrible names, but it is to make a point.
*/
#include "LuaType.h"
#include "lua.hpp"
#include <Rocket/Core/Element.h>

namespace Rocket {
namespace Core {
namespace Lua {
template<> bool LuaType<Element>::is_reference_counted();

//methods
int ElementAddEventListener(lua_State* L, Element* obj);
int ElementAppendChild(lua_State* L, Element* obj);
int ElementBlur(lua_State* L, Element* obj);
int ElementClick(lua_State* L, Element* obj);
int ElementDispatchEvent(lua_State* L, Element* obj);
int ElementFocus(lua_State* L, Element* obj);
int ElementGetAttribute(lua_State* L, Element* obj);
int ElementGetElementById(lua_State* L, Element* obj);
int ElementGetElementsByTagName(lua_State* L, Element* obj);
int ElementHasAttribute(lua_State* L, Element* obj);
int ElementHasChildNodes(lua_State* L, Element* obj);
int ElementInsertBefore(lua_State* L, Element* obj);
int ElementIsClassSet(lua_State* L, Element* obj);
int ElementRemoveAttribute(lua_State* L, Element* obj);
int ElementRemoveChild(lua_State* L, Element* obj);
int ElementReplaceChild(lua_State* L, Element* obj);
int ElementScrollIntoView(lua_State* L, Element* obj);
int ElementSetAttribute(lua_State* L, Element* obj);
int ElementSetClass(lua_State* L, Element* obj);

//getters
int ElementGetAttrattributes(lua_State* L);
int ElementGetAttrchild_nodes(lua_State* L);
int ElementGetAttrclass_name(lua_State* L);
int ElementGetAttrclient_left(lua_State* L);
int ElementGetAttrclient_height(lua_State* L);
int ElementGetAttrclient_top(lua_State* L);
int ElementGetAttrclient_width(lua_State* L);
int ElementGetAttrfirst_child(lua_State* L);
int ElementGetAttrid(lua_State* L);
int ElementGetAttrinner_rml(lua_State* L);
int ElementGetAttrlast_child(lua_State* L);
int ElementGetAttrnext_sibling(lua_State* L);
int ElementGetAttroffset_height(lua_State* L);
int ElementGetAttroffset_left(lua_State* L);
int ElementGetAttroffset_parent(lua_State* L);
int ElementGetAttroffset_top(lua_State* L);
int ElementGetAttroffset_width(lua_State* L);
int ElementGetAttrowner_document(lua_State* L);
int ElementGetAttrparent_node(lua_State* L);
int ElementGetAttrprevious_sibling(lua_State* L);
int ElementGetAttrscroll_height(lua_State* L);
int ElementGetAttrscroll_left(lua_State* L);
int ElementGetAttrscroll_top(lua_State* L);
int ElementGetAttrscroll_width(lua_State* L);
int ElementGetAttrstyle(lua_State* L);
int ElementGetAttrtag_name(lua_State* L);

//setters
int ElementSetAttrclass_name(lua_State* L);
int ElementSetAttrid(lua_State* L);
int ElementSetAttrinner_rml(lua_State* L);
int ElementSetAttrscroll_left(lua_State* L);
int ElementSetAttrscroll_top(lua_State* L);



RegType<Element> ElementMethods[];
luaL_reg ElementGetters[];
luaL_reg ElementSetters[];

/*
template<> const char* GetTClassName<Element>();
template<> RegType<Element>* GetMethodTable<Element>();
template<> luaL_reg* GetAttrTable<Element>();
template<> luaL_reg* SetAttrTable<Element>();
*/
}
}
}