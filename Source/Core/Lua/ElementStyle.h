#pragma once
/*
    This is the definition of an ElementStyle Lua object

    This object is returned when you have an element, and access Element.style
    You can access it like a regular table, with the exception of iterating
    over the properties.

    A few peculiarities/rules:
    You are unable to create an instance of this object, only use one returned by
    the "style" value of an 'Element' item. Of course, you can store the object in
    a local variable in Lua and still have everything be valid

    When setting a property of a style, both the key and value need to be strings
    --Examples assume "e" is an element object
    e.style["width"] = "40px"
    e.style.width = "40px"  --does the same as above


    When getting a property of a style, it spits out exactly what you put in.
    If you used the above width setting to "40px" then
    --Examples assume "e" is an element object
    local w = e.style["width"] --or e.style.width
    w would be "40px"


    If you need to iterate over the values, you'll have to call style:GetTable() 
    Because of the way that I made the object, the following will not work
    for k,v in pairs(element.style) do --assumes "element" is an element object
        print(k .. " " ..v)
    end
    This is because I don't actually have any properties stored in the Lua table
    To do it, you'd have to say
    local properties = element.style:GetTable() --assumes "element" is an element object
    for k,v in pairs(properties) do
        print(k .. " " .. v)
    end
    However, the table returned from style:GetTable() is read only. Whatever changes you
    make to that table will not be saved to the 'style' object. To do that, just do whatever
    operation you were going to on the table before, but do it on the 'style' object instead.
*/
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <ElementStyle.h>

namespace Rocket {
namespace Core {
namespace Lua {
template<> void LuaType<ElementStyle>::extra_init(lua_State* L, int metatable_index);
int ElementStyle__index(lua_State* L);
int ElementStyle__newindex(lua_State* L);

//methods
int ElementStyleGetTable(lua_State* L, ElementStyle* obj);

RegType<ElementStyle> ElementStyleMethods[];
luaL_reg ElementStyleGetters[];
luaL_reg ElementStyleSetters[];

/*
template<> const char* GetTClassName<ElementStyle>();
template<> RegType<ElementStyle>* GetMethodTable<ElementStyle>();
template<> luaL_reg* GetAttrTable<ElementStyle>();
template<> luaL_reg* SetAttrTable<ElementStyle>();
*/
}
}
}