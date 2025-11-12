#pragma once

#include <RmlUi/Core/NodeInstancer.h>

namespace Rml {
namespace Lua {

class LuaDocumentNodeInstancer : public ::Rml::NodeInstancer {
public:
	NodePtr InstanceNode(const String& tag) override;
	void ReleaseNode(Node* node) override;
};

} // namespace Lua
} // namespace Rml
