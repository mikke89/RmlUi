#include "LuaDocumentNodeInstancer.h"
#include "LuaDocument.h"

namespace Rml {
namespace Lua {

NodePtr LuaDocumentNodeInstancer::InstanceNode(const String& tag)
{
	return NodePtr(new LuaDocument(tag));
}

void LuaDocumentNodeInstancer::ReleaseNode(Node* node)
{
	delete node;
}

} // namespace Lua
} // namespace Rml
