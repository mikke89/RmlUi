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
 
#include <RmlUi/Lua/Interpreter.h>
#include "LuaPlugin.h"
#include "LuaDocumentElementInstancer.h"
#include "LuaEventListenerInstancer.h"
#include <RmlUi/Lua/Utilities.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

lua_State* Interpreter::GetLuaState()
{
    return LuaPlugin::GetLuaState();
}

void Interpreter::LoadFile(const String& file)
{
    lua_State* L = GetLuaState();

    //use the file interface to get the contents of the script
    FileInterface* file_interface = GetFileInterface();
    FileHandle handle = file_interface->Open(file);
    if (handle == 0) {
        lua_pushfstring(L, "LoadFile: Unable to open file: %s", file.c_str());
        Report(L);
        return;
    }

    size_t size = file_interface->Length(handle);
    if (size == 0) {
        lua_pushfstring(L, "LoadFile: File is 0 bytes in size: %s", file.c_str());
        Report(L);
        return;
    }
    char* file_contents = new char[size];
    file_interface->Read(file_contents, size, handle);
    file_interface->Close(handle);

    if (luaL_loadbuffer(L, file_contents, size, file.c_str()) != 0)
        Report(L);
    else //if there were no errors loading, then the compiled function is on the top of the stack
    {
        if (lua_pcall(L, 0, 0, 0) != 0)
            Report(L);
    }

    delete[] file_contents;
}


void Interpreter::DoString(const String& code, const String& name)
{
    lua_State* L = GetLuaState();

    if (luaL_loadbuffer(L, code.c_str(), code.length(), name.c_str()) != 0)
        Report(L);
    else
    {
        if (lua_pcall(L, 0, 0, 0) != 0)
            Report(L);
    }
}

void Interpreter::LoadString(const String& code, const String& name)
{
    lua_State* L = GetLuaState();

    if (luaL_loadbuffer(L, code.c_str(), code.length(), name.c_str()) != 0)
        Report(L);
}


void Interpreter::BeginCall(int funRef)
{
    lua_State* L = GetLuaState();

    lua_settop(L, 0); //empty stack
    //lua_getref(g_L,funRef);
    lua_rawgeti(L, LUA_REGISTRYINDEX, (int)funRef);
}

bool Interpreter::ExecuteCall(int params, int res)
{
    lua_State* L = GetLuaState();

    bool ret = true;
    int top = lua_gettop(L);
    if (lua_type(L, top - params) != LUA_TFUNCTION)
    {
        ret = false;
        //stack cleanup
        if (params > 0)
        {
            for (int i = top; i >= (top - params); i--)
            {
                if (!lua_isnone(L, i))
                    lua_remove(L, i);
            }
        }
    }
    else
    {
        if (lua_pcall(L, params, res, 0) != 0)
        {
            Report(L);
            ret = false;
        }
    }
    return ret;
}

void Interpreter::EndCall(int res)
{
    lua_State* L = GetLuaState();

    //stack cleanup
    for (int i = res; i > 0; i--)
    {
        if (!lua_isnone(L, res))
            lua_remove(L, res);
    }
}

} // namespace Lua
} // namespace Rml
