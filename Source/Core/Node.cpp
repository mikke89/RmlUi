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

Node* Node::AppendChild(NodePtr child, bool dom_node)
{
	RMLUI_ASSERT(child && child.get() != this);
	Node* child_ptr = child.get();
	if (dom_node)
	{
		auto it_end = children.end();
		children.insert(it_end - num_non_dom_children, std::move(child));
	}
	else
	{
		children.push_back(std::move(child));
		num_non_dom_children++;
	}
	// Set parent just after inserting into children. This allows us to e.g. get our previous sibling in SetParent.
	child_ptr->SetParent(this);

	OnChildNodeAdd(child_ptr, dom_node);

	return child_ptr;
}

Node* Node::InsertBefore(NodePtr child, Node* adjacent_node)
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
		child_ptr = child.get();

		const bool dom_node = ((int)child_index < GetNumChildNodes());
		if (!dom_node)
			num_non_dom_children++;

		children.insert(children.begin() + child_index, std::move(child));
		child_ptr->SetParent(this);

		OnChildNodeAdd(child_ptr, dom_node);
	}
	else
	{
		child_ptr = AppendChild(std::move(child));
	}

	return child_ptr;
}

NodePtr Node::ReplaceChild(NodePtr insert_node, Node* replace_node)
{
	RMLUI_ASSERT(insert_node);
	auto it_replace = std::find_if(children.begin(), children.end(), [replace_node](const NodePtr& node) { return node.get() == replace_node; });
	if (it_replace == children.end())
	{
		AppendChild(std::move(insert_node));
		return nullptr;
	}

	const std::ptrdiff_t replace_index = std::distance(children.begin(), it_replace);
	const bool dom_node = (replace_index < (std::ptrdiff_t)children.size() - num_non_dom_children);

	Node* inserted_node_ptr = insert_node.get();
	children.insert(children.begin() + replace_index, std::move(insert_node));
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

Element* Node::AppendChild(ElementPtr child, bool dom_element)
{
	return rmlui_static_cast<Element*>(AppendChild(As<NodePtr>(std::move(child)), dom_element));
}

Element* Node::InsertBefore(ElementPtr child, Element* adjacent_element)
{
	return rmlui_static_cast<Element*>(InsertBefore(As<NodePtr>(std::move(child)), adjacent_element));
}

ElementPtr Node::ReplaceChild(ElementPtr inserted_element, Element* replaced_element)
{
	return As<ElementPtr>(ReplaceChild(As<NodePtr>(std::move(inserted_element)), replaced_element));
}

ElementPtr Node::RemoveChild(Element* child)
{
	return As<ElementPtr>(RemoveChild(As<Node*>(child)));
}

Element* Node::GetParentNode() const
{
	return GetParentElement();
}

Element* Node::GetParentElement() const
{
	return rmlui_dynamic_cast<Element*>(parent);
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
