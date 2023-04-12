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

// The ObserverPtrBlock pool
Pool<ObserverPtrBlock>* observerPtrBlockPool = nullptr;

static Pool<ObserverPtrBlock>& GetPool()
{
	// Wrap pool in a function to ensure it is initialized before use.
	// This pool must outlive all other global variables that derive from EnableObserverPtr. This even includes
	// user variables which we have no control over. For this reason, we intentionally let this leak.
	if (observerPtrBlockPool == nullptr)
		observerPtrBlockPool = new Pool<ObserverPtrBlock>(128, true);
	return *observerPtrBlockPool;
}

void DeallocateObserverPtrBlockIfEmpty(ObserverPtrBlock* block)
{
	RMLUI_ASSERT(block->num_observers >= 0);
	if (block->num_observers == 0 && block->pointed_to_object == nullptr)
	{
		GetPool().DestroyAndDeallocate(block);
	}
}

ObserverPtrBlock* AllocateObserverPtrBlock()
{
	return GetPool().AllocateAndConstruct();
}

} // namespace Rml
