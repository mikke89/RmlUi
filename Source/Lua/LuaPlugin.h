#pragma once

#include <RmlUi/Core/Plugin.h>
#include <RmlUi/Lua/Header.h>
#include <RmlUi/Lua/Lua.h>

typedef struct lua_State lua_State;

namespace Rml {
namespace Lua {

class LuaDocumentElementInstancer;
class LuaEventListenerInstancer;

/**
    This initializes the Lua interpreter, and has functions to load the scripts or
    call functions that exist in Lua.
*/
class RMLUILUA_API LuaPlugin : public Plugin {
public:
	LuaPlugin(lua_State* lua_state, const Options& options);

	static lua_State* GetLuaState();

private:
	int GetEventClasses() override;

	void OnInitialise() override;

	void OnShutdown() override;

	LuaDocumentElementInstancer* lua_document_element_instancer = nullptr;
	LuaEventListenerInstancer* lua_event_listener_instancer = nullptr;
	bool owns_lua_state = false;
	Options options;
};

} // namespace Lua
} // namespace Rml
