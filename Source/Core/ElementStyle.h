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

#ifndef RMLUI_CORE_ELEMENTSTYLE_H
#define RMLUI_CORE_ELEMENTSTYLE_H

#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class ElementDefinition;
class PropertiesIterator;
enum class RelativeTarget;

enum class PseudoClassState : uint8_t { Clear = 0, Set = 1, Override = 2 };
using PseudoClassMap = SmallUnorderedMap<String, PseudoClassState>;

/**
    Manages an element's style and property information.
    @author Lloyd Weehuizen
 */

class ElementStyle {
public:
	/// Constructor
	/// @param[in] element The element this structure belongs to.
	ElementStyle(Element* element);

	/// Update this definition if required
	void UpdateDefinition();

	/// Sets or removes a pseudo-class on the element.
	/// @param[in] pseudo_class The pseudo class to activate or deactivate.
	/// @param[in] activate True if the pseudo class is to be activated, false to be deactivated.
	/// @param[in] override_class True to activate or deactivate the override state of the pseudo class, for advanced use cases.
	/// @note An overridden pseudo class means that it will act as if activated even when it has been cleared the normal way.
	/// @return True if the pseudo class was changed.
	bool SetPseudoClass(const String& pseudo_class, bool activate, bool override_class = false);
	/// Checks if a specific pseudo-class has been set on the element.
	/// @param[in] pseudo_class The name of the pseudo-class to check for.
	/// @return True if the pseudo-class is set on the element, false if not.
	bool IsPseudoClassSet(const String& pseudo_class) const;
	/// Gets a list of the current active pseudo classes
	const PseudoClassMap& GetActivePseudoClasses() const;

	/// Sets or removes a class on the element.
	/// @param[in] class_name The name of the class to add or remove from the class list.
	/// @param[in] activate True if the class is to be added, false to be removed.
	/// @return True if the class was changed, false otherwise.
	bool SetClass(const String& class_name, bool activate);
	/// Checks if a class is set on the element.
	/// @param[in] class_name The name of the class to check for.
	/// @return True if the class is set on the element, false otherwise.
	bool IsClassSet(const String& class_name) const;
	/// Specifies the entire list of classes for this element. This will replace any others specified.
	/// @param[in] class_names The list of class names to set on the style, separated by spaces.
	void SetClassNames(const String& class_names);
	/// Return the active class list.
	/// @return A string containing all the classes on the element, separated by spaces.
	String GetClassNames() const;
	/// Return the active class list.
	const StringList& GetClassNameList() const;

	/// Sets a local property override on the element to a pre-parsed value.
	/// @param[in] id The ID  of the new property.
	/// @param[in] property The parsed property to set.
	bool SetProperty(PropertyId id, const Property& property);
	/// Removes a local property override on the element; its value will revert to that defined in
	/// the style sheet.
	/// @param[in] id The ID of the local property definition to remove.
	void RemoveProperty(PropertyId id);
	/// Returns one of this element's properties. If this element is not defined this property, or a parent cannot
	/// be found that we can inherit the property from, the default value will be returned.
	/// @param[in] id The ID of the property to fetch the value for.
	/// @return The value of this property for this element, or nullptr if no property exists with the given name.
	const Property* GetProperty(PropertyId id) const;
	/// Returns one of this element's properties. If this element is not defined this property, nullptr will be
	/// returned.
	/// @param[in] id The ID of the property to fetch the value for.
	/// @return The value of this property for this element, or nullptr if this property has not been explicitly defined for this element.
	const Property* GetLocalProperty(PropertyId id) const;
	/// Returns the local style properties, excluding any properties from local class.
	const PropertyMap& GetLocalStyleProperties() const;

	/// Resolves a numeric value with units of number, percentage, length, or angle to their canonical unit (unit-less, 'px', or 'rad').
	/// @param[in] value The value to be resolved.
	/// @param[in] base_value The value that is scaled by the number or percentage value, if applicable.
	/// @return The resolved value in their canonical unit, or zero if it could not be resolved.
	float ResolveNumericValue(NumericValue value, float base_value) const;
	/// Resolves a property with units of number, length, or percentage to a length in 'px' units.
	/// Numbers and percentages are resolved by scaling the size of the specified target.
	float ResolveRelativeLength(NumericValue value, RelativeTarget relative_target) const;

	/// Mark inherited properties dirty.
	/// Inherited properties will automatically be set when parent inherited properties are changed. However,
	/// some operations may require to dirty these manually, such as when moving an element into another.
	void DirtyInheritedProperties();

	// Sets a single property as dirty.
	void DirtyProperty(PropertyId id);
	/// Dirties all properties with any of the given units (OR-ed together) on the current element (*not* recursive).
	void DirtyPropertiesWithUnits(Units units);
	/// Dirties all properties with any of the given units (OR-ed together) on the current element and recursively on all children.
	void DirtyPropertiesWithUnitsRecursive(Units units);

	/// Returns true if any properties are dirty such that computed values need to be recomputed
	bool AnyPropertiesDirty() const;

	/// Turns the local and inherited properties into computed values for this element. These values can in turn be used during the layout procedure.
	/// Must be called in correct order, always parent before its children.
	PropertyIdSet ComputeValues(Style::ComputedValues& values, const Style::ComputedValues* parent_values,
		const Style::ComputedValues* document_values, bool values_are_default_initialized, float dp_ratio, Vector2f vp_dimensions);

	/// Returns an iterator for iterating the local properties of this element.
	/// Note: Modifying the element's style invalidates its iterator.
	PropertiesIterator Iterate() const;

private:
	// Sets a list of properties as dirty.
	void DirtyProperties(const PropertyIdSet& properties);

	static const Property* GetLocalProperty(PropertyId id, const PropertyDictionary& inline_properties, const ElementDefinition* definition);
	static const Property* GetProperty(PropertyId id, const Element* element, const PropertyDictionary& inline_properties,
		const ElementDefinition* definition);
	static void TransitionPropertyChanges(Element* element, PropertyIdSet& properties, const PropertyDictionary& inline_properties,
		const ElementDefinition* old_definition, const ElementDefinition* new_definition);

	// Element these properties belong to
	Element* element;

	// The list of classes applicable to this object.
	StringList classes;
	// This element's current pseudo-classes.
	PseudoClassMap pseudo_classes;

	// Any properties that have been overridden in this element.
	PropertyDictionary inline_properties;
	// The definition of this element, provides applicable properties from the stylesheet.
	SharedPtr<const ElementDefinition> definition;

	PropertyIdSet dirty_properties;
};

} // namespace Rml
#endif
