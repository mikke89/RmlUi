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

NodePtr NodeInstancerText::InstanceNode(const String& tag)
{
	ElementText* ptr = node_instancer_pools->pool_text_default.AllocateAndConstruct(tag);
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
