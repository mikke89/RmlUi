#pragma once

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
	/// @note Modifying the ink overflow rectangle affects the rendering of all filters active on the current element.
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
