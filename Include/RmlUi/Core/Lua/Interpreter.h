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
 
#ifndef RMLUICORELUAINTERPRETER_H
#define RMLUICORELUAINTERPRETER_H 

#include "Header.h"
#include "lua.hpp"
#include "../Plugin.h"

namespace Rml {
namespace Core {
namespace Lua {

class LuaDocumentElementInstancer;
class LuaEventListenerInstancer;

/**
    This initializes the Lua interpreter, and has functions to load the scripts or
    call functions that exist in Lua.

    @author Nathan Starkey
*/
class RMLUILUA_API Interpreter : public Plugin
{
public:
	/** Creates the plugin.
	@remark This is equivilent to calling Initialise(nullptr). */
	static void Initialise();

	/** Creates the plugin and adds RmlUi to an existing Lua state if one is provided.
	 @remark If nullptr is passed as an argument, the plugin will automatically create the lua state during initialisation
	   and close the state during the call to Rml::Core::Shutdown(). Otherwise, if a Lua state is provided, the user is 
	   responsible for closing the provided Lua state. The state must then be closed after the call to Rml::Core::Shutdown().
	 @remark The plugin registers the "body" tag to generate a LuaDocument rather than a Rml::Core::ElementDocument. */
	static void Initialise(lua_State* L);

	/**
	@return The lua_State that the Interpreter created in Interpreter::Startup()
	@remark This class lacks a SetLuaState for a reason. If you have to use a seperate Lua binding and want to keep the types
	from RmlUi, then use this lua_State; it will already have all of the libraries loaded, and all of the types defined.
	Alternatively, you can call RegisterCoreTypes(lua_State*) with your own Lua state if you need them defined in it. */
	static lua_State* GetLuaState();

    /** This function calls luaL_loadfile and then lua_pcall, reporting the errors (if any)
    @param[in] file Fully qualified file name to execute.
    @remark Somewhat misleading name if you are used to the Lua meaning of "load file". It behaves
    exactly as luaL_dofile does.            */
    static void LoadFile(const Rml::Core::String& file);
    /** Calls lua_dostring and reports the errors.
    @param[in] code String to execute
    @param[in] name Name for the code that will show up in the Log  */
    static void DoString(const Rml::Core::String& code, const Rml::Core::String& name = "");
    /** Same as DoString, except does NOT call pcall on it. It will leave the compiled (but not executed) string
    on top of the stack. It behaves exactly like luaL_loadstring, but you get to specify the name
    @param[in] code String to compile
    @param[in] name Name for the code that will show up in the Log    */
    static void LoadString(const Rml::Core::String& code, const Rml::Core::String& name = "");

    /** Clears all of the items on the stack, and pushes the function from funRef on top of the stack. Only use
    this if you used lua_ref instead of luaL_ref
    @param[in] funRef Lua reference that you would recieve from calling lua_ref   */
    static void BeginCall(int funRef);
    /** Uses lua_pcall on a function, which executes the function with params number of parameters and pushes
    res number of return values on to the stack.
    @pre Before you call this, your stack should look like:
    [1] function to call; 
    [2...top] parameters to pass to the function (if any).
    Or, in words, make sure to push the function on the stack before the parameters.
    @post After this function, the params and function will be popped off, and 'res' 
    number of items will be pushed.     */
    static bool ExecuteCall(int params = 0, int res = 0);
    /** removes 'res' number of items from the stack
    @param[in] res Number of results to remove from the stack.   */
    static void EndCall(int res = 0);
    
private:
    int GetEventClasses() override;
    
	void OnInitialise() override;
    
	void OnShutdown() override;

	/** This will populate the global Lua table with all of the Lua core types by calling LuaType<T>::Register
	@param[in] L The lua_State to use to register the types
	@remark This is called automatically inside of Interpreter::Startup(), so you do not have to
	call this function upon initialization of the Interpreter. If you are using RmlControlsLua, then you
	\em will need to call Rml::Controls::Lua::RegisterTypes(lua_State*)     */
	static void RegisterCoreTypes(lua_State* L);

	LuaDocumentElementInstancer* lua_document_element_instancer = nullptr;
	LuaEventListenerInstancer* lua_event_listener_instancer = nullptr;
	bool owns_lua_state = false;
};

}
}
}
#endif

