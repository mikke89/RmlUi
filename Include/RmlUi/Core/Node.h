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

#ifndef RMLUI_CORE_NODE_H
#define RMLUI_CORE_NODE_H

#include "ScriptInterface.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class Context;
class Element;
class ElementDocument;
class NodeInstancer;
using OwnedNodeList = Vector<NodePtr>;

/**
    A generic node in the DOM tree.
 */
class RMLUICORE_API Node : public ScriptInterface {
public:
	RMLUI_RTTI_DefineWithParent(Node, ScriptInterface)
	virtual ~Node();

	/// Get the child node at the given index.
	/// @param[in] index Index of child to get.
	/// @return The child node at the given index.
	Node* GetChildNode(int index) const;
	/// Get the number of children of this node.
	/// @param[in] include_non_dom_elements True if the caller wants to include the non-DOM children. Only set this to true if you know what you're
	/// doing!
	/// @return The number of children.
	int GetNumChildNodes(bool include_non_dom_elements = false) const;
	/// Returns whether or not this node has any DOM children.
	/// @return True if the node has at least one DOM child, false otherwise.
	bool HasChildNodes() const;

	/// Append a child to this node.
	/// @param[in] node The node to append as a child.
	/// @param[in] dom_node True if the node is to be part of the DOM, false otherwise. Only set this to false if you know what you're doing!
	/// @return A pointer to the just inserted node.
	Node* AppendChild(NodePtr node, bool dom_node = true);
	[[deprecated("Use the NodePtr version of this function")]] Element* AppendChild(ElementPtr element, bool dom_element = true);
	/// Adds a child to this node directly before the adjacent node. The new node inherits the DOM/non-DOM
	/// status from the adjacent node.
	/// @param[in] node Node to be inserted.
	/// @param[in] adjacent_node The reference node which the new node will be inserted before.
	/// @return A pointer to the just inserted node.
	Node* InsertBefore(NodePtr node, Node* adjacent_node);
	[[deprecated("Use the NodePtr version of this function")]] Element* InsertBefore(ElementPtr element, Element* adjacent_element);
	/// Replaces the second node with the first node.
	/// @param[in] insert_node The node to insert and replace the other node.
	/// @param[in] replace_node The existing child node to replace. If this doesn't exist, insert_node will be appended.
	/// @return A unique pointer to the replaced node if found, discard the result to immediately destroy.
	NodePtr ReplaceChild(NodePtr insert_node, Node* replace_node);
	[[deprecated("Use the NodePtr version of this function")]] ElementPtr ReplaceChild(ElementPtr inserted_element, Element* replaced_element);
	/// Remove a child node from this node.
	/// @param[in] node The node to remove.
	/// @return A unique pointer to the node if found, discard the result to immediately destroy.
	NodePtr RemoveChild(Node* node);
	[[deprecated("Use the NodePtr version of this function")]] ElementPtr RemoveChild(Element* element);

	/// Gets this nodes's parent node.
	/// @return This node's parent.
	[[deprecated("Use GetParentElement")]] Element* GetParentNode() const;
	/// Gets this nodes's parent element.
	/// @return This node's parent if it is an element, otherwise false.
	Element* GetParentElement() const;
	/// Gets the document this node belongs to.
	/// @return This node's document.
	ElementDocument* GetOwnerDocument() const;

	/// Returns the element's context.
	/// @return The context this element's document exists within.
	Context* GetContext() const;
	/// Returns the element's render manager.
	/// @return The render manager responsible for this element.
	RenderManager* GetRenderManager() const;

	template <typename T>
	class NodeChildIterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T*;
		using difference_type = std::ptrdiff_t;
		using pointer = T**;
		using reference = T*&;

		struct Stop {};

		NodeChildIterator() = default;
		NodeChildIterator(const Node* host, int index, bool include_non_dom_elements) :
			host(host), index(index), include_non_dom_elements(include_non_dom_elements)
		{
			Advance();
		}

		T* operator*() const
		{
			RMLUI_ASSERT(host && index < (int)host->children.size());
			RMLUI_ASSERTMSG(include_non_dom_elements || index < (int)host->children.size() - host->num_non_dom_children,
				"Attempting to access a non-DOM child, but configured to exclude them.");
			return rmlui_static_cast<T*>(host->children[index].get());
		}

		NodeChildIterator& operator++()
		{
			++index;
			Advance();
			return *this;
		}

		NodeChildIterator operator++(int)
		{
			NodeChildIterator it = *this;
			++*this;
			return it;
		}

		bool operator==(const Stop& /*stop*/) const
		{
			return !host || index >= (int)host->children.size() - (include_non_dom_elements ? 0 : host->num_non_dom_children);
		}
		bool operator!=(const Stop& stop) const { return !(*this == stop); }

