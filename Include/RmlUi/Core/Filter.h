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

#ifndef RMLUI_CORE_FILTER_H
#define RMLUI_CORE_FILTER_H

#include "EffectSpecification.h"
#include "Header.h"
#include "Types.h"

namespace Rml {

class Element;
class PropertyDictionary;
class CompiledFilter;

/**
    The abstract base class for visual filters that are applied when rendering the element.
 */
class RMLUICORE_API Filter {
public:
	Filter();
	virtual ~Filter();

	/// Called to compile the filter for a given element.
	/// @param[in] element The element the filter will be applied to.
	/// @return A compiled filter constructed through the render manager, or a default-constructed one to indicate an error.
	virtual CompiledFilter CompileFilter(Element* element) const = 0;

	/// Called to allow extending the area being affected by this filter beyond the border box of the element.
	/// @param[in] element The element the filter is being rendered on.
	/// @param[in,out] overflow The ink overflow rectangle determining the clipping region to be applied when filtering the current element.
	/// @note Modifying the ink overflow rectangle affects rendering of all filters active on the current element.
	/// @note Only affects the 'filter' property, not 'backdrop-filter'.
	virtual void ExtendInkOverflow(Element* element, Rectanglef& overflow) const;
};

/**
    A filter instancer, which can be inherited from to instance new filters when encountered in the style sheet.
 */
class RMLUICORE_API FilterInstancer : public EffectSpecification {
public:
	FilterInstancer();
	virtual ~FilterInstancer();

	/// Instances a filter given the name and attributes from the RCSS file.
	/// @param[in] name The type of filter desired. For example, "filter: simple(...)" is declared as type "simple".
	/// @param[in] properties All RCSS properties associated with the filter.
	/// @return A shared_ptr to the filter if it was instanced successfully.
	virtual SharedPtr<Filter> InstanceFilter(const String& name, const PropertyDictionary& properties) = 0;
};

} // namespace Rml
#endif
