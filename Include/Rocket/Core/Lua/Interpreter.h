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
    static void LoadFile(const Rocket::Core::String& file);
    static void DoString(const Rocket::Core::String& str);
    static void Report();

    static void BeginCall(int funRef);
    static bool ExecuteCall(int params = 0, int res = 0);
    static void EndCall(int res = 0);

    static lua_State* GetLuaState();

    static void Initialise();
	static void Shutdown();
    
    //From Plugin
    virtual int GetEventClasses();
    virtual void OnInitialise();
    virtual void OnShutdown();
private:
    //This will populate the global Lua table with all of the Lua types and some global functions
    static inline void RegisterEverything(lua_State* L);
    void Startup();

    static lua_State* _L;
};
}
}
}
#endif

