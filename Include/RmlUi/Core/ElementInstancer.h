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

#ifndef RMLUI_CORE_ELEMENTINSTANCER_H
#define RMLUI_CORE_ELEMENTINSTANCER_H

#include "Element.h"
#include "Header.h"
#include "Profiling.h"
#include "Traits.h"
#include "Types.h"

namespace Rml {

class Element;

/**
    An element instancer provides a method for allocating
    and deallocating elements.

    It is important at the same instancer that allocated
    the element releases it. This ensures there are no
    issues with memory from different DLLs getting mixed up.

    The returned element is a unique pointer. When this is
    destroyed, it will call	ReleaseElement on the instancer
    in which it was instanced.

    @author Lloyd Weehuizen
 */

class RMLUICORE_API ElementInstancer : public NonCopyMoveable {
public:
	virtual ~ElementInstancer();

	/// Instances an element given the tag name and attributes.
	/// @param[in] parent The element the new element is destined to be parented to.
	/// @param[in] tag The tag of the element to instance.
	/// @param[in] attributes Dictionary of attributes.
	/// @return A unique pointer to the instanced element.
	virtual ElementPtr InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes) = 0;
	/// Releases an element instanced by this instancer.
	/// @param[in] element The element to release.
	virtual void ReleaseElement(Element* element) = 0;
};

/**
    The element instancer constructs a plain Element, and is used for most elements.
    This is a slightly faster version of the generic instancer, making use of a memory
    pool for allocations.
 */

class RMLUICORE_API ElementInstancerElement : public ElementInstancer {
public:
	ElementPtr InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes) override;
	void ReleaseElement(Element* element) override;
	~ElementInstancerElement();
};

/**
    The element text instancer constructs ElementText.
    This is a slightly faster version of the generic instancer, making use of a memory
    pool for allocations.
 */

class RMLUICORE_API ElementInstancerText : public ElementInstancer {
public:
	ElementPtr InstanceElement(Element* parent, const String& tag, const XMLAttributes& attributes) override;
	void ReleaseElement(Element* element) override;
};

/**
    Generic Instancer that creates the provided element type using new and delete. This instancer
    is typically used for specialized element types.
 */

template <typename T>
class ElementInstancerGeneric : public ElementInstancer {
public:
	virtual ~ElementInstancerGeneric() {}

	ElementPtr InstanceElement(Element* /*parent*/, const String& tag, const XMLAttributes& /*attributes*/) override
	{
		RMLUI_ZoneScopedN("ElementGenericInstance");
		return ElementPtr(new T(tag));
	}

	void ReleaseElement(Element* element) override
	{
		RMLUI_ZoneScopedN("ElementGenericRelease");
		delete element;
	}
};

} // namespace Rml
#endif
