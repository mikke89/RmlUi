/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2025 The RmlUi Team, and contributors
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

#include "../../Include/RmlUi/Core/Node.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/NodeInstancer.h"
#include <iterator>

namespace Rml {

static int NodeIndexInParent(const Node* node)
{
	const Node* parent = node->GetParentNode();
	if (!parent)
		return -1;

	int index = 0;
	for (const Node* sibling : parent->IterateChildren<Node>())
	{
		if (sibling == node)
			return index;
		index++;
	}
	return -1;
}

NodePtrProxy::NodePtrProxy(NodePtr node) : node(std::move(node)) {}
NodePtrProxy::NodePtrProxy(ElementPtr element) : node(As<NodePtr>(std::move(element))) {}
Node* NodePtrProxy::Get()
{
	return node.get();
}
NodePtr NodePtrProxy::Extract()
{
	return std::move(node);
}
NodePtrProxy::operator bool() const
{
	return node != nullptr;
}

Node::Node() {}

Node::~Node()
{
	RMLUI_ASSERT(parent == nullptr);

	// A simplified version of RemoveChild() for destruction.
	for (NodePtr& child : children)
		child->SetParent(nullptr);

	children.clear();
	num_non_dom_children = 0;
}

Node* Node::GetChildNode(int index) const
{
	if (index < 0 || index >= (int)children.size())
		return nullptr;

	return children[index].get();
}

int Node::GetNumChildNodes(bool include_non_dom_elements) const
{
	return (int)children.size() - (include_non_dom_elements ? 0 : num_non_dom_children);
}

bool Node::HasChildNodes() const
{
	return (int)children.size() > num_non_dom_children;
}

Node* Node::AppendChild(NodePtrProxy child, bool dom_node)
{
	RMLUI_ASSERT(child && child.Get() != this);
	Node* child_ptr = child.Get();
	if (dom_node)
	{
		auto it_end = children.end();
		children.insert(it_end - num_non_dom_children, child.Extract());
	}
	else
	{
		children.push_back(child.Extract());
		num_non_dom_children++;
	}
	// Set parent just after inserting into children. This allows us to e.g. get our previous sibling in SetParent.
	child_ptr->SetParent(this);

	OnChildNodeAdd(child_ptr, dom_node);

	return child_ptr;
}

Node* Node::InsertBefore(NodePtrProxy child, Node* adjacent_node)
{
	RMLUI_ASSERT(child);
	// Find the position in the list of children of the adjacent element. If it's nullptr, or we can't find it, then we
	// insert it at the end of the dom children, as a dom element.
	size_t child_index = 0;
	bool found_child = false;
	if (adjacent_node)
	{
		for (child_index = 0; child_index < children.size(); child_index++)
		{
			if (children[child_index].get() == adjacent_node)
			{
				found_child = true;
				break;
			}
		}
	}

	Node* child_ptr = nullptr;

	if (found_child)
	{
		child_ptr = child.Get();

		const bool dom_node = ((int)child_index < GetNumChildNodes());
		if (!dom_node)
			num_non_dom_children++;

		children.insert(children.begin() + child_index, child.Extract());
		child_ptr->SetParent(this);

		OnChildNodeAdd(child_ptr, dom_node);
	}
	else
	{
		child_ptr = AppendChild(child.Extract());
	}

	return child_ptr;
}

NodePtr Node::ReplaceChild(NodePtrProxy insert_node, Node* replace_node)
{
	RMLUI_ASSERT(insert_node);
	auto it_replace = std::find_if(children.begin(), children.end(), [replace_node](const NodePtr& node) { return node.get() == replace_node; });
	if (it_replace == children.end())
	{
		AppendChild(insert_node.Extract());
		return nullptr;
	}

	const std::ptrdiff_t replace_index = std::distance(children.begin(), it_replace);
	const bool dom_node = (replace_index < (std::ptrdiff_t)children.size() - num_non_dom_children);

	Node* inserted_node_ptr = insert_node.Get();
	children.insert(children.begin() + replace_index, insert_node.Extract());
	if (!dom_node)
		num_non_dom_children++;
	inserted_node_ptr->SetParent(this);

	NodePtr result = RemoveChild(replace_node);

	OnChildNodeAdd(inserted_node_ptr, dom_node);

	return result;
}

NodePtr Node::RemoveChild(Node* child)
{
	auto it_child = std::find_if(children.begin(), children.end(), [child](const NodePtr& node) { return node.get() == child; });
	if (it_child == children.end())
		return nullptr;

	const std::ptrdiff_t child_index = std::distance(children.begin(), it_child);
	const bool dom_node = (child_index < (std::ptrdiff_t)children.size() - num_non_dom_children);
	OnChildNodeRemove(child, dom_node);

	if (!dom_node)
		num_non_dom_children--;

	NodePtr detached_child = std::move(*it_child);
	children.erase(it_child);

	detached_child->SetParent(nullptr);

	return detached_child;
}

Element* Node::GetParentElement() const
{
	return AsIf<Element*>(GetParentNode());
}

Node* Node::GetParentNode() const
{
	return parent;
}

Node* Node::GetNextSibling() const
{
	return GetChildNode(NodeIndexInParent(this) + 1);
}

Node* Node::GetPreviousSibling() const
{
	return GetChildNode(NodeIndexInParent(this) - 1);
}

Node* Node::GetFirstChild() const
{
	return children.empty() ? nullptr : children.front().get();
}

Node* Node::GetLastChild() const
{
	return children.empty() ? nullptr : children.back().get();
}

ElementDocument* Node::GetOwnerDocument() const
{
#ifdef RMLUI_DEBUG
	if (parent && !owner_document)
	{
		// Since we have a parent but no owner_document, then we must be a 'loose' element -- that is, constructed
		// outside of a document and not attached to a child of any element in the hierarchy of a document.
		// This check ensures that we didn't just forget to set the owner document.
		RMLUI_ASSERT(!parent->GetOwnerDocument());
	}
#endif

	return owner_document;
}

Context* Node::GetContext() const
{
	ElementDocument* document = GetOwnerDocument();
	if (document != nullptr)
		return document->GetContext();

	return nullptr;
}

RenderManager* Node::GetRenderManager() const
{
	if (Context* context = GetContext())
		return &context->GetRenderManager();
	return nullptr;
}

void Node::SetInstancer(NodeInstancer* new_instancer)
{
	RMLUI_ASSERT(!instancer && new_instancer);
	instancer = new_instancer;
}

NodeInstancer* Node::GetInstancer() const
{
	return instancer;
}

void Node::Release()
{
	if (instancer)
		instancer->ReleaseNode(this);
	else
		Log::Message(Log::LT_WARNING, "Leak detected: Node (%s) not instanced via RmlUi Factory. Unable to release.", rmlui_type_name(*this));
}

void Node::SetOwnerDocument(ElementDocument* document)
{
	if (owner_document && !document)
	{
		// We are detaching from the document and thereby also the context.
		if (Context* context = owner_document->GetContext())
		{
			if (Element* self = AsIf<Element*>(this))
				context->OnElementDetach(self);
		}
	}

	// If this element is a document, then never change owner_document.
	if (owner_document != this && owner_document != document)
	{
		owner_document = document;
		for (NodePtr& child : children)
			child->SetOwnerDocument(document);
	}
}

void Node::OnChildNodeAdd(Node* /*child*/, bool /*dom_node*/) {}
void Node::OnChildNodeRemove(Node* /*child*/, bool /*dom_node*/) {}
void Node::OnParentChange(Node* /*parent*/) {}

void Node::SetParent(Node* new_parent)
{
	// Assumes we are either attaching to or detaching from the hierarchy.
	RMLUI_ASSERT((parent == nullptr) != (new_parent == nullptr));

	parent = new_parent;

	SetOwnerDocument(parent ? parent->GetOwnerDocument() : nullptr);

	OnParentChange(parent);
}

} // namespace Rml
