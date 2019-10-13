/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
 
#include "precompiled.h"
#include <RmlUi/Core/Lua/Interpreter.h>
#include <RmlUi/Core/Lua/Utilities.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Lua/LuaType.h>
#include "LuaDocumentElementInstancer.h"
#include <RmlUi/Core/Factory.h>
#include "LuaEventListenerInstancer.h"
#include "RmlUi.h"
//the types I made
#include "ContextDocumentsProxy.h"
#include "EventParametersProxy.h"
#include "ElementAttributesProxy.h"
#include "Log.h"
#include "Element.h"
#include "ElementStyleProxy.h"
#include "Document.h"
#include "Colourb.h"
#include "Colourf.h"
#include "Vector2f.h"
#include "Vector2i.h"
#include "Context.h"
#include "Event.h"
#include "ElementInstancer.h"
#include "ElementChildNodesProxy.h"
#include "ElementText.h"
#include "GlobalLuaFunctions.h"
#include "RmlUiContextsProxy.h"

namespace Rml {
namespace Core {
namespace Lua {

static lua_State* g_L = nullptr;

//typedefs for nicer Lua names
typedef Rml::Core::ElementDocument Document;


void Interpreter::Initialise()
{
	Rml::Core::Lua::Interpreter::Initialise(nullptr);
}

void Interpreter::Initialise(lua_State* luaStatePointer)
{
	RMLUI_ASSERT(g_L == nullptr);
	g_L = luaStatePointer;
	Rml::Core::RegisterPlugin(new Interpreter());
}

lua_State* Interpreter::GetLuaState() 
{
	return g_L;
}


void Interpreter::LoadFile(const String& file)
{
    //use the file interface to get the contents of the script
    Rml::Core::FileInterface* file_interface = Rml::Core::GetFileInterface();
    Rml::Core::FileHandle handle = file_interface->Open(file);
    if(handle == 0) {
        lua_pushfstring(g_L, "LoadFile: Unable to open file: %s", file.c_str());
        Report(g_L);
        return;
    }

    size_t size = file_interface->Length(handle);
    if(size == 0) {
        lua_pushfstring(g_L, "LoadFile: File is 0 bytes in size: %s", file.c_str());
        Report(g_L);
        return;
    }
    char* file_contents = new char[size];
    file_interface->Read(file_contents,size,handle);
    file_interface->Close(handle);

    if(luaL_loadbuffer(g_L,file_contents,size,file.c_str()) != 0)
        Report(g_L); 
    else //if there were no errors loading, then the compiled function is on the top of the stack
    {
        if(lua_pcall(g_L,0,0,0) != 0)
            Report(g_L);
    }

    delete[] file_contents;
}


void Interpreter::DoString(const Rml::Core::String& code, const Rml::Core::String& name)
{
    if(luaL_loadbuffer(g_L,code.c_str(),code.length(), name.c_str()) != 0)
        Report(g_L);
    else
    {
        if(lua_pcall(g_L,0,0,0) != 0)
            Report(g_L);
    }
}

void Interpreter::LoadString(const Rml::Core::String& code, const Rml::Core::String& name)
{
    if(luaL_loadbuffer(g_L,code.c_str(),code.length(), name.c_str()) != 0)
        Report(g_L);
}


void Interpreter::BeginCall(int funRef)
{
    lua_settop(g_L,0); //empty stack
    //lua_getref(g_L,funRef);
    lua_rawgeti(g_L, LUA_REGISTRYINDEX, (int)funRef);
}

bool Interpreter::ExecuteCall(int params, int res)
{
    bool ret = true;
    int top = lua_gettop(g_L);
    if(lua_type(g_L,top-params) != LUA_TFUNCTION)
    {
        ret = false;
        //stack cleanup
        if(params > 0)
        {
            for(int i = top; i >= (top-params); i--)
            {
                if(!lua_isnone(g_L,i))
                    lua_remove(g_L,i);
            }
        }
    }
    else
    {
        if(lua_pcall(g_L,params,res,0) != 0)
        {
            Report(g_L);
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
        if(!lua_isnone(g_L,res))
            lua_remove(g_L,res);
    }
}


//From Plugin
int Interpreter::GetEventClasses()
{
    return EVT_BASIC;
}

void Interpreter::OnInitialise()
{
	if (g_L == nullptr)
	{
		Log::Message(Log::LT_INFO, "Loading Lua interpreter");
		g_L = luaL_newstate();
		luaL_openlibs(g_L);
		owns_lua_state = true;
	}
	else
	{
		owns_lua_state = false;
	}
	RegisterCoreTypes(g_L);

	lua_document_element_instancer = new LuaDocumentElementInstancer();
	lua_event_listener_instancer = new LuaEventListenerInstancer();
    Factory::RegisterElementInstancer("body", lua_document_element_instancer);
	Factory::RegisterEventListenerInstancer(lua_event_listener_instancer);
}

void Interpreter::OnShutdown()
{
	delete lua_document_element_instancer;
	delete lua_event_listener_instancer;
	lua_document_element_instancer = nullptr;
	lua_event_listener_instancer = nullptr;

	if (owns_lua_state)
		lua_close(g_L);
	
	g_L = nullptr;

	delete this;
}

void Interpreter::RegisterCoreTypes(lua_State* L)
{
	LuaType<Vector2i>::Register(L);
	LuaType<Vector2f>::Register(L);
	LuaType<Colourf>::Register(L);
	LuaType<Colourb>::Register(L);
	LuaType<Log>::Register(L);
	LuaType<ElementStyleProxy>::Register(L);
	LuaType<Element>::Register(L);
	//things that inherit from Element
	LuaType<Document>::Register(L);
	LuaType<ElementText>::Register(L);
	LuaType<ElementPtr>::Register(L);
	LuaType<Event>::Register(L);
	LuaType<Context>::Register(L);
	LuaType<LuaRmlUi>::Register(L);
	LuaType<ElementInstancer>::Register(L);
	//Proxy tables
	LuaType<ContextDocumentsProxy>::Register(L);
	LuaType<EventParametersProxy>::Register(L);
	LuaType<ElementAttributesProxy>::Register(L);
	LuaType<ElementChildNodesProxy>::Register(L);
	LuaType<RmlUiContextsProxy>::Register(L);
	OverrideLuaGlobalFunctions(L);
	//push the global variable "rmlui" to use the "RmlUi" methods
	LuaRmlUiPushrmluiGlobal(L);
}



}
}
}
