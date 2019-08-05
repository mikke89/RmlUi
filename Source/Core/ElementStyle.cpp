/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "ElementBackground.h"
#include "ElementBorder.h"
#include "ElementDecoration.h"
#include "ElementDefinition.h"
#include "FontFaceHandle.h"
#include "ComputeProperty.h"
#include "DirtyPropertyList.h"
#include "PropertiesIterator.h"


namespace Rml {
namespace Core {

ElementStyle::ElementStyle(Element* _element) : dirty_properties(true)
{
	definition = nullptr;
	element = _element;

	definition_dirty = true;
}

const ElementDefinition* ElementStyle::GetDefinition() const
{
	return definition.get();
}

// Returns one of this element's properties.
const Property* ElementStyle::GetLocalProperty(PropertyId id, const PropertyDictionary& inline_properties, const ElementDefinition* definition)
{
	// Check for overriding local properties.
	const Property* property = inline_properties.GetProperty(id);
	if (property)
		return property;

	// Check for a property defined in an RCSS rule.
	if (definition)
		return definition->GetProperty(id);

	return nullptr;
}

// Returns one of this element's properties.
const Property* ElementStyle::GetProperty(PropertyId id, const Element* element, const PropertyDictionary& inline_properties, const ElementDefinition* definition)
{
	const Property* local_property = GetLocalProperty(id, inline_properties, definition);
	if (local_property)
		return local_property;

	// Fetch the property specification.
	const PropertyDefinition* property = StyleSheetSpecification::GetProperty(id);
	if (!property)
		return nullptr;

	// If we can inherit this property, return our parent's property.
	if (property->IsInherited())
	{
		Element* parent = element->GetParentNode();
		while (parent)
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
void ElementStyle::TransitionPropertyChanges(Element* element, PropertyNameList& properties, const PropertyDictionary& inline_properties, const ElementDefinition* old_definition, const ElementDefinition* new_definition)
{
	RMLUI_ASSERT(element);
	if (!old_definition || !new_definition || properties.empty())
		return;

	// We get the local property instead of the computed value here, because we want to intercept property changes even before the computed values are ready.
	// Now that we have the concept of computed values, we may want do this operation directly on them instead.
	if (const Property* transition_property = GetLocalProperty(PropertyId::Transition, inline_properties, new_definition))
	{
		auto transition_list = transition_property->Get<TransitionList>();

		if (!transition_list.none)
		{
			static const PropertyDictionary empty_properties;

			auto add_transition = [&](const Transition& transition) {
				bool transition_added = false;
				const Property* start_value = GetProperty(transition.id, element, inline_properties, old_definition);
				const Property* target_value = GetProperty(transition.id, element, empty_properties, new_definition);
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

		SharedPtr<ElementDefinition> new_definition;
		
		if (auto& style_sheet = element->GetStyleSheet())
		{
			new_definition = style_sheet->GetElementDefinition(element);
		}
		
		// Switch the property definitions if the definition has changed.
		if (!definition && new_definition)
		{
			// Since we had no definition before there is a likelihood that everything is dirty.
			// We could do as in the next else-if block, but this is considerably faster.
			dirty_properties.DirtyAll();
			definition = new_definition;
		}
		else if (new_definition != definition)
		{
			PropertyNameList properties;
			
			if (definition)
				definition->GetDefinedProperties(properties);

			if (new_definition)
				new_definition->GetDefinedProperties(properties);

			TransitionPropertyChanges(element, properties, inline_properties, definition.get(), new_definition.get());

			definition = new_definition;
			
			DirtyProperties(properties);
		}

		// Even if the definition was not changed, the child definitions may have changed as a result of anything that
		// could change the definition of this element, such as a new pseudo class.
		DirtyChildDefinitions();
	}
}



// Sets or removes a pseudo-class on the element.
void ElementStyle::SetPseudoClass(const String& pseudo_class, bool activate)
{

	bool changed = false;

	if (activate)
		changed = pseudo_classes.insert(pseudo_class).second;
	else
		changed = (pseudo_classes.erase(pseudo_class) == 1);

	if (changed)
	{
		DirtyDefinition();
	}
}

// Checks if a specific pseudo-class has been set on the element.
bool ElementStyle::IsPseudoClassSet(const String& pseudo_class) const
{
	return (pseudo_classes.count(pseudo_class) == 1);
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
	if (!new_property.definition)
		return false;

	inline_properties.SetProperty(id, new_property);
	DirtyProperty(id);

	return true;
}

// Removes a local property override on the element.
void ElementStyle::RemoveProperty(PropertyId id)
{
	int size_before = inline_properties.GetNumProperties();
	inline_properties.RemoveProperty(id);

	if(inline_properties.GetNumProperties() != size_before)
		DirtyProperty(id);
}



// Returns one of this element's properties.
const Property* ElementStyle::GetProperty(PropertyId id) const
{
	return GetProperty(id, element, inline_properties, definition.get());
}

// Returns one of this element's properties.
const Property* ElementStyle::GetLocalProperty(PropertyId id) const
{
	return GetLocalProperty(id, inline_properties, definition.get());
}

const PropertyMap& ElementStyle::GetLocalStyleProperties() const
{
	return inline_properties.GetProperties();
}

float ElementStyle::ResolveNumberLengthPercentage(const Property * property, RelativeTarget relative_target) const
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
float ElementStyle::ResolveLengthPercentage(const Property* property, float base_value) const
{
	if (!property)
	{
		RMLUI_ERROR;
		return 0.0f;
	}
	RMLUI_ASSERT(property->unit & Property::LENGTH_PERCENT);

	const float font_size = element->GetComputedValues().font_size;
	const float doc_font_size = element->GetOwnerDocument()->GetComputedValues().font_size;
	const float dp_ratio = ElementUtilities::GetDensityIndependentPixelRatio(element);

	Style::LengthPercentage computed = ComputeLengthPercentage(property, font_size, doc_font_size, dp_ratio);

	float result = ResolveValue(computed, base_value);

	return result;
}

void ElementStyle::DirtyDefinition()
{
	definition_dirty = true;
}

void ElementStyle::DirtyChildDefinitions()
{
	for (int i = 0; i < element->GetNumChildren(true); i++)
		element->GetChild(i)->GetStyle()->DirtyDefinition();
}

void ElementStyle::DirtyPropertiesWithUnitRecursive(Property::Unit unit)
{
	// Dirty all the properties of this element that use the unit.
	for (auto it = Iterate(); !it.AtEnd(); ++it)
	{
		auto name_property_pair = *it;
		PropertyId id = name_property_pair.first;
		const Property& property = name_property_pair.second;
		if (property.unit == unit)
			DirtyProperty(id);
	}

	// Now dirty all of our descendant's properties that use the unit.
	int num_children = element->GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		element->GetChild(i)->GetStyle()->DirtyPropertiesWithUnitRecursive(unit);
}

bool ElementStyle::AnyPropertiesDirty() const 
{
	return !dirty_properties.Empty(); 
}

PropertiesIterator ElementStyle::Iterate() const {
	// Note: Value initialized iterators are only guaranteed to compare equal in C++14, and only for iterators satisfying the ForwardIterator requirements.
#ifdef _MSC_VER
	// Null forward iterator supported since VS 2015
	static_assert(_MSC_VER >= 1900, "Visual Studio 2015 or higher required, see comment.");
#else
	static_assert(__cplusplus >= 201402L, "C++14 or higher required, see comment.");
#endif

	const PropertyMap& property_map = inline_properties.GetProperties();
	auto it_style_begin = property_map.begin();
	auto it_style_end = property_map.end();

	PropertyMap::const_iterator it_definition{}, it_definition_end{};
	if (definition)
	{
		const PropertyMap& definition_properties = definition->GetProperties().GetProperties();
		it_definition = definition_properties.begin();
		it_definition_end = definition_properties.end();
	}
	return PropertiesIterator(it_style_begin, it_style_end, it_definition, it_definition_end);
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
	if (dirty_properties.Empty())
		return DirtyPropertyList();

	// Generally, this is how it works:
	//   1. Assign default values (clears any removed properties)
	//   2. Inherit inheritable values from parent
	//   3. Assign any local properties (from inline style or stylesheet)

	const float font_size_before = values.font_size;
	const Style::LineHeight line_height_before = values.line_height;

	// The next flag is just a small optimization, if the element was just created we don't need to copy all the default values.
	if (!values_are_default_initialized)
	{
		// This needs to be done in case some properties were removed and thus not in our local style anymore.
		// If we skipped this, the old dirty value would be unmodified, instead, now it is set to its default value.
		// Strictly speaking, we only really need to do this for the dirty values, and only non-inherited. However,
		// it seems assigning the whole thing is faster in most cases.
		values = DefaultComputedValues;
	}

	// Always do font-size first if dirty, because of em-relative values
	if(dirty_properties.Contains(PropertyId::FontSize))
	{
		if (auto p = GetLocalProperty(PropertyId::FontSize))
			values.font_size = ComputeFontsize(*p, values, parent_values, document_values, dp_ratio);
		else if (parent_values)
			values.font_size = parent_values->font_size;
		
		if (font_size_before != values.font_size)
			DirtyEmProperties(dirty_properties, element);
	}
	else
	{
		values.font_size = font_size_before;
	}

	const float font_size = values.font_size;
	const float document_font_size = (document_values ? document_values->font_size : DefaultComputedValues.font_size);


	// Since vertical-align depends on line-height we compute this before iteration
	if(dirty_properties.Contains(PropertyId::LineHeight))
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

		if(line_height_before.value != values.line_height.value || line_height_before.inherit_value != values.line_height.inherit_value)
			dirty_properties.Insert(PropertyId::VerticalAlign);
	}
	else
	{
		values.line_height = line_height_before;
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
		
		values.font_effect = parent_values->font_effect;
	}


	for (auto it = Iterate(); !it.AtEnd(); ++it)
	{
		auto name_property_pair = *it;
		const PropertyId id = name_property_pair.first;
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
			values.transform = p->Get<TransformPtr>();
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
			if (p->unit == Property::DECORATOR)
			{
				values.decorator = p->Get<DecoratorList>();
			}
			else if (p->unit == Property::STRING)
			{
				// Usually the decorator is converted from string after the style sheet is set on the ElementDocument. However, if the
				// user sets a decorator on the element's style, we may still get a string here which must be parsed and instanced.
				if(auto& style_sheet = element->GetStyleSheet())
				{
					String value = p->Get<String>();
					values.decorator = style_sheet->InstanceDecoratorsFromString(value, p->source, p->source_line_number);
				}
			}
			break;
		case PropertyId::FontEffect:
			values.font_effect.reset();
			if (p->unit == Property::FONTEFFECT)
			{
				values.font_effect = p->Get<FontEffectListPtr>();
			}
			else if (p->unit == Property::STRING)
			{
				if (auto & style_sheet = element->GetStyleSheet())
				{
					String value = p->Get<String>();
					values.font_effect = style_sheet->InstanceFontEffectsFromString(value, p->source, p->source_line_number);
				}
			}
			break;

		default:
			break;
		}
	}


	// Next, pass inheritable dirty properties onto our children
	// @performance: We might avoid an allocation here in case of dirty non-inherited custom properties. Instead of the initial copy and &=, introduce & operator.
	DirtyPropertyList dirty_inherited_properties = dirty_properties;
	dirty_inherited_properties &= StyleSheetSpecification::GetRegisteredInheritedPropertyBitList();

	if (!dirty_inherited_properties.Empty())
	{
		for (int i = 0; i < element->GetNumChildren(true); i++)
		{
			auto child = element->GetChild(i);
			child->GetStyle()->dirty_properties |= dirty_inherited_properties;
		}
	}
	
	DirtyPropertyList result(std::move(dirty_properties));
	dirty_properties.Clear();
	return result;
}

}
}
