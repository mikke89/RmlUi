#include "../../Include/RmlUi/Core/ObserverPtr.h"
#include "Pool.h"

namespace Rml {

struct ObserverPtrData {
	bool is_shutdown = false;
	Pool<Detail::ObserverPtrBlock> block_pool{128, true};
};
static ObserverPtrData* observer_ptr_data = nullptr;

void Detail::DeallocateObserverPtrBlockIfEmpty(ObserverPtrBlock* block)
{
	RMLUI_ASSERT(block->num_observers >= 0);
	if (block->num_observers == 0 && block->pointed_to_object == nullptr)
	{
		observer_ptr_data->block_pool.DestroyAndDeallocate(block);
		if (observer_ptr_data->is_shutdown && observer_ptr_data->block_pool.GetNumAllocatedObjects() == 0)
		{
			delete observer_ptr_data;
			observer_ptr_data = nullptr;
		}
	}
}

void Detail::InitializeObserverPtrPool()
{
	if (!observer_ptr_data)
		observer_ptr_data = new ObserverPtrData;
	observer_ptr_data->is_shutdown = false;
}

void Detail::ShutdownObserverPtrPool()
{
	if (observer_ptr_data->block_pool.GetNumAllocatedObjects() == 0)
	{
		delete observer_ptr_data;
		observer_ptr_data = nullptr;
	}
	else
	{
		// This pool must outlive all other global variables that derive from EnableObserverPtr. This even includes user
		// variables which we have no control over. So if there are any objects still alive, let the pool garbage
		// collect itself when all references to it are gone. It is somewhat unreasonable to expect that no observer
		// pointers remain, particularly because that means no objects derived from Rml::EventListener can be alive in
		// user space, which can be a hassle to ensure and is otherwise pretty innocent.
		observer_ptr_data->is_shutdown = true;
	}
}

Detail::ObserverPtrBlock* Detail::AllocateObserverPtrBlock()
{
	return observer_ptr_data->block_pool.AllocateAndConstruct();
}

} // namespace Rml
