#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>

namespace Rml {
namespace Lua {

struct LuaDataModel;

// Create or Get a DataModel in L, return false on fail
bool OpenLuaDataModel(lua_State* L, Rml::Context* context, int name_index, int table_index);
// Should Close object (on L top) after DataModel released (Context::RemoveDataModel)
void CloseLuaDataModel(lua_State* L);

} // namespace Lua
} // namespace Rml
