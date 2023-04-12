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
#ifndef RMLUI_CORE_PROPERTIESITERATORVIEW_H
#define RMLUI_CORE_PROPERTIESITERATORVIEW_H

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

    @author Michael R. P. Ragazzon
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
#endif
