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
    String function = "function (event,element,document) ";
    function.Append(code);
    function.Append("end");

    lua_State* L = Interpreter::GetLuaState();

    luaL_loadstring(L,function.CString()); //pushes the compiled string to the top of the stack
    luaFuncRef = lua_ref(L,true); //creates a reference to the item at the top of the stack
    attached = element;
    parent = element->GetOwnerDocument();
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
    int top = lua_gettop(L); 
    //push the arguments
    LuaType<Event>::push(L,&event,false);
    LuaType<Element>::push(L,attached,false);
    LuaType<Document>::push(L,parent,false);
    
    lua_getref(L,luaFuncRef);
    Interpreter::ExecuteCall(3,0); //call the function at the top of the stack with 3 arguments

    lua_settop(L,top); //balanced stack makes Lua happy
    
}

}
}
}