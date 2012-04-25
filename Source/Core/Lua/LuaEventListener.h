#pragma once
#include <Rocket/Core/EventListener.h>
#include <Rocket/Core/String.h>
#include <Rocket/Core/Lua/lua.hpp>

namespace Rocket {
namespace Core {
namespace Lua {

class LuaEventListener : public EventListener
{
public:
    //The plan is to wrap the code in an anonymous function so that we can have named parameters to use,
    //rather than putting them in global variables
    LuaEventListener(const String& code, Element* element);

    //This is called from a Lua Element if in element:AddEventListener it passes a function in as the 2nd
    //parameter rather than a string. We don't wrap the function in an anonymous function, so the user
    //should take care to have the proper order. The order is event,element,document.
	//narg is the position on the stack
    LuaEventListener(lua_State* L, int narg, Element* element);

    virtual ~LuaEventListener();

    /// Process the incoming Event
	virtual void ProcessEvent(Event& event);
private:
    //the lua-side function to call when ProcessEvent is called
    int luaFuncRef;
    Element* attached;
    ElementDocument* parent;
    String strFunc; //for debugging purposes
};

}
}
}