#include "LuaDocumentElementInstancer.h"
#include "LuaEventListenerInstancer.h"
#include "LuaPlugin.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Lua/Interpreter.h>
#include <RmlUi/Lua/LuaType.h>

namespace Rml {
namespace Lua {

static int ErrorHandler(lua_State* L)
{
	const char* msg = lua_tostring(L, 1);
	if (msg == NULL)
	{
		if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
			return 1;
		else
			msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1);
	return 1;
}

static bool LuaCall(lua_State* L, int nargs, int nresults)
{
	int errfunc = -2 - nargs;
	lua_pushcfunction(L, ErrorHandler);
	lua_insert(L, errfunc);
	if (lua_pcall(L, nargs, nresults, errfunc) != LUA_OK)
	{
		Log::Message(Log::LT_WARNING, "%s", lua_tostring(L, -1));
		lua_pop(L, 2);
		return false;
	}
	lua_remove(L, -1 - nresults);
	return true;
}

lua_State* Interpreter::GetLuaState()
{
	return LuaPlugin::GetLuaState();
}

bool Interpreter::LoadFile(const String& file)
{
	lua_State* L = GetLuaState();

	// use the file interface to get the contents of the script
	FileInterface* file_interface = GetFileInterface();
	FileHandle handle = file_interface->Open(file);
	if (handle == 0)
	{
		Log::Message(Log::LT_WARNING, "LoadFile: Unable to open file: %s", file.c_str());
		return false;
	}

	size_t size = file_interface->Length(handle);
	if (size == 0)
	{
		Log::Message(Log::LT_WARNING, "LoadFile: File is 0 bytes in size: %s", file.c_str());
		return false;
	}
	UniquePtr<char[]> file_contents(new char[size]);
	file_interface->Read(file_contents.get(), size, handle);
	file_interface->Close(handle);

	if (luaL_loadbuffer(L, file_contents.get(), size, ("@" + file).c_str()) != 0)
	{
		Log::Message(Log::LT_WARNING, "%s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	return LuaCall(L, 0, 0);
	;
}

bool Interpreter::DoString(const String& code, const String& name)
{
	lua_State* L = GetLuaState();
	return LoadString(code, name) && LuaCall(L, 0, 0);
}

bool Interpreter::LoadString(const String& code, const String& name)
{
	lua_State* L = GetLuaState();

	if (luaL_loadbuffer(L, code.c_str(), code.length(), name.c_str()) != 0)
	{
		Log::Message(Log::LT_WARNING, "%s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	return true;
}

void Interpreter::BeginCall(int funRef)
{
	lua_State* L = GetLuaState();

	lua_settop(L, 0); // empty stack
	// lua_getref(g_L,funRef);
	lua_rawgeti(L, LUA_REGISTRYINDEX, (int)funRef);
}

bool Interpreter::ExecuteCall(int params, int res)
{
	lua_State* L = GetLuaState();
	return LuaCall(L, params, res);
}

void Interpreter::EndCall(int res)
{
	lua_State* L = GetLuaState();
	lua_pop(L, res);
}

} // namespace Lua
} // namespace Rml
