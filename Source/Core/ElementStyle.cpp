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

#include "precompiled.h"
#include "ElementStyle.h"
#include <algorithm>
#include "../../Include/Rocket/Core/ElementDocument.h"
#include "../../Include/Rocket/Core/ElementUtilities.h"
#include "../../Include/Rocket/Core/Log.h"
#include "../../Include/Rocket/Core/Math.h"
#include "../../Include/Rocket/Core/Property.h"
#include "../../Include/Rocket/Core/PropertyDefinition.h"
#include "../../Include/Rocket/Core/PropertyDictionary.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"
#include "../../Include/Rocket/Core/TransformPrimitive.h"
#include "ElementBackground.h"
#include "ElementBorder.h"
#include "ElementDecoration.h"
#include "ElementDefinition.h"
#include "FontFaceHandle.h"


namespace Rocket {
namespace Core {

static const Style::ComputedValues DefaultComputedValues;


ElementStyle::ElementStyle(Element* _element)
{
	local_properties = NULL;
	em_properties = NULL;
	definition = NULL;
	element = _element;

	definition_dirty = true;
}

ElementStyle::~ElementStyle()
{
	if (local_properties != NULL)
		delete local_properties;
	if (em_properties != NULL)
		delete em_properties;

	if (definition != NULL)
		definition->RemoveReference();
}


// Returns the element's definition, updating if necessary.
const ElementDefinition* ElementStyle::GetDefinition()
{
	return definition;
}



// Returns one of this element's properties.
const Property* ElementStyle::GetLocalProperty(const String& name, PropertyDictionary* local_properties, ElementDefinition* definition, const PseudoClassList& pseudo_classes)
{
	// Check for overriding local properties.
	if (local_properties != NULL)
	{
		const Property* property = local_properties->GetProperty(name);
		if (property != NULL)
			return property;
	}

	// Check for a property defined in an RCSS rule.
	if (definition != NULL)
		return definition->GetProperty(name, pseudo_classes);

	return NULL;
}

// Returns one of this element's properties.
const Property* ElementStyle::GetProperty(const String& name, Element* element, PropertyDictionary* local_properties, ElementDefinition* definition, const PseudoClassList& pseudo_classes)
{
	const Property* local_property = GetLocalProperty(name, local_properties, definition, pseudo_classes);
	if (local_property != NULL)
		return local_property;

	// Fetch the property specification.
	const PropertyDefinition* property = StyleSheetSpecification::GetProperty(name);
	if (property == NULL)
		return NULL;

	// If we can inherit this property, return our parent's property.
	if (property->IsInherited())
	{
		Element* parent = element->GetParentNode();
		while (parent != NULL)
		{
			const Property* parent_property = parent->GetStyle()->GetLocalProperty(name);
			if (parent_property)
				return parent_property;

			parent = parent->GetParentNode();
		}
	}

	// No property available! Return the default value.
	return property->GetDefaultValue();
}

// Apply transition to relevant properties if a transition is defined on element.
// Properties that are part of a transition are removed from the properties list.
void ElementStyle::TransitionPropertyChanges(Element* element, PropertyNameList& properties, PropertyDictionary* local_properties, ElementDefinition* old_definition, ElementDefinition* new_definition,
	const PseudoClassList& pseudo_classes_before, const PseudoClassList& pseudo_classes_after)
{
	ROCKET_ASSERT(element);
	if (!old_definition || !new_definition || properties.empty())
		return;

	if (const Property* transition_property = GetLocalProperty(TRANSITION, local_properties, new_definition, pseudo_classes_after))
	{
		auto transition_list = transition_property->Get<TransitionList>();

		if (!transition_list.none)
		{
			auto add_transition = [&](const Transition& transition) {
				bool transition_added = false;
				const Property* start_value = GetProperty(transition.name, element, local_properties, old_definition, pseudo_classes_before);
				const Property* target_value = GetProperty(transition.name, element, nullptr, new_definition, pseudo_classes_after);
				if (start_value && target_value && (*start_value != *target_value))
					transition_added = element->StartTransition(transition, *start_value, *target_value);
				return transition_added;
			};

			if (transition_list.all)
			{
				Transition transition = transition_list.transitions[0];
				for (auto it = properties.begin(); it != properties.end(); )
				{
					transition.name = *it;
					if (add_transition(transition))
						it = properties.erase(it);
					else
						++it;
				}
			}
			else
			{
				for (auto& transition : transition_list.transitions)
				{
					if (auto it = properties.find(transition.name); it != properties.end())
					{
						if (add_transition(transition))
							properties.erase(it);
					}
				}
			}
		}
	}
}
	
void ElementStyle::UpdateDefinition()
{
	if (definition_dirty)
	{
		definition_dirty = false;
		
		ElementDefinition* new_definition = NULL;
		
		const StyleSheet* style_sheet = GetStyleSheet();
		if (style_sheet != NULL)
		{
			new_definition = style_sheet->GetElementDefinition(element);
		}
		
		// Switch the property definitions if the definition has changed.
		if (new_definition != definition || new_definition == NULL)
		{
			PropertyNameList properties;
			
			if (definition != NULL)
				definition->GetDefinedProperties(properties, pseudo_classes);

			if (new_definition != NULL)
				new_definition->GetDefinedProperties(properties, pseudo_classes);

			TransitionPropertyChanges(element, properties, local_properties, definition, new_definition, pseudo_classes, pseudo_classes);

			if (definition != NULL)
				definition->RemoveReference();

			definition = new_definition;
			
			DirtyProperties(properties);
			element->GetElementDecoration()->DirtyDecorators(true);
		}
		else if (new_definition != NULL)
		{
			new_definition->RemoveReference();
		}
	}
}



// Sets or removes a pseudo-class on the element.
void ElementStyle::SetPseudoClass(const String& pseudo_class, bool activate)
{
	size_t num_pseudo_classes = pseudo_classes.size();

	auto pseudo_classes_before = pseudo_classes;

	if (activate)
		pseudo_classes.push_back(pseudo_class);
	else
	{
		// In case of duplicates, we do a loop here. We could do a sort and unique instead,
		// but that might even be slower for a small list with few duplicates, which
		// is probably the most common case.
		auto it = std::find(pseudo_classes.begin(), pseudo_classes.end(), pseudo_class);
		while(it != pseudo_classes.end())
		{
			pseudo_classes.erase(it);
			it = std::find(pseudo_classes.begin(), pseudo_classes.end(), pseudo_class);
		}
	}

	if (pseudo_classes.size() != num_pseudo_classes)
	{
		element->GetElementDecoration()->DirtyDecorators(false);

		if (definition != NULL)
		{
			PropertyNameList properties;
			definition->GetDefinedProperties(properties, pseudo_classes, pseudo_class);

			TransitionPropertyChanges(element, properties, local_properties, definition, definition, pseudo_classes_before, pseudo_classes);

			DirtyProperties(properties);

			switch (definition->GetPseudoClassVolatility(pseudo_class))
			{
				case ElementDefinition::FONT_VOLATILE:
					element->DirtyFont();
					break;

				case ElementDefinition::STRUCTURE_VOLATILE:
					DirtyChildDefinitions();
					break;

				default:
					break;
			}
		}
	}
}

// Checks if a specific pseudo-class has been set on the element.
bool ElementStyle::IsPseudoClassSet(const String& pseudo_class) const
{
	return (std::find(pseudo_classes.begin(), pseudo_classes.end(), pseudo_class) != pseudo_classes.end());
}

const PseudoClassList& ElementStyle::GetActivePseudoClasses() const
{
	return pseudo_classes;
}

// Sets or removes a class on the element.
void ElementStyle::SetClass(const String& class_name, bool activate)
{
	StringList::iterator class_location = std::find(classes.begin(), classes.end(), class_name);

	if (activate)
	{
		if (class_location == classes.end())
		{
			classes.push_back(class_name);
			DirtyDefinition();
		}
	}
	else
	{
		if (class_location != classes.end())
		{
			classes.erase(class_location);
			DirtyDefinition();
		}
	}
}

// Checks if a class is set on the element.
bool ElementStyle::IsClassSet(const String& class_name) const
{
	return std::find(classes.begin(), classes.end(), class_name) != classes.end();
}

// Specifies the entire list of classes for this element. This will replace any others specified.
void ElementStyle::SetClassNames(const String& class_names)
{
	classes.clear();
	StringUtilities::ExpandString(classes, class_names, ' ');
	DirtyDefinition();
}

// Returns the list of classes specified for this element.
String ElementStyle::GetClassNames() const
{
	String class_names;
	for (size_t i = 0; i < classes.size(); i++)
	{
		if (i != 0)
		{
			class_names += " ";
		}
		class_names += classes[i];
	}

	return class_names;
}

// Sets a local property override on the element.
bool ElementStyle::SetProperty(const String& name, const String& value)
{
	if (local_properties == NULL)
		local_properties = new PropertyDictionary();

	if (StyleSheetSpecification::ParsePropertyDeclaration(*local_properties, name, value))
	{
		DirtyProperty(name);
		return true;
	}
	else
	{
		Log::Message(Log::LT_WARNING, "Syntax error parsing inline property declaration '%s: %s;'.", name.c_str(), value.c_str());
		return false;
	}
}

// Sets a local property override on the element to a pre-parsed value.
bool ElementStyle::SetProperty(const String& name, const Property& property)
{
	Property new_property = property;

	new_property.definition = StyleSheetSpecification::GetProperty(name);
	if (new_property.definition == NULL)
		return false;

	if (local_properties == NULL)
		local_properties = new PropertyDictionary();

	local_properties->SetProperty(name, new_property);
	DirtyProperty(name);

	return true;
}

// Removes a local property override on the element.
void ElementStyle::RemoveProperty(const String& name)
{
	if (local_properties == NULL)
		return;

	if (local_properties->GetProperty(name) != NULL)
	{
		local_properties->RemoveProperty(name);
		DirtyProperty(name);
	}
}



// Returns one of this element's properties.
const Property* ElementStyle::GetProperty(const String& name)
{
	return GetProperty(name, element, local_properties, definition, pseudo_classes);
}

// Returns one of this element's properties.
const Property* ElementStyle::GetLocalProperty(const String& name)
{
	return GetLocalProperty(name, local_properties, definition, pseudo_classes);
}

const PropertyMap * ElementStyle::GetLocalProperties() const
{
	if (local_properties)
		return &local_properties->GetProperties();
	return NULL;
}

float ElementStyle::ResolveLength(const Property * property)
{
	if (!property)
	{
		ROCKET_ERROR;
		return 0.0f;
	}

	if (!(property->unit & Property::LENGTH))
	{
		ROCKET_ERRORMSG("Trying to resolve length on a non-length property.");
		return 0.0f;
	}

	switch (property->unit)
	{
	case Property::NUMBER:
	case Property::PX:
		return property->value.Get< float >();
	case Property::EM:
		return property->value.Get< float >()* element->GetComputedValues().font_size;
	case Property::REM:
		return property->value.Get< float >()* element->GetOwnerDocument()->GetComputedValues().font_size;
	case Property::DP:
		return property->value.Get< float >() * ElementUtilities::GetDensityIndependentPixelRatio(element);
	}

	// Values based on pixels-per-inch.
	if (property->unit & Property::PPI_UNIT)
	{
		float inch = property->value.Get< float >() * element->GetRenderInterface()->GetPixelsPerInch();

		switch (property->unit)
		{
		case Property::INCH: // inch
			return inch;
		case Property::CM: // centimeter
			return inch * (1.0f / 2.54f);
		case Property::MM: // millimeter
			return inch * (1.0f / 25.4f);
		case Property::PT: // point
			return inch * (1.0f / 72.0f);
		case Property::PC: // pica
			return inch * (1.0f / 6.0f);
		}
	}

	// We're not a numeric property; return 0.
	return 0.0f;
}

float ElementStyle::ResolveAngle(const Property * property)
{
	switch (property->unit)
	{
	case Property::NUMBER:
	case Property::DEG:
		return Math::DegreesToRadians(property->value.Get< float >());
	case Property::RAD:
		return property->value.Get< float >();
	case Property::PERCENT:
		return property->value.Get< float >() * 0.01f * 2.0f * Math::ROCKET_PI;
	}
	ROCKET_ERRORMSG("Trying to resolve angle on a non-angle property.");
	return 0.0f;
}

float ElementStyle::ResolveNumericProperty(const String& property_name, const Property * property)
{
	if ((property->unit & Property::LENGTH) && !(property->unit == Property::EM && property_name == FONT_SIZE))
	{
		return ResolveLength(property);
	}

	auto definition = property->definition;
	if (!definition) definition = StyleSheetSpecification::GetProperty(property_name);
	if (!definition) return 0.0f;

	auto relative_target = definition->GetRelativeTarget();
	
	return ResolveNumericProperty(property, relative_target);
}

float ElementStyle::ResolveNumericProperty(const Property * property, RelativeTarget relative_target)
{
	// There is an exception on font-size properties, as 'em' units here refer to parent font size instead
	if ((property->unit & Property::LENGTH) && !(property->unit == Property::EM && relative_target == RelativeTarget::ParentFontSize))
	{
		return ResolveLength(property);
	}

	float base_value = 0.0f;

	switch (relative_target)
	{
	case RelativeTarget::None:
		base_value = 1.0f;
		break;
	case RelativeTarget::ContainingBlockWidth:
		base_value = element->GetContainingBlock().x;
		break;
	case RelativeTarget::ContainingBlockHeight:
		base_value = element->GetContainingBlock().y;
		break;
	case RelativeTarget::FontSize:
		base_value = element->GetComputedValues().font_size;
		break;
	case RelativeTarget::ParentFontSize:
		base_value = element->GetParentNode()->GetComputedValues().font_size;
		break;
	case RelativeTarget::LineHeight:
		base_value = element->GetLineHeight();
		break;
	default:
		break;
	}

	float scale_value = 0.0f;

	switch (property->unit)
	{
	case Property::EM:
	case Property::NUMBER:
		scale_value = property->value.Get< float >();
		break;
	case Property::PERCENT:
		scale_value = property->value.Get< float >() * 0.01f;
		break;
	}

	return base_value * scale_value;
}

// Resolves one of this element's properties.
float ElementStyle::ResolveProperty(const Property* property, float base_value)
{
	if (!property)
	{
		ROCKET_ERROR;
		return 0.0f;
	}

	switch (property->unit)
	{
		case Property::NUMBER:
		case Property::PX:
		case Property::RAD:
			return property->value.Get< float >();

		case Property::PERCENT:
			return base_value * property->value.Get< float >() * 0.01f;

		case Property::EM:
			return property->value.Get< float >() * element->GetComputedValues().font_size;
		case Property::REM:
			return property->value.Get< float >() * element->GetOwnerDocument()->GetComputedValues().font_size;
		case Property::DP:
			return property->value.Get< float >() * ElementUtilities::GetDensityIndependentPixelRatio(element);

		case Property::DEG:
			return Math::DegreesToRadians(property->value.Get< float >());
	}

	// Values based on pixels-per-inch.
	if (property->unit & Property::PPI_UNIT)
	{
		float inch = property->value.Get< float >() * element->GetRenderInterface()->GetPixelsPerInch();
		
		switch (property->unit)
		{
			case Property::INCH: // inch
				return inch;
			case Property::CM: // centimeter
				return inch * (1.0f / 2.54f);
			case Property::MM: // millimeter
				return inch * (1.0f / 25.4f);
			case Property::PT: // point
				return inch * (1.0f / 72.0f);
			case Property::PC: // pica
				return inch * (1.0f / 6.0f);
		}
	}

	// We're not a numeric property; return 0.
	return 0.0f;
}

// Resolves one of this element's properties.
float ElementStyle::ResolveProperty(const String& name, float base_value)
{
	const Property* property = GetProperty(name);
	if (!property)
	{
		ROCKET_ERROR;
		return 0.0f;
	}

	// The calculated value of the font-size property is inherited, so we need to check if this
	// is an inherited property. If so, then we return our parent's font size instead.
	if (name == FONT_SIZE && property->unit & Property::RELATIVE_UNIT)
	{
		// If the rem unit is used, the font-size is inherited directly from the document,
		// otherwise we use the parent's font size.
		if (property->unit & Property::REM)
		{
			Rocket::Core::ElementDocument* owner_document = element->GetOwnerDocument();
			if (owner_document == NULL)
				return 0;

			base_value = owner_document->ResolveProperty(FONT_SIZE, 0);
		}
		else
		{
			Rocket::Core::Element* parent = element->GetParentNode();
			if (parent == NULL)
				return 0;

			if (GetLocalProperty(FONT_SIZE) == NULL)
				return parent->ResolveProperty(FONT_SIZE, 0);

			// The base value for font size is always the height of *this* element's parent's font.
			base_value = parent->ResolveProperty(FONT_SIZE, 0);
		}

		switch (property->unit)
		{
			case Property::PERCENT:
				return base_value * property->value.Get< float >() * 0.01f;

			case Property::EM:
				return property->value.Get< float >() * base_value;

			case Property::REM:
				// If an rem-relative font size is specified, it is expressed relative to the document's
				// font height.
				return property->value.Get< float >() * element->GetOwnerDocument()->GetComputedValues().font_size;
		}
	}

	return ResolveProperty(property, base_value);
}

// Iterates over the properties defined on the element.
bool ElementStyle::IterateProperties(int& index, String& name, const Property*& property, const PseudoClassList** property_pseudo_classes)
{
	// First check for locally defined properties.
	if (local_properties != NULL)
	{
		if (index < local_properties->GetNumProperties())
		{
			PropertyMap::const_iterator i = local_properties->GetProperties().begin();
			for (int count = 0; count < index; ++count)
				++i;

			name = (*i).first;
			property = &((*i).second);
			if (property_pseudo_classes)
				* property_pseudo_classes = nullptr;
			++index;

			return true;
		}
	}

	const ElementDefinition* definition = GetDefinition();
	if (definition != NULL)
	{
		int index_offset = 0;
		if (local_properties != NULL)
			index_offset = local_properties->GetNumProperties();

		// Offset the index to be relative to the definition before we start indexing. When we do get a property back,
		// check that it hasn't been overridden by the element's local properties; if so, continue on to the next one.
		index -= index_offset;
		while (definition->IterateProperties(index, pseudo_classes, name, property, property_pseudo_classes))
		{
			if (local_properties == NULL ||
				local_properties->GetProperty(name) == NULL)
			{
				index += index_offset;
				return true;
			}
		}

		return false;
	}

	return false;
}

// Returns the active style sheet for this element. This may be NULL.
StyleSheet* ElementStyle::GetStyleSheet() const
{
	ElementDocument* document = element->GetOwnerDocument();
	if (document != NULL)
		return document->GetStyleSheet();

	return NULL;
}

void ElementStyle::DirtyDefinition()
{
	definition_dirty = true;
	DirtyChildDefinitions();
}

void ElementStyle::DirtyChildDefinitions()
{
	for (int i = 0; i < element->GetNumChildren(true); i++)
		element->GetChild(i)->GetStyle()->DirtyDefinition();
}

// Dirties every property.
void ElementStyle::DirtyProperties()
{
	const PropertyNameList &properties = StyleSheetSpecification::GetRegisteredProperties();
	DirtyProperties(properties);
}

// Dirties em-relative properties.
void ElementStyle::DirtyEmProperties()
{
	if (!em_properties)
	{
		// Check if any of these are currently em-relative. If so, dirty them.
		em_properties = new PropertyNameList;
		for (auto& property : StyleSheetSpecification::GetRegisteredProperties())
		{
			// Skip font-size; this is relative to our parent's em, not ours.
			if (property == FONT_SIZE)
				continue;

			// Get this property from this element. If this is em-relative, then add it to the list to
			// dirty.
			if (element->GetProperty(property)->unit == Property::EM)
				em_properties->insert(property);
		}
	}

	if (!em_properties->empty())
		DirtyProperties(*em_properties, false);

	// Now dirty all of our descendant's font-size properties that are relative to ems.
	int num_children = element->GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		element->GetChild(i)->GetStyle()->DirtyInheritedEmProperties();
}

// Dirties font-size on child elements if appropriate.
void ElementStyle::DirtyInheritedEmProperties()
{
	const Property* font_size = element->GetLocalProperty(FONT_SIZE);
	if (font_size == NULL)
	{
		int num_children = element->GetNumChildren(true);
		for (int i = 0; i < num_children; ++i)
			element->GetChild(i)->GetStyle()->DirtyInheritedEmProperties();
	}
	else
	{
		if (font_size->unit & Property::RELATIVE_UNIT)
			DirtyProperty(FONT_SIZE);
	}
}

// Dirties rem properties.
void ElementStyle::DirtyRemProperties()
{
	const PropertyNameList &properties = StyleSheetSpecification::GetRegisteredProperties();
	PropertyNameList rem_properties;

	// Dirty all the properties of this element that use the rem unit.
	for (PropertyNameList::const_iterator list_iterator = properties.begin(); list_iterator != properties.end(); ++list_iterator)
	{
		if (element->GetProperty(*list_iterator)->unit == Property::REM)
			rem_properties.insert(*list_iterator);
	}

	if (!rem_properties.empty())
		DirtyProperties(rem_properties, false);

	// Now dirty all of our descendant's properties that use the rem unit.
	int num_children = element->GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		element->GetChild(i)->GetStyle()->DirtyRemProperties();
}

void ElementStyle::DirtyDpProperties()
{
	const PropertyNameList &properties = StyleSheetSpecification::GetRegisteredProperties();
	PropertyNameList dp_properties;

	// Dirty all the properties of this element that use the dp unit.
	for (PropertyNameList::const_iterator list_iterator = properties.begin(); list_iterator != properties.end(); ++list_iterator)
	{
		if (element->GetProperty(*list_iterator)->unit == Property::DP)
			dp_properties.insert(*list_iterator);
	}

	if (!dp_properties.empty())
		DirtyProperties(dp_properties, false);

	// Now dirty all of our descendant's properties that use the dp unit.
	int num_children = element->GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		element->GetChild(i)->GetStyle()->DirtyDpProperties();
}

// Sets a single property as dirty.
void ElementStyle::DirtyProperty(const String& property)
{
	PropertyNameList properties;
	properties.insert(String(property));

	DirtyProperties(properties);
}

// Sets a list of properties as dirty.
void ElementStyle::DirtyProperties(const PropertyNameList& properties, bool clear_em_properties)
{
	if (properties.empty())
		return;

	bool all_inherited_dirty = 
		&properties == &StyleSheetSpecification::GetRegisteredProperties() ||
		StyleSheetSpecification::GetRegisteredProperties() == properties ||
		StyleSheetSpecification::GetRegisteredInheritedProperties() == properties;


	if (all_inherited_dirty)
	{
		const PropertyNameList &all_inherited_properties = StyleSheetSpecification::GetRegisteredInheritedProperties();
		for (int i = 0; i < element->GetNumChildren(true); i++)
			element->GetChild(i)->GetStyle()->DirtyInheritedProperties(all_inherited_properties);
	}
	else
	{
		PropertyNameList inherited_properties;

		for (PropertyNameList::const_iterator i = properties.begin(); i != properties.end(); ++i)
		{
			// If this property is an inherited property, then push it into the list to be passed onto our children.
			const PropertyDefinition* property = StyleSheetSpecification::GetProperty(*i);
			if (property != NULL &&
				property->IsInherited())
				inherited_properties.insert(*i);
		}

		// Pass the list of those properties that are inherited onto our children.
		if (!inherited_properties.empty())
		{
			for (int i = 0; i < element->GetNumChildren(true); i++)
				element->GetChild(i)->GetStyle()->DirtyInheritedProperties(inherited_properties);
		}
	}

	// clear the list of EM-properties, we will refill it in DirtyEmProperties
	if (clear_em_properties && em_properties != NULL)
	{
		delete em_properties;
		em_properties = NULL;
	}

	// And send the event.
	element->DirtyProperties(properties);
}

// Sets a list of our potentially inherited properties as dirtied by an ancestor.
void ElementStyle::DirtyInheritedProperties(const PropertyNameList& properties)
{
	bool clear_em_properties = em_properties != NULL;

	PropertyNameList inherited_properties;
	for (PropertyNameList::const_iterator i = properties.begin(); i != properties.end(); ++i)
	{
		const Property *property = GetLocalProperty((*i));
		if (property == NULL)
		{
			inherited_properties.insert(*i);
			if (!clear_em_properties && em_properties != NULL && em_properties->find((*i)) != em_properties->end()) {
				clear_em_properties = true;
			}
		}
	}

	if (inherited_properties.empty())
		return;

	// clear the list of EM-properties, we will refill it in DirtyEmProperties
	if (clear_em_properties && em_properties != NULL)
	{
		delete em_properties;
		em_properties = NULL;
	}

	// Pass the list of those properties that this element doesn't override onto our children.
	for (int i = 0; i < element->GetNumChildren(true); i++)
		element->GetChild(i)->GetStyle()->DirtyInheritedProperties(inherited_properties);

	element->DirtyProperties(properties);
}


static float ComputeLength(const Property* property, float font_size, float document_font_size, float dp_ratio, float pixels_per_inch)
{
	// Note that percentages are not lengths! They have to be resolved elsewhere.
	if (!property)
	{
		ROCKET_ERROR;
		return 0.0f;
	}

	switch (property->unit)
	{
	case Property::NUMBER:
	case Property::PX:
	case Property::RAD:
		return property->value.Get< float >();

	case Property::EM:
		return property->value.Get< float >() * font_size;
	case Property::REM:
		return property->value.Get< float >() * document_font_size;
	case Property::DP:
		return property->value.Get< float >() * dp_ratio;

	case Property::DEG:
		return Math::DegreesToRadians(property->value.Get< float >());
	default: 
		break;
	}

	// Values based on pixels-per-inch.
	if (property->unit & Property::PPI_UNIT)
	{
		float inch = property->value.Get< float >() * pixels_per_inch;

		switch (property->unit)
		{
		case Property::INCH: // inch
			return inch;
		case Property::CM: // centimeter
			return inch * (1.0f / 2.54f);
		case Property::MM: // millimeter
			return inch * (1.0f / 25.4f);
		case Property::PT: // point
			return inch * (1.0f / 72.0f);
		case Property::PC: // pica
			return inch * (1.0f / 6.0f);
		default:
			break;
		}
	}

	// We're not a numeric property; return 0.
	return 0.0f;
}




static float ComputeAbsoluteLength(const Property& property, float dp_ratio, float pixels_per_inch)
{
	ROCKET_ASSERT(property.unit & Property::ABSOLUTE_LENGTH);

	switch (property.unit)
	{
	case Property::PX:
		return property.value.Get< float >();
	case Property::DP:
		return property.value.Get< float >()* dp_ratio;
	default:
		// Values based on pixels-per-inch.
		if (property.unit & Property::PPI_UNIT)
		{
			float inch = property.value.Get< float >() * pixels_per_inch;

			switch (property.unit)
			{
			case Property::INCH: // inch
				return inch;
			case Property::CM: // centimeter
				return inch * (1.0f / 2.54f);
			case Property::MM: // millimeter
				return inch * (1.0f / 25.4f);
			case Property::PT: // point
				return inch * (1.0f / 72.0f);
			case Property::PC: // pica
				return inch * (1.0f / 6.0f);
			}
		}
	}

	ROCKET_ERROR;
	return 0.0f;
}



// Resolves one of this element's properties.
static float ComputeFontsize(const Property& property, const Style::ComputedValues& values, const Style::ComputedValues* parent_values, const Style::ComputedValues* document_values, float dp_ratio, float pixels_per_inch)
{
	// The calculated value of the font-size property is inherited, so we need to check if this
	// is an inherited property. If so, then we return our parent's font size instead.
	if (property.unit & Property::RELATIVE_UNIT)
	{
		float multiplier = 1.0f;

		switch (property.unit)
		{
		case Property::PERCENT:
			multiplier = 0.01f;
			[[fallthrough]];
		case Property::EM:
			if (!parent_values)
				return 0;
			return property.value.Get< float >() * multiplier * parent_values->font_size;

		case Property::REM:
			if (!document_values)
				return 0;
			// If the current element is a document, the rem unit is relative to the default size
			if(&values == document_values)
				return property.value.Get< float >() * DefaultComputedValues.font_size;
			// Otherwise it is relative to the document font size
			return property.value.Get< float >() * document_values->font_size;
		default:
			ROCKET_ERRORMSG("A relative unit must be percentage, em or rem.");
		}
	}

	return ComputeAbsoluteLength(property, dp_ratio, pixels_per_inch);
}


static inline Style::Clip ComputeClip(const Property* property)
{
	int value = property->Get<int>();
	if (property->unit == Property::KEYWORD)
		return (value == CLIP_NONE ? Style::Clip::None : Style::Clip::Auto);
	else if (property->unit == Property::NUMBER)
		return (Style::Clip)value;
	ROCKET_ERRORMSG("Invalid clip type");
	return Style::Clip::Auto;
}

static inline Style::LineHeight ComputeLineHeight(const Property* property, float font_size, float document_font_size, float dp_ratio, float pixels_per_inch)
{
	if (property->unit & Property::LENGTH)
	{
		float value = ComputeLength(property, font_size, document_font_size, dp_ratio, pixels_per_inch);
		return Style::LineHeight(value, Style::LineHeight::Length, value);
	}

	float scale_factor = 1.0f;

	switch (property->unit)
	{
	case Property::NUMBER:
		scale_factor = property->value.Get< float >();
		break;
	case Property::PERCENT:
		scale_factor = property->value.Get< float >() * 0.01f;
		break;
	default:
		ROCKET_ERRORMSG("Invalid unit for line-height");
	}

	float value = font_size * scale_factor;
	return Style::LineHeight(value, Style::LineHeight::Number, scale_factor);
}


static inline Style::VerticalAlign ComputeVerticalAlign(const Property* property, float line_height, float font_size, float document_font_size, float dp_ratio, float pixels_per_inch)
{
	if (property->unit & Property::LENGTH)
	{
		float value = ComputeLength(property, font_size, document_font_size, dp_ratio, pixels_per_inch);
		return Style::VerticalAlign(value);
	}
	else if (property->unit & Property::PERCENT)
	{
		return Style::VerticalAlign(property->Get<float>() * line_height);
	}

	ROCKET_ASSERT(property->unit & Property::KEYWORD);
	return Style::VerticalAlign((Style::VerticalAlign::Type)property->Get<int>());
}

static inline LengthPercentage ComputeLengthPercentage(const Property* property, float font_size, float document_font_size, float dp_ratio, float pixels_per_inch)
{
	if (property->unit & Property::PERCENT)
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	return LengthPercentage(LengthPercentage::Length, ComputeLength(property, font_size, document_font_size, dp_ratio, pixels_per_inch));
}


static inline LengthPercentageAuto ComputeLengthPercentageAuto(const Property* property, float font_size, float document_font_size, float dp_ratio, float pixels_per_inch)
{
	// Assuming here that 'auto' is the only possible keyword
	if (property->unit & Property::PERCENT)
		return LengthPercentageAuto(LengthPercentageAuto::Percentage, property->Get<float>());
	else if (property->unit & Property::KEYWORD)
		return LengthPercentageAuto(LengthPercentageAuto::Auto);

	return LengthPercentageAuto(LengthPercentageAuto::Length, ComputeLength(property, font_size, document_font_size, dp_ratio, pixels_per_inch));
}

static inline LengthPercentage ComputeOrigin(const Property* property, float font_size, float document_font_size, float dp_ratio, float pixels_per_inch)
{
	using namespace Style;
	static_assert((int)OriginX::Left == (int)OriginY::Top && (int)OriginX::Center == (int)OriginY::Center && (int)OriginX::Right == (int)OriginY::Bottom, "");

	if (property->unit & Property::KEYWORD)
	{
		float percent = 0.0f;
		OriginX origin = (OriginX)property->Get<int>();
		switch (origin)
		{
		case OriginX::Left: percent = 0.0f; break;
		case OriginX::Center: percent = 50.0f; break;
		case OriginX::Right: percent = 100.f; break;
		}
		return LengthPercentage(LengthPercentage::Percentage, percent);
	}
	else if (property->unit & Property::PERCENT)
		return LengthPercentage(LengthPercentage::Percentage, property->Get<float>());

	return LengthPercentage(LengthPercentage::Length, ComputeLength(property, font_size, document_font_size, dp_ratio, pixels_per_inch));
}




// Must be called in correct order, from document root to children elements.
void ElementStyle::ComputeValues(Style::ComputedValues& values, const Style::ComputedValues* parent_values, const Style::ComputedValues* document_values, bool values_are_defaulted, float dp_ratio, float pixels_per_inch)
{
	// Generally, this is how it works (for now, we can probably be smarter about this):
	//   1. Assign default values (clears any newly dirtied properties)
	//   2. Inherit inheritable values from parent
	//   3. Assign any local properties (from inline style or stylesheet)


	// The next flag is just a small optimization, if the element was just created we don't need to copy all the default values.
	if (!values_are_defaulted)
	{
		values = DefaultComputedValues;
	}

	// Always do font-size first if dirty, because of em-relative values
	if (auto p = GetLocalProperty(FONT_SIZE))
		values.font_size = ComputeFontsize(*p, values, parent_values, document_values, dp_ratio, pixels_per_inch);
	else if (parent_values)
		values.font_size = parent_values->font_size;

	const float font_size = values.font_size;
	const float document_font_size = (document_values ? document_values->font_size : DefaultComputedValues.font_size);

	// Since vertical-align depends on line-height we compute this before iteration
	if (auto p = GetLocalProperty(LINE_HEIGHT))
	{
		values.line_height = ComputeLineHeight(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
	}
	else if (parent_values)
	{
		// Line height has a special inheritance case for numbers/percent: they inherit them directly instead of computed length, but for lengths, they inherit the length.
		// See CSS specs for details. Percent is already converted to number.
		if (parent_values->line_height.inherit_type == Style::LineHeight::Number)
			values.line_height = Style::LineHeight(font_size * parent_values->line_height.inherit_value, Style::LineHeight::Number, parent_values->line_height.inherit_value);
		else
			values.line_height = parent_values->line_height;
	}


	if (parent_values)
	{
		// Inherited properties are copied here, but may be overwritten below by locally defined properties
		// Line-height and font-size are computed above
		values.clip = parent_values->clip;
		
		values.color = parent_values->color;
		values.opacity = parent_values->opacity;

		values.font_family = parent_values->font_family;
		values.font_charset = parent_values->font_charset;
		values.font_style = parent_values->font_style;
		values.font_weight = parent_values->font_weight;

		values.text_align = parent_values->text_align;
		values.text_decoration = parent_values->text_decoration;
		values.text_transform = parent_values->text_transform;
		values.white_space = parent_values->white_space;

		values.cursor = parent_values->cursor;
		values.focus = parent_values->focus;

		values.pointer_events = parent_values->pointer_events;
	}


	int index = 0;
	String name;
	const Property* p = nullptr;

	while (IterateProperties(index, name, p))
	{
		using namespace Style;

		// @performance: Can use a switch-case with constexpr hashing function
		// Or even better: a PropertyId enum, but the problem is custom properties such as decorators. We may want to redo the decleration of these perhaps...
		// @performance: Compare to the list of actually changed properties, skip if not inside it

		if (name == MARGIN_TOP)
			values.margin_top = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == MARGIN_RIGHT)
			values.margin_right = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == MARGIN_BOTTOM)
			values.margin_bottom = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == MARGIN_LEFT)
			values.margin_left = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == PADDING_TOP)
			values.padding_top = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == PADDING_RIGHT)
			values.padding_right = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == PADDING_BOTTOM)
			values.padding_bottom = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == PADDING_LEFT)
			values.padding_left = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == BORDER_TOP_WIDTH)
			values.border_top_width = ComputeLength(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == BORDER_RIGHT_WIDTH)
			values.border_right_width = ComputeLength(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == BORDER_BOTTOM_WIDTH)
			values.border_bottom_width = ComputeLength(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == BORDER_LEFT_WIDTH)
			values.border_left_width = ComputeLength(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == BORDER_TOP_COLOR)
			values.border_top_color = p->Get<Colourb>();
		else if (name == BORDER_RIGHT_COLOR)
			values.border_right_color = p->Get<Colourb>();
		else if (name == BORDER_BOTTOM_COLOR)
			values.border_bottom_color = p->Get<Colourb>();
		else if (name == BORDER_LEFT_COLOR)
			values.border_left_color = p->Get<Colourb>();

		else if (name == DISPLAY)
			values.display = (Display)p->Get<int>();
		else if (name == POSITION)
			values.position = (Position)p->Get<int>();

		else if (name == TOP)
			values.top = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == RIGHT)
			values.right = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == BOTTOM)
			values.bottom = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == LEFT)
			values.left = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == FLOAT)
			values.float_ = (Float)p->Get<int>();
		else if (name == CLEAR)
			values.clear = (Clear)p->Get<int>();

		else if (name == Z_INDEX)
			values.z_index = (p->unit == Property::KEYWORD ? NumberAuto(NumberAuto::Auto) : NumberAuto(NumberAuto::Number, p->Get<float>()));

		else if (name == WIDTH)
			values.width = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == MIN_WIDTH)
			values.min_width = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == MAX_WIDTH)
			values.max_width = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == HEIGHT)
			values.height = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == MIN_HEIGHT)
			values.min_height = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == MAX_HEIGHT)
			values.max_height = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		// (Line-height computed above)
		else if (name == VERTICAL_ALIGN)
			values.vertical_align = ComputeVerticalAlign(p, values.line_height.value, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == OVERFLOW_X)
			values.overflow_x = (Overflow)p->Get< int >();
		else if (name == OVERFLOW_Y)
			values.overflow_y = (Overflow)p->Get< int >();
		else if (name == CLIP)
			values.clip = ComputeClip(p);
		else if (name == VISIBILITY)
			values.visibility = (Visibility)p->Get< int >();


		else if (name == BACKGROUND_COLOR)
			values.background_color = p->Get<Colourb>();
		else if (name == COLOR)
			values.color = p->Get<Colourb>();
		else if (name == IMAGE_COLOR)
			values.image_color = p->Get<Colourb>();
		else if (name == OPACITY)
			values.opacity = p->Get<float>();


		else if (name == FONT_FAMILY)
			values.font_family = p->Get<String>();
		else if (name == FONT_CHARSET)
			values.font_charset = p->Get<String>();
		else if (name == FONT_STYLE)
			values.font_style = (FontStyle)p->Get< int >();
		else if (name == FONT_WEIGHT)
			values.font_weight = (FontWeight)p->Get< int >();
		// (font-size computed above)

		else if (name == TEXT_ALIGN)
			values.text_align = (TextAlign)p->Get< int >();
		else if (name == TEXT_DECORATION)
			values.text_decoration = (TextDecoration)p->Get< int >();
		else if (name == TEXT_TRANSFORM)
			values.text_transform = (TextTransform)p->Get< int >();
		else if (name == WHITE_SPACE)
			values.white_space = (WhiteSpace)p->Get< int >();

		else if (name == CURSOR)
			values.cursor = p->Get< String >();

		else if (name == DRAG)
			values.drag = (Drag)p->Get< int >();
		else if (name == TAB_INDEX)
			values.tab_index = (TabIndex)p->Get< int >();
		else if (name == FOCUS)
			values.focus = (Focus)p->Get<int>();
		else if (name == SCROLLBAR_MARGIN)
			values.scrollbar_margin = ComputeLength(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == POINTER_EVENTS)
			values.pointer_events = (PointerEvents)p->Get<int>();

		else if (name == PERSPECTIVE)
			values.perspective = ComputeLength(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == PERSPECTIVE_ORIGIN_X)
			values.perspective_origin_x = ComputeOrigin(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == PERSPECTIVE_ORIGIN_Y)
			values.perspective_origin_y = ComputeOrigin(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == TRANSFORM)
			values.transform = p->Get<TransformRef>();
		else if(name == TRANSFORM_ORIGIN_X)
			values.transform_origin_x = ComputeOrigin(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == TRANSFORM_ORIGIN_Y)
			values.transform_origin_y = ComputeOrigin(p, font_size, document_font_size, dp_ratio, pixels_per_inch);
		else if (name == TRANSFORM_ORIGIN_Z)
			values.transform_origin_z = ComputeLength(p, font_size, document_font_size, dp_ratio, pixels_per_inch);

		else if (name == TRANSITION)
			values.transition = p->Get<TransitionList>();
		else if (name == ANIMATION)
			values.animation = p->Get<AnimationList>();

	}




}

}
}
