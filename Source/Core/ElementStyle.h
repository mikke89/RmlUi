/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef ROCKETCOREELEMENTSTYLE_H
#define ROCKETCOREELEMENTSTYLE_H

#include "ElementDefinition.h"
#include "../../Include/Rocket/Core/Types.h"

namespace Rocket {
namespace Core {

/**
	Manages an element's style and property information.
	@author Lloyd Weehuizen
 */

class ElementStyle
{
public:
	/// Constructor
	/// @param[in] element The element this structure belongs to.
	ElementStyle(Element* element);
	~ElementStyle();

	/// Returns the element's definition, updating if necessary.
	const ElementDefinition* GetDefinition();
	
	/// Update this definition if required
	void UpdateDefinition();

	/// Sets or removes a pseudo-class on the element.
	/// @param[in] pseudo_class The pseudo class to activate or deactivate.
	/// @param[in] activate True if the pseudo-class is to be activated, false to be deactivated.
	void SetPseudoClass(const String& pseudo_class, bool activate);
	/// Checks if a specific pseudo-class has been set on the element.
	/// @param[in] pseudo_class The name of the pseudo-class to check for.
	/// @return True if the pseudo-class is set on the element, false if not.
	bool IsPseudoClassSet(const String& pseudo_class) const;
	/// Gets a list of the current active pseudo classes
	const PseudoClassList& GetActivePseudoClasses() const;

	/// Sets or removes a class on the element.
	/// @param[in] class_name The name of the class to add or remove from the class list.
	/// @param[in] activate True if the class is to be added, false to be removed.
	void SetClass(const String& class_name, bool activate);
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

	/// Sets a local property override on the element.
	/// @param[in] name The name of the new property.
	/// @param[in] property The new property to set.
	bool SetProperty(const String& name, const String& value);
	/// Sets a local property override on the element to a pre-parsed value.
	/// @param[in] name The name of the new property.
	/// @param[in] property The parsed property to set.
	bool SetProperty(const String& name, const Property& property);
	/// Removes a local property override on the element; its value will revert to that defined in
	/// the style sheet.
	/// @param[in] name The name of the local property definition to remove.
	void RemoveProperty(const String& name);
	/// Returns one of this element's properties. If this element is not defined this property, or a parent cannot
	/// be found that we can inherit the property from, the default value will be returned.
	/// @param[in] name The name of the property to fetch the value for.
	/// @return The value of this property for this element, or NULL if no property exists with the given name.
	const Property* GetProperty(const String& name);
	/// Returns one of this element's properties. If this element is not defined this property, NULL will be
	/// returned.
	/// @param[in] name The name of the property to fetch the value for.
	/// @return The value of this property for this element, or NULL if this property has not been explicitly defined for this element.
	const Property* GetLocalProperty(const String& name);
	/// Returns the local properties, excluding any properties from local class.
	/// @return The local properties for this element, or NULL if no properties defined
	const PropertyMap* GetLocalProperties() const;

	/// Resolves a property with units of length or percentage to 'px'. Percentages are resolved by scaling the base value.
	/// @param[in] name The property to resolve the value for.
	/// @param[in] base_value The value that is scaled by the percentage value, if it is a percentage.
	/// @return The resolved value in 'px' unit.
	float ResolveLengthPercentage(const Property *property, float base_value);
	/// Resolves a property with units of number, length or percentage. Lengths are resolved to 'px'. 
	/// Number and percentages are resolved by scaling the size of the specified target.
	float ResolveNumberLengthPercentage(const Property* property, RelativeTarget relative_target);

	/// Iterates over the properties defined on the element.
	/// @param[inout] index Index of the property to fetch. This is incremented to the next valid index after the fetch. Indices are not necessarily incremental.
	/// @param[out] pseudo_classes The pseudo-classes the property is defined by.
	/// @param[out] name The name of the property at the specified index.
	/// @param[out] property The property at the specified index.
	/// @return True if a property was successfully fetched.
	bool IterateProperties(int& index, String& name, const Property*& property, const PseudoClassList** pseudo_classes = nullptr);

	/// Returns the active style sheet for this element. This may be NULL.
	StyleSheet* GetStyleSheet() const;

	/// Mark definition and all children dirty
	void DirtyDefinition();
	/// Dirty all child definitions
	void DirtyChildDefinitions();

	// Dirties every property.
	void DirtyProperties();
	// Dirties em-relative properties.
	void DirtyEmProperties();
	// Dirties font-size on child elements if appropriate.
	void DirtyInheritedEmProperties();
	// Dirties rem properties.
	void DirtyRemProperties();
	// Dirties dp properties.
	void DirtyDpProperties();

	/// Turns the local and inherited properties into computed values for this element. These values can in turn be used during the layout procedure.
	/// Must be called in correct order, from document root to children elements.
	void ComputeValues(Style::ComputedValues& values, const Style::ComputedValues* parent_values, const Style::ComputedValues* document_values, bool values_are_default_initialized, float dp_ratio);

private:
	// Sets a single property as dirty.
	void DirtyProperty(const String& property);
	// Sets a list of properties as dirty.
	void DirtyProperties(const PropertyNameList& properties, bool clear_em_properties = true);
	// Sets a list of our potentially inherited properties as dirtied by an ancestor.
	void DirtyInheritedProperties(const PropertyNameList& properties);

	static const Property* GetLocalProperty(const String & name, PropertyDictionary * local_properties, ElementDefinition * definition, const PseudoClassList & pseudo_classes);
	static const Property* GetProperty(const String & name, Element * element, PropertyDictionary * local_properties, ElementDefinition * definition, const PseudoClassList & pseudo_classes);
	static void TransitionPropertyChanges(Element * element, PropertyNameList & properties, PropertyDictionary * local_properties, ElementDefinition * old_definition, ElementDefinition * new_definition,
		const PseudoClassList & pseudo_classes_before, const PseudoClassList & pseudo_classes_after);

	// Element these properties belong to
	Element* element;

	// The list of classes applicable to this object.
	StringList classes;
	// This element's current pseudo-classes.
	PseudoClassList pseudo_classes;

	// Any properties that have been overridden in this element.
	PropertyDictionary* local_properties;
	// All properties (including inherited) that are EM-relative.
	PropertyNameList* em_properties;
	// The definition of this element; if this is NULL one will be fetched from the element's style.
	ElementDefinition* definition;
	// Set if a new element definition should be fetched from the style.
	bool definition_dirty;
};

}
}

#endif
