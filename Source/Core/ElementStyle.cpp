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
#include "ComputeProperty.h"


namespace Rocket {
namespace Core {


ElementStyle::ElementStyle(Element* _element) : dirty_properties(true)
{
	local_properties = NULL;
	definition = NULL;
	element = _element;

	definition_dirty = true;
}

ElementStyle::~ElementStyle()
{
	if (local_properties != NULL)
		delete local_properties;

	if (definition != NULL)
		definition->RemoveReference();
}


// Returns the element's definition, updating if necessary.
const ElementDefinition* ElementStyle::GetDefinition()
{
	return definition;
}



// Returns one of this element's properties.
const Property* ElementStyle::GetLocalProperty(PropertyId id, PropertyDictionary* local_properties, ElementDefinition* definition, const PseudoClassList& pseudo_classes)
{
	// Check for overriding local properties.
	if (local_properties != NULL)
	{
		const Property* property = local_properties->GetProperty(id);
		if (property != NULL)
			return property;
	}

	// Check for a property defined in an RCSS rule.
	if (definition != NULL)
		return definition->GetProperty(id, pseudo_classes);

	return NULL;
}

// Returns one of this element's properties.
const Property* ElementStyle::GetProperty(PropertyId id, Element* element, PropertyDictionary* local_properties, ElementDefinition* definition, const PseudoClassList& pseudo_classes)
{
	const Property* local_property = GetLocalProperty(id, local_properties, definition, pseudo_classes);
	if (local_property != NULL)
		return local_property;

	// Fetch the property specification.
	const PropertyDefinition* property = StyleSheetSpecification::GetProperty(id);
	if (property == NULL)
		return NULL;

	// If we can inherit this property, return our parent's property.
	if (property->IsInherited())
	{
		Element* parent = element->GetParentNode();
		while (parent != NULL)
		{
			const Property* parent_property = parent->GetStyle()->GetLocalProperty(id);
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

	if (const Property* transition_property = GetLocalProperty(PropertyId::Transition, local_properties, new_definition, pseudo_classes_after))
	{
		auto transition_list = transition_property->Get<TransitionList>();

		if (!transition_list.none)
		{
			auto add_transition = [&](const Transition& transition) {
				bool transition_added = false;
				const Property* start_value = GetProperty(transition.id, element, local_properties, old_definition, pseudo_classes_before);
				const Property* target_value = GetProperty(transition.id, element, nullptr, new_definition, pseudo_classes_after);
				if (start_value && target_value && (*start_value != *target_value))
					transition_added = element->StartTransition(transition, *start_value, *target_value);
				return transition_added;
			};

			if (transition_list.all)
			{
				Transition transition = transition_list.transitions[0];
				for (auto it = properties.begin(); it != properties.end(); )
				{
					transition.id = *it;
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
					auto it = properties.find(transition.id);
					if (it != properties.end())
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
		
		ElementDefinition* new_definition = nullptr;
		
		const StyleSheet* style_sheet = GetStyleSheet();
		if (style_sheet)
		{
			new_definition = style_sheet->GetElementDefinition(element);
		}
		
		// Switch the property definitions if the definition has changed.
		if (!definition && new_definition)
		{
			// Since we had no definition before there is a likelihood that everything is dirty.
			// We could do as in the next else-if block, but this is considerably faster.
			dirty_properties.DirtyAll();
			element->GetElementDecoration()->DirtyDecorators();
			definition = new_definition;
		}
		else if (new_definition != definition)
		{
			PropertyNameList properties;
			
			if (definition)
				definition->GetDefinedProperties(properties, pseudo_classes);

			if (new_definition)
				new_definition->GetDefinedProperties(properties, pseudo_classes);

			TransitionPropertyChanges(element, properties, local_properties, definition, new_definition, pseudo_classes, pseudo_classes);

			if (definition)
				definition->RemoveReference();

			definition = new_definition;
			
			DirtyProperties(properties);
			element->GetElementDecoration()->DirtyDecorators();
		}
		else if (!new_definition)
		{
			// Both definitions empty
			ROCKET_ASSERT(!definition);
			// Is this really necessary?
			element->GetElementDecoration()->DirtyDecorators();
		}
		else if (new_definition)
		{
			// We got the same definition
			new_definition->RemoveReference();
		}
	}
}



// Sets or removes a pseudo-class on the element.
void ElementStyle::SetPseudoClass(const String& pseudo_class, bool activate)
{
	PseudoClassList pseudo_classes_before = pseudo_classes;

	bool changed = false;

	if (activate)
		changed = pseudo_classes.insert(pseudo_class).second;
	else
		changed = (pseudo_classes.erase(pseudo_class) == 1);

	if (changed)
	{
		element->GetElementDecoration()->DirtyDecorators();

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

// Sets a local property override on the element to a pre-parsed value.
bool ElementStyle::SetProperty(PropertyId id, const Property& property)
{
	Property new_property = property;

	new_property.definition = StyleSheetSpecification::GetProperty(id);
	if (new_property.definition == NULL)
		return false;

	if (local_properties == NULL)
		local_properties = new PropertyDictionary();

	local_properties->SetProperty(id, new_property);
	DirtyProperty(id);

	return true;
}

// Removes a local property override on the element.
void ElementStyle::RemoveProperty(PropertyId id)
{
	if (local_properties == NULL)
		return;

	if (local_properties->GetProperty(id) != NULL)
	{
		local_properties->RemoveProperty(id);
		DirtyProperty(id);
	}
}



// Returns one of this element's properties.
const Property* ElementStyle::GetProperty(PropertyId id)
{
	return GetProperty(id, element, local_properties, definition, pseudo_classes);
}

// Returns one of this element's properties.
const Property* ElementStyle::GetLocalProperty(PropertyId id)
{
	return GetLocalProperty(id, local_properties, definition, pseudo_classes);
}

const PropertyMap * ElementStyle::GetLocalProperties() const
{
	if (local_properties)
		return &local_properties->GetProperties();
	return NULL;
}

float ElementStyle::ResolveNumberLengthPercentage(const Property * property, RelativeTarget relative_target)
{
	// There is an exception on font-size properties, as 'em' units here refer to parent font size instead
	if ((property->unit & Property::LENGTH) && !(property->unit == Property::EM && relative_target == RelativeTarget::ParentFontSize))
	{
		float result = ComputeLength(property, element->GetComputedValues().font_size, element->GetOwnerDocument()->GetComputedValues().font_size, ElementUtilities::GetDensityIndependentPixelRatio(element));
		return result;
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
	default:
		break;
	}

	return base_value * scale_value;
}

// Resolves one of this element's properties.
float ElementStyle::ResolveLengthPercentage(const Property* property, float base_value)
{
	if (!property)
	{
		ROCKET_ERROR;
		return 0.0f;
	}
	ROCKET_ASSERT(property->unit & Property::LENGTH_PERCENT);

	const float font_size = element->GetComputedValues().font_size;
	const float doc_font_size = element->GetOwnerDocument()->GetComputedValues().font_size;
	const float dp_ratio = ElementUtilities::GetDensityIndependentPixelRatio(element);

	Style::LengthPercentage computed = ComputeLengthPercentage(property, font_size, doc_font_size, dp_ratio);

	float result = ResolveValue(computed, base_value);

	return result;
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

// Dirties rem properties.
void ElementStyle::DirtyRemProperties()
{
	const PropertyNameList &properties = StyleSheetSpecification::GetRegisteredProperties();
	PropertyNameList rem_properties;

	// Dirty all the properties of this element that use the rem unit.
	for (auto name_property_pair : *this)
	{
		PropertyId id = name_property_pair.first;
		const Property& property = name_property_pair.second;
		if (property.unit == Property::REM)
			rem_properties.insert(id);
	}

	if (!rem_properties.empty())
		DirtyProperties(rem_properties);

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
	for (auto name_property_pair : *this)
	{
		PropertyId id = name_property_pair.first;
		const Property& property = name_property_pair.second;
		if (property.unit == Property::DP)
			dp_properties.insert(id);
	}

	if (!dp_properties.empty())
		DirtyProperties(dp_properties);

	// Now dirty all of our descendant's properties that use the dp unit.
	int num_children = element->GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		element->GetChild(i)->GetStyle()->DirtyDpProperties();
}

bool ElementStyle::AnyPropertiesDirty() const 
{
	return !dirty_properties.Empty(); 
}

ElementStyleIterator ElementStyle::begin() const {
	const PropertyMap* local = nullptr;
	PropertyMap::const_iterator it_local_begin, it_local_end;
	if (local_properties)
	{
		local = &local_properties->GetProperties();
		it_local_begin = local->begin();
		it_local_end = local->end();
	}
	ElementDefinitionIterator it_definition_begin, it_definition_end;
	if (definition)
	{
		it_definition_begin = definition->begin(pseudo_classes);
		it_definition_end = definition->end(pseudo_classes);
	}
	return ElementStyleIterator(local, it_local_begin, it_definition_begin, it_local_end, it_definition_end);
}

ElementStyleIterator ElementStyle::end() const {
	const PropertyMap* local = nullptr;
	PropertyMap::const_iterator it_local_end;
	if (local_properties)
	{
		local = &local_properties->GetProperties();
		it_local_end = local->end();
	}
	ElementDefinitionIterator it_definition_end;
	if (definition)
	{
		it_definition_end = definition->end(pseudo_classes);
	}
	return ElementStyleIterator(local, it_local_end, it_definition_end, it_local_end, it_definition_end);
}

// Sets a single property as dirty.
void ElementStyle::DirtyProperty(PropertyId id)
{
	dirty_properties.Insert(id);
}

// Sets a list of properties as dirty.
void ElementStyle::DirtyProperties(const PropertyNameList& properties)
{
	dirty_properties.Insert(properties);
}

// Sets a list of our potentially inherited properties as dirtied by an ancestor.
void ElementStyle::DirtyInheritedProperties(const PropertyNameList& properties)
{
	dirty_properties.Insert(properties);
	return;
}

static void DirtyEmProperties(DirtyPropertyList& dirty_properties, Element* element)
{
	// Either we can dirty every property, or we can iterate over all properties and see if anyone uses em-units.
	// Choose whichever is fastest based on benchmarking.
#if 1
	// Dirty every property
	dirty_properties.DirtyAll();
#else
	if (dirty_properties.AllDirty())
		return;

	// Check if any of these are currently em-relative. If so, dirty them.
	for (auto& property_name : StyleSheetSpecification::GetRegisteredProperties())
	{
		// Skip font-size; this is relative to our parent's em, not ours.
		if (property_name == FONT_SIZE)
			continue;

		// Get this property from this element. If this is em-relative, then add it to the list to
		// dirty.
		if (element->GetProperty(property_name)->unit == Property::EM)
			dirty_properties.Insert(property_name);
	}
#endif
}


DirtyPropertyList ElementStyle::ComputeValues(Style::ComputedValues& values, const Style::ComputedValues* parent_values, const Style::ComputedValues* document_values, bool values_are_default_initialized, float dp_ratio)
{
	ROCKET_ASSERT_NONRECURSIVE;

	if (dirty_properties.Empty())
		return DirtyPropertyList();

	// Generally, this is how it works (for now, we can probably be smarter about this):
	//   1. Assign default values (clears any newly dirtied properties)
	//   2. Inherit inheritable values from parent
	//   3. Assign any local properties (from inline style or stylesheet)

	const float font_size_before = values.font_size;
	const float line_height_before = values.line_height.value;

	// The next flag is just a small optimization, if the element was just created we don't need to copy all the default values.
	if (!values_are_default_initialized)
	{
		values = DefaultComputedValues;
	}

	// Always do font-size first if dirty, because of em-relative values
	{
		if (auto p = GetLocalProperty(PropertyId::FontSize))
			values.font_size = ComputeFontsize(*p, values, parent_values, document_values, dp_ratio);
		else if (parent_values)
			values.font_size = parent_values->font_size;
		
		if (font_size_before != values.font_size)
			DirtyEmProperties(dirty_properties, element);
	}

	const float font_size = values.font_size;
	const float document_font_size = (document_values ? document_values->font_size : DefaultComputedValues.font_size);


	// Since vertical-align depends on line-height we compute this before iteration
	{
		if (auto p = GetLocalProperty(PropertyId::LineHeight))
		{
			values.line_height = ComputeLineHeight(p, font_size, document_font_size, dp_ratio);
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

		if(line_height_before != values.line_height.value)
			dirty_properties.Insert(PropertyId::VerticalAlign);
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


	for(auto name_property_pair : *this)
	{
		PropertyId id = name_property_pair.first;
		const Property* p = &name_property_pair.second;

		using namespace Style;

		switch (id)
		{
		case PropertyId::MarginTop:
			values.margin_top = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::MarginRight:
			values.margin_right = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::MarginBottom:
			values.margin_bottom = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::MarginLeft:
			values.margin_left = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::PaddingTop:
			values.padding_top = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::PaddingRight:
			values.padding_right = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::PaddingBottom:
			values.padding_bottom = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::PaddingLeft:
			values.padding_left = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::BorderTopWidth:
			values.border_top_width = ComputeLength(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::BorderRightWidth:
			values.border_right_width = ComputeLength(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::BorderBottomWidth:
			values.border_bottom_width = ComputeLength(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::BorderLeftWidth:
			values.border_left_width = ComputeLength(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::BorderTopColor:
			values.border_top_color = p->Get<Colourb>();
			break;
		case PropertyId::BorderRightColor:
			values.border_right_color = p->Get<Colourb>();
			break;
		case PropertyId::BorderBottomColor:
			values.border_bottom_color = p->Get<Colourb>();
			break;
		case PropertyId::BorderLeftColor:
			values.border_left_color = p->Get<Colourb>();
			break;

		case PropertyId::Display:
			values.display = (Display)p->Get<int>();
			break;
		case PropertyId::Position:
			values.position = (Position)p->Get<int>();
			break;

		case PropertyId::Top:
			values.top = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::Right:
			values.right = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::Bottom:
			values.bottom = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::Left:
			values.left = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::Float:
			values.float_ = (Float)p->Get<int>();
			break;
		case PropertyId::Clear:
			values.clear = (Clear)p->Get<int>();
			break;

		case PropertyId::ZIndex:
			values.z_index = (p->unit == Property::KEYWORD ? ZIndex(ZIndex::Auto) : ZIndex(ZIndex::Number, p->Get<float>()));
			break;

		case PropertyId::Width:
			values.width = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::MinWidth:
			values.min_width = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::MaxWidth:
			values.max_width = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::Height:
			values.height = ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::MinHeight:
			values.min_height = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::MaxHeight:
			values.max_height = ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::LineHeight:
			// (Line-height computed above)
			break;
		case PropertyId::VerticalAlign:
			values.vertical_align = ComputeVerticalAlign(p, values.line_height.value, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::OverflowX:
			values.overflow_x = (Overflow)p->Get< int >();
			break;
		case PropertyId::OverflowY:
			values.overflow_y = (Overflow)p->Get< int >();
			break;
		case PropertyId::Clip:
			values.clip = ComputeClip(p);
			break;
		case PropertyId::Visibility:
			values.visibility = (Visibility)p->Get< int >();
			break;

		case PropertyId::BackgroundColor:
			values.background_color = p->Get<Colourb>();
			break;
		case PropertyId::Color:
			values.color = p->Get<Colourb>();
			break;
		case PropertyId::ImageColor:
			values.image_color = p->Get<Colourb>();
			break;
		case PropertyId::Opacity:
			values.opacity = p->Get<float>();
			break;

		case PropertyId::FontFamily:
			values.font_family = ToLower(p->Get<String>());
			break;
		case PropertyId::FontCharset:
			values.font_charset = p->Get<String>();
			break;
		case PropertyId::FontStyle:
			values.font_style = (FontStyle)p->Get< int >();
			break;
		case PropertyId::FontWeight:
			values.font_weight = (FontWeight)p->Get< int >();
			break;
		case PropertyId::FontSize:
			// (font-size computed above)
			break;

		case PropertyId::TextAlign:
			values.text_align = (TextAlign)p->Get< int >();
			break;
		case PropertyId::TextDecoration:
			values.text_decoration = (TextDecoration)p->Get< int >();
			break;
		case PropertyId::TextTransform:
			values.text_transform = (TextTransform)p->Get< int >();
			break;
		case PropertyId::WhiteSpace:
			values.white_space = (WhiteSpace)p->Get< int >();
			break;

		case PropertyId::Cursor:
			values.cursor = p->Get< String >();
			break;

		case PropertyId::Drag:
			values.drag = (Drag)p->Get< int >();
			break;
		case PropertyId::TabIndex:
			values.tab_index = (TabIndex)p->Get< int >();
			break;
		case PropertyId::Focus:
			values.focus = (Focus)p->Get<int>();
			break;
		case PropertyId::ScrollbarMargin:
			values.scrollbar_margin = ComputeLength(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::PointerEvents:
			values.pointer_events = (PointerEvents)p->Get<int>();
			break;

		case PropertyId::Perspective:
			values.perspective = ComputeLength(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::PerspectiveOriginX:
			values.perspective_origin_x = ComputeOrigin(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::PerspectiveOriginY:
			values.perspective_origin_y = ComputeOrigin(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::Transform:
			values.transform = p->Get<TransformRef>();
			break;
		case PropertyId::TransformOriginX:
			values.transform_origin_x = ComputeOrigin(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::TransformOriginY:
			values.transform_origin_y = ComputeOrigin(p, font_size, document_font_size, dp_ratio);
			break;
		case PropertyId::TransformOriginZ:
			values.transform_origin_z = ComputeLength(p, font_size, document_font_size, dp_ratio);
			break;

		case PropertyId::Transition:
			values.transition = p->Get<TransitionList>();
			break;
		case PropertyId::Animation:
			values.animation = p->Get<AnimationList>();
			break;

		case PropertyId::Decorator:
			values.decorator.clear();
			// Usually the decorator is converted from string after the style sheet is set on the ElementDocument. However, if the
			// user sets a decorator on the element's style, we may still get a string here which must be parsed and instanced.
			if (p->unit == Property::DECORATOR)
			{
				values.decorator = p->Get<DecoratorList>();
			}
			else if (p->unit == Property::STRING)
			{
				if(const StyleSheet* style_sheet = GetStyleSheet())
				{
					String value = p->Get<String>();
					values.decorator = style_sheet->InstanceDecoratorsFromString(value, p->source, p->source_line_number);
				}
			}
			break;

		case PropertyId::FontEffect:
			break;
		}
	}


	// Next, pass inheritable dirty properties onto our children

	// Static to avoid repeated allocations, requires non-recursion.
	static PropertyNameList dirty_inherited;
	dirty_inherited.clear();

	bool all_inherited_dirty = false;

	if (dirty_properties.AllDirty())
	{
		all_inherited_dirty = true;
	}
	else
	{
		// Find all dirtied properties which are also inherited
		const auto& inherited_properties = StyleSheetSpecification::GetRegisteredInheritedProperties();
		std::set_intersection(
			inherited_properties.begin(), inherited_properties.end(), 
			dirty_properties.GetList().begin(), dirty_properties.GetList().end(), 
			std::back_inserter(dirty_inherited.modify_container())
		);
	}

	if (all_inherited_dirty || dirty_inherited.size() > 0)
	{
		// Dirty inherited properties in our children
		const auto& dirty_inherited_ref = (all_inherited_dirty ? StyleSheetSpecification::GetRegisteredInheritedProperties() : dirty_inherited);
		for (int i = 0; i < element->GetNumChildren(true); i++)
		{
			auto child = element->GetChild(i);
			child->GetStyle()->dirty_properties.Insert(dirty_inherited_ref);
		}
	}

	DirtyPropertyList result(std::move(dirty_properties));
	dirty_properties.Clear();
	return result;
}

}
}
