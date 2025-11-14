#include "../../Include/RmlUi/Core/NodeInstancer.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "ControlledLifetimeResource.h"
#include "Pool.h"
#include "XMLParseTools.h"

namespace Rml {

NodeInstancer::~NodeInstancer() {}

struct NodeInstancerPools {
	Pool<Element> pool_element{200, true};
	Pool<ElementText> pool_text_default{200, true};

	bool IsEmpty() const { return pool_element.GetNumAllocatedObjects() == 0 && pool_text_default.GetNumAllocatedObjects() == 0; }
};
static ControlledLifetimeResource<NodeInstancerPools> node_instancer_pools;

NodePtr NodeInstancerElement::InstanceNode(const String& tag)
{
	Element* ptr = node_instancer_pools->pool_element.AllocateAndConstruct(tag);
	return NodePtr(ptr);
}

void NodeInstancerElement::ReleaseNode(Node* node)
{
	node_instancer_pools->pool_element.DestroyAndDeallocate(rmlui_static_cast<Element*>(node));
}

NodeInstancerElement::~NodeInstancerElement()
{
	int num_elements = node_instancer_pools->pool_element.GetNumAllocatedObjects();
	if (num_elements > 0)
	{
		Log::Message(Log::LT_WARNING, "--- Found %d leaked element(s) ---", num_elements);

		for (auto it = node_instancer_pools->pool_element.Begin(); it; ++it)
			Log::Message(Log::LT_WARNING, "    %s", it->GetAddress().c_str());

		Log::Message(Log::LT_WARNING, "------");
	}
}

NodePtr NodeInstancerText::InstanceNode(const String& /*tag*/)
{
	ElementText* ptr = node_instancer_pools->pool_text_default.AllocateAndConstruct();
	return NodePtr(ptr);
}

void NodeInstancerText::ReleaseNode(Node* node)
{
	node_instancer_pools->pool_text_default.DestroyAndDeallocate(rmlui_static_cast<ElementText*>(node));
}

void Detail::InitializeNodeInstancerPools()
{
	node_instancer_pools.InitializeIfEmpty();
}

void Detail::ShutdownNodeInstancerPools()
{
	if (node_instancer_pools->IsEmpty())
		node_instancer_pools.Shutdown();
	else
		node_instancer_pools.Leak();
}

} // namespace Rml
