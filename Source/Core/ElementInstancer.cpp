#include "../../Include/RmlUi/Core/ElementInstancer.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "ControlledLifetimeResource.h"
#include "Pool.h"
#include "XMLParseTools.h"

namespace Rml {

ElementInstancer::~ElementInstancer() {}

struct ElementInstancerPools {
	Pool<Element> pool_element{200, true};
	Pool<ElementText> pool_text_default{200, true};

	bool IsEmpty() const { return pool_element.GetNumAllocatedObjects() == 0 && pool_text_default.GetNumAllocatedObjects() == 0; }
};
static ControlledLifetimeResource<ElementInstancerPools> element_instancer_pools;

ElementPtr ElementInstancerElement::InstanceElement(Element* /*parent*/, const String& tag, const XMLAttributes& /*attributes*/)
{
	Element* ptr = element_instancer_pools->pool_element.AllocateAndConstruct(tag);
	return ElementPtr(ptr);
}

void ElementInstancerElement::ReleaseElement(Element* element)
{
	element_instancer_pools->pool_element.DestroyAndDeallocate(element);
}

ElementInstancerElement::~ElementInstancerElement()
{
	int num_elements = element_instancer_pools->pool_element.GetNumAllocatedObjects();
	if (num_elements > 0)
	{
		Log::Message(Log::LT_WARNING, "--- Found %d leaked element(s) ---", num_elements);

		for (auto it = element_instancer_pools->pool_element.Begin(); it; ++it)
			Log::Message(Log::LT_WARNING, "    %s", it->GetAddress().c_str());

		Log::Message(Log::LT_WARNING, "------");
	}
}

ElementPtr ElementInstancerText::InstanceElement(Element* /*parent*/, const String& tag, const XMLAttributes& /*attributes*/)
{
	ElementText* ptr = element_instancer_pools->pool_text_default.AllocateAndConstruct(tag);
	return ElementPtr(static_cast<Element*>(ptr));
}

void ElementInstancerText::ReleaseElement(Element* element)
{
	element_instancer_pools->pool_text_default.DestroyAndDeallocate(rmlui_static_cast<ElementText*>(element));
}

void Detail::InitializeElementInstancerPools()
{
	element_instancer_pools.InitializeIfEmpty();
}

void Detail::ShutdownElementInstancerPools()
{
	if (element_instancer_pools->IsEmpty())
		element_instancer_pools.Shutdown();
	else
		element_instancer_pools.Leak();
}

} // namespace Rml