		bool operator==(const NodeChildIterator& other) const { return host == other.host && index == other.index; }
		bool operator!=(const NodeChildIterator& other) const { return !(*this == other); }

	private:
		const Node* host = nullptr;
		int index = 0;
		bool include_non_dom_elements = false;

		void Advance()
		{
			const int size = (int)host->children.size();
			while (index < size)
			{
				if (rmlui_dynamic_cast<T*>(host->children[index].get()))
					return;
				++index;
			}

			if (!include_non_dom_elements && index >= size - host->num_non_dom_children)
			{
				index = size;
			}
		}
	};

	template <typename T>
	class NodeChildReverseIterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T*;
		using difference_type = std::ptrdiff_t;
		using pointer = T**;
		using reference = T*&;

		struct Stop {};

		NodeChildReverseIterator() = default;
		NodeChildReverseIterator(const Node* host, int index, bool include_non_dom_elements) :
			host(host), index(index), include_non_dom_elements(include_non_dom_elements)
		{
			Advance();
		}

		T* operator*() const
		{
			RMLUI_ASSERT(host && index >= 0 && index < (int)host->children.size());
			RMLUI_ASSERTMSG(include_non_dom_elements || index < (int)host->children.size() - host->num_non_dom_children,
				"Attempting to access a non-DOM child, but configured to exclude them.");
			return rmlui_static_cast<T*>(host->children[index].get());
		}

		NodeChildReverseIterator& operator++()
		{
			--index;
			Advance();
			return *this;
		}

		NodeChildReverseIterator operator++(int)
		{
			NodeChildIterator it = *this;
			--*this;
			return it;
		}

		bool operator==(const Stop& /*stop*/) const { return !host || index < 0; }
		bool operator!=(const Stop& stop) const { return !(*this == stop); }

		bool operator==(const NodeChildReverseIterator& other) const { return host == other.host && index == other.index; }
		bool operator!=(const NodeChildReverseIterator& other) const { return !(*this == other); }

	private:
		const Node* host = nullptr;
		int index = 0;
		bool include_non_dom_elements = false;

		void Advance()
		{
			while (index >= 0)
			{
				if (rmlui_dynamic_cast<T*>(host->children[index].get()))
					return;
				--index;
			}
		}
	};

	template <typename T>
	class NodeChildRange {
	public:
		NodeChildRange() = default;
		NodeChildRange(const Node* node, bool include_non_dom_elements) : node(node), include_non_dom_elements(include_non_dom_elements) {}
		NodeChildIterator<T> begin() { return {node, 0, include_non_dom_elements}; }
		typename NodeChildIterator<T>::Stop end() { return {}; }

	private:
		const Node* node = nullptr;
		bool include_non_dom_elements = false;
	};

	template <typename T>
	class NodeChildReverseRange {
	public:
		NodeChildReverseRange() = default;
		NodeChildReverseRange(const Node* node, bool include_non_dom_elements) : node(node), include_non_dom_elements(include_non_dom_elements) {}
		NodeChildReverseIterator<T> begin()
		{
			return {node, (int)node->children.size() - 1 - (include_non_dom_elements ? 0 : node->num_non_dom_children), include_non_dom_elements};
		}
		typename NodeChildReverseIterator<T>::Stop end() { return {}; }

	private:
		const Node* node = nullptr;
		bool include_non_dom_elements = false;
	};

	template <typename T>
	NodeChildRange<T> IterateChildren(bool include_non_dom_elements = false) const
	{
		return NodeChildRange<T>(this, include_non_dom_elements);
	}

	template <typename T>
	NodeChildReverseRange<T> IterateChildrenReverse(bool include_non_dom_elements = false) const
	{
		return NodeChildReverseRange<T>(this, include_non_dom_elements);
	}

	/// Sets the instancer to use for releasing this node.
	/// @param[in] instancer Instancer to set on this node.
	void SetInstancer(NodeInstancer* instancer);

protected:
	Node();

	NodeInstancer* GetInstancer() const;
	void Release() override;

	void SetOwnerDocument(ElementDocument* document);

	virtual void OnChildNodeAdd(Node* child, bool dom_node);
	virtual void OnChildNodeRemove(Node* child, bool dom_node);
	virtual void OnParentChange(Node* parent);

private:
	void SetParent(Node* new_parent);

	NodeInstancer* instancer = nullptr;

	Node* parent = nullptr;
	ElementDocument* owner_document = nullptr;

	OwnedNodeList children;
	int num_non_dom_children = 0;

	friend class Rml::Context;
};

} // namespace Rml

#endif
