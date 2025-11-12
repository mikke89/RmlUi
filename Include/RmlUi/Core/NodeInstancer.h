#pragma once

#include "Element.h"
#include "Header.h"
#include "Profiling.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class Node;

/**
    A node instancer provides a method for allocating and deallocating elements.

    It is important at the same instancer that allocated the node releases it. This ensures that the node is constructed
    and freed from the same allocator.

    The returned element is a unique pointer. When this is destroyed, it will call ReleaseElement on the instancer in
    which it was instanced.
 */

class RMLUICORE_API NodeInstancer : public NonCopyMoveable {
public:
	virtual ~NodeInstancer();

	/// Instances an element given the tag name and attributes.
	/// @param[in] tag The tag of the element to instance.
	/// @return A unique pointer to the instanced element.
	virtual NodePtr InstanceNode(const String& tag) = 0;
	/// Releases an element instanced by this instancer.
	/// @param[in] node The element to release.
	virtual void ReleaseNode(Node* node) = 0;
};

/**
    The element instancer constructs a plain Element, and is used for most elements.
    This is a slightly faster version of the generic instancer, making use of a memory
    pool for allocations.
 */

class RMLUICORE_API NodeInstancerElement : public NodeInstancer {
public:
	NodePtr InstanceNode(const String& tag) override;
	void ReleaseNode(Node* node) override;
	~NodeInstancerElement();
};

/**
    The element text instancer constructs ElementText.
    This is a slightly faster version of the generic instancer, making use of a memory
    pool for allocations.
 */

class RMLUICORE_API NodeInstancerText : public NodeInstancer {
public:
	NodePtr InstanceNode(const String& tag) override;
	void ReleaseNode(Node* node) override;
};

/**
    Generic Instancer that creates the provided element type using new and delete. This instancer
    is typically used for specialized element types.
 */

template <typename T>
class NodeInstancerGeneric : public NodeInstancer {
public:
	virtual ~NodeInstancerGeneric() {}

	NodePtr InstanceNode(const String& tag) override
	{
		RMLUI_ZoneScopedN("InstanceNodeGeneric");
		return NodePtr(new T(tag));
	}

	void ReleaseNode(Node* node) override
	{
		RMLUI_ZoneScopedN("ReleaseNodeGeneric");
		delete node;
	}
};

namespace Detail {
	void InitializeNodeInstancerPools();
	void ShutdownNodeInstancerPools();
} // namespace Detail

} // namespace Rml
