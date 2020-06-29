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
 
#ifndef RMLUI_LUA_LUAPLUGIN_H
#define RMLUI_LUA_LUAPLUGIN_H 

#include <RmlUi/Lua/Header.h>
#include <RmlUi/Core/Plugin.h>

typedef struct lua_State lua_State;

namespace Rml {
namespace Lua {

class LuaDocumentElementInstancer;
class LuaEventListenerInstancer;

/**
    This initializes the Lua interpreter, and has functions to load the scripts or
    call functions that exist in Lua.

    @author Nathan Starkey
*/
class RMLUILUA_API LuaPlugin : public Plugin
{
public:
    LuaPlugin(lua_State* lua_state);

    static lua_State* GetLuaState();

private:
    int GetEventClasses() override;
    
	void OnInitialise() override;
    
	void OnShutdown() override;

	LuaDocumentElementInstancer* lua_document_element_instancer = nullptr;
	LuaEventListenerInstancer* lua_event_listener_instancer = nullptr;
	bool owns_lua_state = false;
};

} // namespace Lua
} // namespace Rml
#endif
