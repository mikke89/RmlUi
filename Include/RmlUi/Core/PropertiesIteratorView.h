#pragma once

#include "Property.h"
#include "Types.h"

namespace Rml {

class PropertiesIterator;

/**
    Provides an iterator for properties defined in the element's style or definition.
    Construct it through the desired Element.
    Warning: Modifying the underlying style invalidates the iterator.

    Usage:
        for(auto it = element.IterateLocalProperties(); !it.AtEnd(); ++it) { ... }

    Note: Not an std-style iterator. Implemented as a wrapper over the internal
    iterator to avoid exposing internal headers to the user.
 */

class RMLUICORE_API PropertiesIteratorView {
public:
	PropertiesIteratorView(UniquePtr<PropertiesIterator> ptr);
	PropertiesIteratorView(PropertiesIteratorView&& other) noexcept;
	PropertiesIteratorView& operator=(PropertiesIteratorView&& other) noexcept;
	PropertiesIteratorView(const PropertiesIteratorView& other) = delete;
	PropertiesIteratorView& operator=(const PropertiesIteratorView&) = delete;
	~PropertiesIteratorView();

	PropertiesIteratorView& operator++();

	// Returns true when all properties have been iterated over.
	// @warning The getters and operator++ can only be called if the iterator is not at the end.
	bool AtEnd() const;

	PropertyId GetId() const;
	const String& GetName() const;
	const Property& GetProperty() const;

private:
	UniquePtr<PropertiesIterator> ptr;
};

} // namespace Rml
