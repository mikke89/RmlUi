#include "ElementMeta.h"

namespace Rml {

ControlledLifetimeResource<ElementMetaPool> ElementMetaPool::element_meta_pool;

void ElementMetaPool::Initialize()
{
	element_meta_pool.InitializeIfEmpty();
}

void ElementMetaPool::Shutdown()
{
	const int num_objects = element_meta_pool->pool.GetNumAllocatedObjects();
	if (num_objects == 0)
	{
		element_meta_pool.Shutdown();
	}
	else
	{
		Log::Message(Log::LT_ERROR,
			"Element meta pool not empty on shutdown, %d object(s) leaked. This will likely lead to a crash when element is destroyed. Ensure that "
			"no Rml::Element objects are kept alive in user space at the end of Rml::Shutdown.",
			num_objects);
		RMLUI_ERROR;
		element_meta_pool.Leak();
	}
}

} // namespace Rml
