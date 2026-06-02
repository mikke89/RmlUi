#pragma once

#include "Property.h"
#include "Types.h"

namespace Rml {

class AllPropertiesIterator;

/**
    Provides an iterator for properties defined in the element's style or definition.
    Construct it through the desired Element.
    Warning: Modifying the underlying style invalidates the iterator.

    Usage:
        for(auto it = element.IterateLocalProperties(nullptr); !it.AtEnd(); ++it) { ... }

    Note: Not an std-style iterator. Implemented as a wrapper over the internal
    iterator to avoid exposing internal headers to the user.
 */

class RMLUICORE_API PropertiesIteratorView {
public:
	using ValueType = Pair<String, const Property&>;

	PropertiesIteratorView(UniquePtr<AllPropertiesIterator> ptr);
	PropertiesIteratorView(PropertiesIteratorView&& other) noexcept;
	PropertiesIteratorView& operator=(PropertiesIteratorView&& other) noexcept;
	PropertiesIteratorView(const PropertiesIteratorView& other) = delete;
	PropertiesIteratorView& operator=(const PropertiesIteratorView&) = delete;
	~PropertiesIteratorView();

	PropertiesIteratorView& operator++();

	// Returns true when all properties have been iterated over.
	// @warning The getters and operator++ can only be called if the iterator is not at the end.
	bool AtEnd() const;

	// Returns the property name and value pair.
	Pair<String, const Property&> operator*() const;

private:
	UniquePtr<AllPropertiesIterator> ptr;
};

} // namespace Rml
