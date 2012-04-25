#include "precompiled.h"
#include <Rocket/Core/Lua/Interpreter.h>
#include <Rocket/Core/Log.h>
#include <Rocket/Core/String.h>
#include <Rocket/Core/Lua/LuaType.h>
#include "LuaDocumentElementInstancer.h"
#include <Rocket/Core/Factory.h>
#include "LuaEventListenerInstancer.h"
#include "LuaDataFormatter.h"
#include "Rocket.h"

namespace Rocket {
namespace Core {
namespace Lua {
lua_State* Interpreter::_L = NULL;
typedef Rocket::Core::ElementDocument Document;
typedef Rocket::Core::Lua::LuaDataFormatter DataFormatter;

void Interpreter::Startup()
{
    Log::Message(Log::LT_INFO, "Loading Lua interpreter");
    _L = luaL_newstate();
    luaL_openlibs(_L);

    RegisterEverything(_L);
}


void Interpreter::RegisterEverything(lua_State* L)
{
    LuaType<Vector2i>::Register(L);
    LuaType<Vector2f>::Register(L);
    LuaType<Colourf>::Register(L);
    LuaType<Colourb>::Register(L);
    LuaType<Log>::Register(L);
    LuaType<ElementStyle>::Register(L);
    LuaType<Element>::Register(L);
        //things that inherit from Element
        LuaType<Document>::Register(L);
        //controls that inherit from Element
        LuaType<Rocket::Controls::ElementTabSet>::Register(L);
        LuaType<Rocket::Controls::ElementDataGrid>::Register(L);
        LuaType<Rocket::Controls::ElementDataGridRow>::Register(L);
        LuaType<Rocket::Controls::ElementForm>::Register(L);
        LuaType<Rocket::Controls::ElementFormControl>::Register(L);
            //inherits from ElementFormControl
            LuaType<Rocket::Controls::ElementFormControlSelect>::Register(L);
            LuaType<Rocket::Controls::ElementFormControlDataSelect>::Register(L);
            LuaType<Rocket::Controls::ElementFormControlInput>::Register(L);
            LuaType<Rocket::Controls::ElementFormControlTextArea>::Register(L);
    LuaType<Event>::Register(L);
    LuaType<Context>::Register(L);
    LuaType<DataFormatter>::Register(L);
    LuaType<rocket>::Register(L);
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
        lua_pop(_L,1);
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
    //String strtype = lua_typename(_L,top-params);
    String strtype;
    for(int i = top; i >= 1; i--)
        strtype = lua_typename(_L,lua_type(_L,i));
    if(lua_type(_L,top-params) != LUA_TFUNCTION)
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
    Factory::RegisterEventListenerInstancer(new LuaEventListenerInstancer())->RemoveReference();
}

void Interpreter::OnShutdown()
{
	//causing crashes
    //lua_close(_L);
}

void Interpreter::Initialise()
{
    Rocket::Core::RegisterPlugin(new Interpreter());
}

void Interpreter::Shutdown()
{
	lua_close(_L);
}

}
}
}
