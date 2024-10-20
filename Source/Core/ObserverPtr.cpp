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
