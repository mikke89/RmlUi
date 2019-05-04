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
#include "ElementStyleCache.h"
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

ElementStyle::ElementStyle(Element* _element)
{
	local_properties = NULL;
	em_properties = NULL;
	definition = NULL;
	element = _element;
	cache = new ElementStyleCache(this);

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

	delete cache;
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
		return property->value.Get< float >() * ElementUtilities::GetFontSize(element);
	case Property::REM:
		return property->value.Get< float >() * ElementUtilities::GetFontSize(element->GetOwnerDocument());
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
		base_value = (float)ElementUtilities::GetFontSize(element);
		break;
	case RelativeTarget::ParentFontSize:
		base_value = (float)ElementUtilities::GetFontSize(element->GetParentNode());
		break;
	case RelativeTarget::LineHeight:
		base_value = (float)ElementUtilities::GetLineHeight(element);
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
			return property->value.Get< float >() * (float)ElementUtilities::GetFontSize(element);
		case Property::REM:
			return property->value.Get< float >() * (float)ElementUtilities::GetFontSize(element->GetOwnerDocument());
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

			base_value = element->GetOwnerDocument()->ResolveProperty(FONT_SIZE, 0);
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
				return property->value.Get< float >() * ElementUtilities::GetFontSize(element->GetOwnerDocument());
		}
	}

	return ResolveProperty(property, base_value);
}

// Iterates over the properties defined on the element.
bool ElementStyle::IterateProperties(int& index, PseudoClassList& property_pseudo_classes, String& name, const Property*& property)
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
			property_pseudo_classes.clear();
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
		while (definition->IterateProperties(index, pseudo_classes, property_pseudo_classes, name, property))
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

	// Clear all cached properties.
	cache->Clear();
	cache->ClearInherited();

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

	// Clear cached inherited properties.
	cache->ClearInherited();

	// Pass the list of those properties that this element doesn't override onto our children.
	for (int i = 0; i < element->GetNumChildren(true); i++)
		element->GetChild(i)->GetStyle()->DirtyInheritedProperties(inherited_properties);

	element->DirtyProperties(properties);
}

void ElementStyle::GetOffsetProperties(const Property **top, const Property **bottom, const Property **left, const Property **right )
{
	cache->GetOffsetProperties(top, bottom, left, right);
}

void ElementStyle::GetBorderWidthProperties(const Property **border_top_width, const Property **border_bottom_width, const Property **border_left_width, const Property **bottom_right_width)
{
	cache->GetBorderWidthProperties(border_top_width, border_bottom_width, border_left_width, bottom_right_width);
}

void ElementStyle::GetMarginProperties(const Property **margin_top, const Property **margin_bottom, const Property **margin_left, const Property **margin_right)
{
	cache->GetMarginProperties(margin_top, margin_bottom, margin_left, margin_right);
}

void ElementStyle::GetPaddingProperties(const Property **padding_top, const Property **padding_bottom, const Property **padding_left, const Property **padding_right)
{
	cache->GetPaddingProperties(padding_top, padding_bottom, padding_left, padding_right);
}

void ElementStyle::GetDimensionProperties(const Property **width, const Property **height)
{
	cache->GetDimensionProperties(width, height);
}

void ElementStyle::GetLocalDimensionProperties(const Property **width, const Property **height)
{
	cache->GetLocalDimensionProperties(width, height);
}

void ElementStyle::GetOverflow(int *overflow_x, int *overflow_y)
{
	cache->GetOverflow(overflow_x, overflow_y);
}

int ElementStyle::GetPosition()
{
	return cache->GetPosition();
}

int ElementStyle::GetFloat()
{
	return cache->GetFloat();
}

int ElementStyle::GetDisplay()
{
	return cache->GetDisplay();
}

int ElementStyle::GetWhitespace()
{
	return cache->GetWhitespace();
}

int ElementStyle::GetPointerEvents()
{
	return cache->GetPointerEvents();
}

const Property *ElementStyle::GetLineHeightProperty()
{
	return cache->GetLineHeightProperty();
}

int ElementStyle::GetTextAlign()
{
	return cache->GetTextAlign();
}

int ElementStyle::GetTextTransform()
{
	return cache->GetTextTransform();
}

const Property *ElementStyle::GetVerticalAlignProperty()
{
	return cache->GetVerticalAlignProperty();
}

// Returns 'perspective' property value from element's style or local cache.
const Property *ElementStyle::GetPerspective()
{
	return element->GetProperty(PERSPECTIVE);
}

// Returns 'perspective-origin-x' property value from element's style or local cache.
const Property *ElementStyle::GetPerspectiveOriginX()
{
	return element->GetProperty(PERSPECTIVE_ORIGIN_X);
}

// Returns 'perspective-origin-y' property value from element's style or local cache.
const Property *ElementStyle::GetPerspectiveOriginY()
{
	return element->GetProperty(PERSPECTIVE_ORIGIN_Y);
}

// Returns 'transform' property value from element's style or local cache.
const Property *ElementStyle::GetTransform()
{
	return element->GetProperty(TRANSFORM);
}

// Returns 'transform-origin-x' property value from element's style or local cache.
const Property *ElementStyle::GetTransformOriginX()
{
	return element->GetProperty(TRANSFORM_ORIGIN_X);
}

// Returns 'transform-origin-y' property value from element's style or local cache.
const Property *ElementStyle::GetTransformOriginY()
{
	return element->GetProperty(TRANSFORM_ORIGIN_Y);
}

// Returns 'transform-origin-z' property value from element's style or local cache.
const Property *ElementStyle::GetTransformOriginZ()
{
	return element->GetProperty(TRANSFORM_ORIGIN_Z);
}

}
}
