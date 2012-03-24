#include "precompiled.h"
#include "Interpreter.h"
#include <Rocket/Core/Log.h>
#include <Rocket/Core/String.h>
#include "LuaType.h"
#include "LuaDocumentElementInstancer.h"
#include <Rocket/Core/Factory.h>

namespace Rocket {
namespace Core {
namespace Lua {
lua_State* Interpreter::_L = NULL;
//forward declaration of the types unique to the lua portion
class rocket;

void Interpreter::Startup()
{
    Log::Message(Log::LT_INFO, "Loading Lua interpreter");
    _L = luaL_newstate();
    luaL_openlibs(_L);

    RegisterEverything(_L);
}


void Interpreter::RegisterEverything(lua_State* L)
{
#include "Register.h" //think of it as a glorified macro file
}



void Interpreter::LoadFile(const String& file)
{
    String msg = "Loading";
    if(luaL_loadfile(_L, file.CString()) != 0)
    {
        msg.Append(" failed. Could not load. ").Append(file);
        Log::Message(Log::LT_ERROR, msg.CString());
        Report();
    }
    else
    {
        if(lua_pcall(_L,0,0,0) != 0)
        {
            msg.Append(" failed. Could not run. ").Append(file);
            Log::Message(Log::LT_ERROR, msg.CString());
            Report();
        }
        else
        {
            msg.Append(" was successful. ").Append(file);
            Log::Message(Log::LT_INFO, msg.CString());
        }
    }
}


void Interpreter::DoString(const Rocket::Core::String& str)
{
    if(luaL_dostring(_L,str.CString()) != 0)
        Report();
}


void Interpreter::Report()
{
    const char * msg= lua_tostring(_L,-1);
    while(msg)
    {
        lua_pop(_L,-1);
        Log::Message(Log::LT_WARNING, msg);
        msg=lua_tostring(_L,-1);
    }
}

void Interpreter::BeginCall(int funRef)
{
    lua_settop(_L,0); //empty stack
    lua_getref(_L,funRef);
}

bool Interpreter::ExecuteCall(int params, int res)
{
    bool ret = true;
    int top = lua_gettop(_L);
    if(strcmp(luaL_typename(_L,top-params),"function"))
    {
        ret = false;
        //stack cleanup
        if(params > 0)
        {
            for(int i = top; i >= (top-params); i--)
            {
                if(!lua_isnone(_L,i))
                    lua_remove(_L,i);
            }
        }
    }
    else
    {
        if(lua_pcall(_L,params,res,0))
        {
            Report();
            ret = false;
        }
    }
    return ret;
}

void Interpreter::EndCall(int res)
{
    //stack cleanup
    for(int i = res; i > 0; i--)
    {
        if(!lua_isnone(_L,res))
            lua_remove(_L,res);
    }
}

lua_State* Interpreter::GetLuaState() { return _L; }


//From Plugin
int Interpreter::GetEventClasses()
{
    return EVT_BASIC;
}

void Interpreter::OnInitialise()
{
    Startup();
    Factory::RegisterElementInstancer("body",new LuaDocumentElementInstancer())->RemoveReference();
}

void Interpreter::OnShutdown()
{
    lua_close(_L);
}

}
}
}