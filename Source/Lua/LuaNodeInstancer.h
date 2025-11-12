#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/NodeInstancer.h>
#include <RmlUi/Lua/IncludeLua.h>

namespace Rml {
namespace Lua {

class LuaNodeInstancer : public ::Rml::NodeInstancer {
public:
	LuaNodeInstancer(lua_State* L);

	NodePtr InstanceNode(const String& tag) override;

	void ReleaseNode(Node* node) override;

	int ref_InstanceNode;

	// Pushes on to the top of the stack the table named NODEINSTANCERFUNCTIONS
	void PushFunctionsTable(lua_State* L);
};

} // namespace Lua
} // namespace Rml
