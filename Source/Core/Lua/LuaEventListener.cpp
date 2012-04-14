#include "precompiled.h"
#include "LuaEventListener.h"
#include "Interpreter.h"
#include "LuaType.h"

namespace Rocket {
namespace Core {
namespace Lua {
typedef Rocket::Core::ElementDocument Document;

LuaEventListener::LuaEventListener(const String& code, Element* element) : EventListener()
{
    String function = "return function (event,element,document) ";
    function.Append(code);
    function.Append(" end");

    lua_State* L = Interpreter::GetLuaState();
    lua_getglobal(L,"EVENTLISTENERFUNCTIONS");
    if(lua_isnoneornil(L,-1))
    {
        lua_newtable(L);
        lua_setglobal(L,"EVENTLISTENERFUNCTIONS");
        lua_pop(L,1); //pop the unsucessful getglobal
        lua_getglobal(L,"EVENTLISTENERFUNCTIONS");
    }
    int tbl = lua_gettop(L);
    strFunc = lua_typename(L,lua_type(L,tbl));

    luaL_loadstring(L,function.CString()); //pushes the compiled string to the top of the stack
    if(lua_pcall(L,0,1,0) != 0)
        Interpreter::Report();
    strFunc = lua_typename(L,lua_type(L,-1));
    luaFuncRef = luaL_ref(L,tbl); //creates a reference to the item at the top of the stack in to the table we just created
    lua_pop(L,1); //pop the EVENTLISTENERFUNCTIONS table
    attached = element;
    parent = element->GetOwnerDocument();
    strFunc = function;
}

//if it is created from a Lua Element
LuaEventListener::LuaEventListener(int ref, Element* element)
{
    luaFuncRef = ref;
    attached = element;
    parent = element->GetOwnerDocument();
}

/// Process the incoming Event
void LuaEventListener::ProcessEvent(Event& event)
{
    lua_State* L = Interpreter::GetLuaState();
    String strtype;
    int top = lua_gettop(L); 
    //push the arguments
    lua_getglobal(L,"EVENTLISTENERFUNCTIONS");
    int table = lua_gettop(L); //needed for lua_remove
    strtype = lua_typename(L,lua_type(L,table));
    lua_rawgeti(L,table,luaFuncRef);
    strtype = lua_typename(L,lua_type(L,-1));
    strtype = lua_typename(L,lua_type(L,LuaType<Event>::push(L,&event,false)));
    strtype = lua_typename(L,lua_type(L,LuaType<Element>::push(L,attached,false)));
    strtype = lua_typename(L,lua_type(L,LuaType<Document>::push(L,parent,false)));
    
    Interpreter::ExecuteCall(3,0); //call the function at the top of the stack with 3 arguments

    lua_settop(L,top); //balanced stack makes Lua happy
    
}

}
}
}