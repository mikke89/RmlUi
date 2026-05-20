#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Lua/IncludeLua.h>

namespace Rml {
namespace Lua {

struct LuaDataModel;

// Create or Get a DataModel in L, return false on fail
bool OpenLuaDataModel(lua_State* L, Rml::Context* context, int name_index, int table_index);
// Close a DataModel
void CloseLuaDataModel(lua_State* L, Rml::Context* context, int name_index);

} // namespace Lua
} // namespace Rml
