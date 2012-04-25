#ifndef ROCKETCORELUAINTERPRETER_H
#define ROCKETCORELUALUATYPE_H 
/*
    This initializes the lua interpreter, and has functions to load the scripts
    A glorified namespace, but I want the lua_State to be unchangeable
*/
#include "Header.h"
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Plugin.h>

namespace Rocket {
namespace Core {
namespace Lua {

class ROCKETLUA_API Interpreter : public Plugin
{
public:
    //Somewhat misleading name if you are used to the Lua meaning of Load, but this function calls
    //luaL_loadfile and then lua_pcall, reporting the errors (if any)
    static void LoadFile(const Rocket::Core::String& file);
    //Calls lua_dostring and reports the errors. The first parameter is the string to execute, the
    //second parameter is the name, which will show up in the console/error log
    static void DoString(const Rocket::Core::String& code, const Rocket::Core::String& name = "");
    //Same as DoString, except does NOT call pcall on it. It will leave the compiled (but not executed) string
    //on top of the stack. It is just like luaL_loadstring, but you get to specify the name
    static void LoadString(const Rocket::Core::String& code, const Rocket::Core::String& name = "");
    //If there are errors on the top of the stack, this will print those out to the log.
    static void Report();

    //clears all of the items on the stack, and pushes the function from funRef on top of the stack. Only use
    //this if you used lua_ref instead of luaL_ref
    static void BeginCall(int funRef);
    /*  Before you call this, your stack should look like:
    [0] function to call
    [1...top] parameters to pass to the function (if any)
    Or, in words, make sure to push the function on the stack before the parameters. After this function, the params and function will
    be popped off, and 'res' number of items will be pushed.
    */
    static bool ExecuteCall(int params = 0, int res = 0);
    //removes 'res' number of items from the stack
    static void EndCall(int res = 0);

    //This lacks a SetLuaState for a reason. If you have to use your own Lua binding, then use this lua_State; it will
    //already have all of the libraries loaded, and all of the types defined
    static lua_State* GetLuaState();

    //Creates the plugin
    static void Initialise();
    //removes the plugin
	static void Shutdown();
    
    //From Plugin
    virtual int GetEventClasses();
    virtual void OnInitialise();
    virtual void OnShutdown();
private:
    //This will populate the global Lua table with all of the Lua types and some global functions
    static inline void RegisterEverything(lua_State* L);
    void Startup(); //loads default libs

    static lua_State* _L;
};
}
}
}
#endif

