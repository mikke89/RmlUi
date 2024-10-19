/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "../../Include/RmlUi/Core/ObserverPtr.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "ControlledLifetimeResource.h"
#include "Pool.h"

namespace Rml {

struct ObserverPtrData {
	Pool<Detail::ObserverPtrBlock> block_pool{128, true};
};
static ControlledLifetimeResource<ObserverPtrData> observer_ptr_data;

void Detail::DeallocateObserverPtrBlockIfEmpty(ObserverPtrBlock* block)
{
	RMLUI_ASSERT(block->num_observers >= 0);
	if (block->num_observers == 0 && block->pointed_to_object == nullptr)
	{
		observer_ptr_data->block_pool.DestroyAndDeallocate(block);
	}
}
void Detail::InitializeObserverPtrPool()
{
	observer_ptr_data.InitializeIfEmpty();
}

void Detail::ShutdownObserverPtrPool()
{
	const int num_objects = observer_ptr_data->block_pool.GetNumAllocatedObjects();
	if (num_objects == 0)
	{
		observer_ptr_data.Shutdown();
	}
	else
	{
		// This pool must outlive all other global variables that derive from EnableObserverPtr. This even includes user
		// variables which we have no control over. So if there are any objects still alive, let the pool leak.
		Log::Message(Log::LT_WARNING,
			"Observer pointers still alive on shutdown, %d object(s) leaked. "
			"Please ensure that no RmlUi objects are retained in user space at the end of Rml::Shutdown.",
			num_objects);
		observer_ptr_data.Leak();
	}
}

Detail::ObserverPtrBlock* Detail::AllocateObserverPtrBlock()
{
	return observer_ptr_data->block_pool.AllocateAndConstruct();
}

} // namespace Rml
