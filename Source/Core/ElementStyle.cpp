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

#include "ElementStyle.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyDictionary.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/TransformPrimitive.h"
#include "ComputeProperty.h"
#include "ElementDefinition.h"
#include "PropertiesIterator.h"
#include <algorithm>

namespace Rml {

inline PseudoClassState operator|(PseudoClassState lhs, PseudoClassState rhs)
{
	return PseudoClassState(int(lhs) | int(rhs));
}
inline PseudoClassState operator&(PseudoClassState lhs, PseudoClassState rhs)
{
	return PseudoClassState(int(lhs) & int(rhs));
}

ElementStyle::ElementStyle(Element* _element)
{
	element = _element;
}

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

const Property* ElementStyle::GetProperty(PropertyId id, const Element* element, const PropertyDictionary& inline_properties,
	const ElementDefinition* definition)
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

void ElementStyle::TransitionPropertyChanges(Element* element, PropertyIdSet& properties, const PropertyDictionary& inline_properties,
	const ElementDefinition* old_definition, const ElementDefinition* new_definition)
{
	// Apply transition to relevant properties if a transition is defined on element.
	// Properties that are part of a transition are removed from the properties list.

	RMLUI_ASSERT(element);
	if (!old_definition || !new_definition || properties.Empty())
		return;

	// We get the local property instead of the computed value here, because we want to intercept property changes even before the computed values are
	// ready. Now that we have the concept of computed values, we may want do this operation directly on them instead.
	if (const Property* transition_property = GetLocalProperty(PropertyId::Transition, inline_properties, new_definition))
	{
		if (transition_property->value.GetType() != Variant::TRANSITIONLIST)
			return;

		const TransitionList& transition_list = transition_property->value.GetReference<TransitionList>();

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
				for (auto it = properties.begin(); it != properties.end();)
				{
					transition.id = *it;
					if (add_transition(transition))
						it = properties.Erase(it);
					else
						++it;
				}
			}
			else
			{
				for (const Transition& transition : transition_list.transitions)
				{
					if (properties.Contains(transition.id))
					{
						if (add_transition(transition))
							properties.Erase(transition.id);
					}
				}
			}
		}
	}
}

void ElementStyle::UpdateDefinition()
{
	RMLUI_ZoneScoped;

	SharedPtr<const ElementDefinition> new_definition;

	if (const StyleSheet* style_sheet = element->GetStyleSheet())
	{
		new_definition = style_sheet->GetElementDefinition(element);
	}

	// Switch the property definitions if the definition has changed.
	if (new_definition != definition)
	{
		PropertyIdSet changed_properties;

		if (definition)
			changed_properties = definition->GetPropertyIds();

		if (new_definition)
			changed_properties |= new_definition->GetPropertyIds();

		if (definition && new_definition)
		{
			// Remove properties that compare equal from the changed list.
			const PropertyIdSet properties_in_both_definitions = (definition->GetPropertyIds() & new_definition->GetPropertyIds());

			for (PropertyId id : properties_in_both_definitions)
			{
				const Property* p0 = definition->GetProperty(id);
				const Property* p1 = new_definition->GetProperty(id);
				if (p0 && p1 && *p0 == *p1)
					changed_properties.Erase(id);
			}

			// Transition changed properties if transition property is set
			TransitionPropertyChanges(element, changed_properties, inline_properties, definition.get(), new_definition.get());
		}

		definition = new_definition;

		DirtyProperties(changed_properties);
	}
}

bool ElementStyle::SetPseudoClass(const String& pseudo_class, bool activate, bool override_class)
{
	bool changed = false;

	if (activate)
	{
		PseudoClassState& state = pseudo_classes[pseudo_class];
		changed = (state == PseudoClassState::Clear);
		state = (state | (override_class ? PseudoClassState::Override : PseudoClassState::Set));
	}
	else
	{
		auto it = pseudo_classes.find(pseudo_class);
		if (it != pseudo_classes.end())
		{
			PseudoClassState& state = it->second;
			state = (state & (override_class ? PseudoClassState::Set : PseudoClassState::Override));
			if (state == PseudoClassState::Clear)
			{
				pseudo_classes.erase(it);
				changed = true;
			}
		}
	}

	return changed;
}

bool ElementStyle::IsPseudoClassSet(const String& pseudo_class) const
{
	return (pseudo_classes.count(pseudo_class) == 1);
}

const PseudoClassMap& ElementStyle::GetActivePseudoClasses() const
{
	return pseudo_classes;
}

bool ElementStyle::SetClass(const String& class_name, bool activate)
{
	const auto class_location = std::find(classes.begin(), classes.end(), class_name);

	bool changed = false;
	if (activate)
	{
		if (class_location == classes.end())
		{
			classes.push_back(class_name);
			changed = true;
		}
	}
	else
	{
		if (class_location != classes.end())
		{
			classes.erase(class_location);
			changed = true;
		}
	}

	return changed;
}

bool ElementStyle::IsClassSet(const String& class_name) const
{
	return std::find(classes.begin(), classes.end(), class_name) != classes.end();
}

void ElementStyle::SetClassNames(const String& class_names)
{
	classes.clear();
	StringUtilities::ExpandString(classes, class_names, ' ');
}

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

const StringList& ElementStyle::GetClassNameList() const
{
	return classes;
}

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

void ElementStyle::RemoveProperty(PropertyId id)
{
	int size_before = inline_properties.GetNumProperties();
	inline_properties.RemoveProperty(id);

	if (inline_properties.GetNumProperties() != size_before)
		DirtyProperty(id);
}

const Property* ElementStyle::GetProperty(PropertyId id) const
{
	return GetProperty(id, element, inline_properties, definition.get());
}

const Property* ElementStyle::GetLocalProperty(PropertyId id) const
{
	return GetLocalProperty(id, inline_properties, definition.get());
}

const PropertyMap& ElementStyle::GetLocalStyleProperties() const
{
	return inline_properties.GetProperties();
}

static float ComputeLength(NumericValue value, Element* element)
{
	float font_size = 0.f;
	float doc_font_size = 0.f;
	float dp_ratio = 1.0f;
	Vector2f vp_dimensions(1.0f);

	if (Any(value.unit & Unit::DP_SCALABLE_LENGTH))
	{
		if (Context* context = element->GetContext())
			dp_ratio = context->GetDensityIndependentPixelRatio();
	}

	switch (value.unit)
	{
	case Unit::EM: font_size = element->GetComputedValues().font_size(); break;
	case Unit::REM:
		if (ElementDocument* document = element->GetOwnerDocument())
			doc_font_size = document->GetComputedValues().font_size();
		else
			doc_font_size = DefaultComputedValues.font_size();
		break;
	case Unit::VW:
	case Unit::VH:
		if (Context* context = element->GetContext())
			vp_dimensions = Vector2f(context->GetDimensions());
		break;
	default: break;
	}

	const float result = ComputeLength(value, font_size, doc_font_size, dp_ratio, vp_dimensions);
	return result;
}

float ElementStyle::ResolveNumericValue(NumericValue value, float base_value) const
{
	if (value.unit == Unit::PX)
		return value.number;
	else if (Any(value.unit & Unit::LENGTH))
		return ComputeLength(value, element);

	switch (value.unit)
	{
	case Unit::NUMBER: return value.number * base_value;
	case Unit::PERCENT: return value.number * base_value * 0.01f;
	case Unit::X: return value.number;
	case Unit::DEG:
	case Unit::RAD: return ComputeAngle(value);
	default: break;
	}

	RMLUI_ERROR;
	return 0.f;
}

float ElementStyle::ResolveRelativeLength(NumericValue value, RelativeTarget relative_target) const
{
	// There is an exception on font-size properties, as 'em' units here refer to parent font size instead
	if (Any(value.unit & Unit::LENGTH) && !(value.unit == Unit::EM && relative_target == RelativeTarget::ParentFontSize))
	{
		const float result = ComputeLength(value, element);
		return result;
	}

	float base_value = 0.0f;

	switch (relative_target)
	{
	case RelativeTarget::None: base_value = 1.0f; break;
	case RelativeTarget::ContainingBlockWidth: base_value = element->GetContainingBlock().x; break;
	case RelativeTarget::ContainingBlockHeight: base_value = element->GetContainingBlock().y; break;
	case RelativeTarget::FontSize: base_value = element->GetComputedValues().font_size(); break;
	case RelativeTarget::ParentFontSize:
	{
		auto p = element->GetParentNode();
		base_value = (p ? p->GetComputedValues().font_size() : DefaultComputedValues.font_size());
	}
	break;
	case RelativeTarget::LineHeight: base_value = element->GetLineHeight(); break;
	}

	float scale_value = 0.0f;

	switch (value.unit)
	{
	case Unit::EM:
	case Unit::NUMBER: scale_value = value.number; break;
	case Unit::PERCENT: scale_value = value.number * 0.01f; break;
	default: break;
	}

	return base_value * scale_value;
}

void ElementStyle::DirtyInheritedProperties()
{
	dirty_properties |= StyleSheetSpecification::GetRegisteredInheritedProperties();
}

void ElementStyle::DirtyPropertiesWithUnits(Units units)
{
	// Dirty all the properties of this element that use the unit(s).
	for (auto it = Iterate(); !it.AtEnd(); ++it)
	{
		auto name_property_pair = *it;
		PropertyId id = name_property_pair.first;
		const Property& property = name_property_pair.second;
		if (Any(property.unit & units))
			DirtyProperty(id);
	}
}

void ElementStyle::DirtyPropertiesWithUnitsRecursive(Units units)
{
	DirtyPropertiesWithUnits(units);

	// Now dirty all of our descendant's properties that use the unit(s).
	int num_children = element->GetNumChildren(true);
	for (int i = 0; i < num_children; ++i)
		element->GetChild(i)->GetStyle()->DirtyPropertiesWithUnitsRecursive(units);
}

bool ElementStyle::AnyPropertiesDirty() const
{
	return !dirty_properties.Empty();
}

PropertiesIterator ElementStyle::Iterate() const
{
	// Note: Value initialized iterators are only guaranteed to compare equal in C++14, and only for iterators
	// satisfying the ForwardIterator requirements.
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

void ElementStyle::DirtyProperty(PropertyId id)
{
	dirty_properties.Insert(id);
}

void ElementStyle::DirtyProperties(const PropertyIdSet& properties)
{
	dirty_properties |= properties;
}

PropertyIdSet ElementStyle::ComputeValues(Style::ComputedValues& values, const Style::ComputedValues* parent_values,
	const Style::ComputedValues* document_values, bool values_are_default_initialized, float dp_ratio, Vector2f vp_dimensions)
{
	if (dirty_properties.Empty())
		return PropertyIdSet();

	RMLUI_ZoneScopedC(0xFF7F50);

	// Generally, this is how it works:
	//   1. Assign default values (clears any removed properties)
	//   2. Inherit inheritable values from parent
	//   3. Assign any local properties (from inline style or stylesheet)
	//   4. Dirty properties in children that are inherited

	const float font_size_before = values.font_size();
	const Style::LineHeight line_height_before = values.line_height();

	// The next flag is just a small optimization, if the element was just created we don't need to copy all the default values.
	if (!values_are_default_initialized)
	{
		// This needs to be done in case some properties were removed and thus not in our local style anymore.
		// If we skipped this, the old dirty value would be unmodified, instead, now it is set to its default value.
		// Strictly speaking, we only really need to do this for the dirty, non-inherited values. However, in most
		// cases it seems simply assigning all non-inherited values is faster than iterating the dirty properties.
		values.CopyNonInherited(DefaultComputedValues);
	}

	if (parent_values)
		values.CopyInherited(*parent_values);
	else if (!values_are_default_initialized)
		values.CopyInherited(DefaultComputedValues);

	bool dirty_em_properties = false;

	// Always do font-size first if dirty, because of em-relative values
	if (dirty_properties.Contains(PropertyId::FontSize))
	{
		if (auto p = GetLocalProperty(PropertyId::FontSize))
			values.font_size(ComputeFontsize(p->GetNumericValue(), values, parent_values, document_values, dp_ratio, vp_dimensions));
		else if (parent_values)
			values.font_size(parent_values->font_size());

		if (font_size_before != values.font_size())
		{
			dirty_em_properties = true;
			dirty_properties.Insert(PropertyId::LineHeight);
		}
	}
	else
	{
		values.font_size(font_size_before);
	}

	const float font_size = values.font_size();
	const float document_font_size = (document_values ? document_values->font_size() : DefaultComputedValues.font_size());

	// Since vertical-align depends on line-height we compute this before iteration
	if (dirty_properties.Contains(PropertyId::LineHeight))
	{
		if (auto p = GetLocalProperty(PropertyId::LineHeight))
		{
			values.line_height(ComputeLineHeight(p, font_size, document_font_size, dp_ratio, vp_dimensions));
		}
		else if (parent_values)
		{
			// Line height has a special inheritance case for numbers/percent: they inherit them directly instead of computed length, but for lengths,
			// they inherit the length. See CSS specs for details. Percent is already converted to number.
			if (parent_values->line_height().inherit_type == Style::LineHeight::Number)
				values.line_height(Style::LineHeight(font_size * parent_values->line_height().inherit_value, Style::LineHeight::Number,
					parent_values->line_height().inherit_value));
			else
				values.line_height(parent_values->line_height());
		}

		if (line_height_before.value != values.line_height().value || line_height_before.inherit_value != values.line_height().inherit_value)
			dirty_properties.Insert(PropertyId::VerticalAlign);
	}
	else
	{
		values.line_height(line_height_before);
	}

	bool dirty_font_face_handle = false;

	for (auto it = Iterate(); !it.AtEnd(); ++it)
	{
		auto name_property_pair = *it;
		const PropertyId id = name_property_pair.first;
		const Property* p = &name_property_pair.second;

		if (dirty_em_properties && p->unit == Unit::EM)
			dirty_properties.Insert(id);

		using namespace Style;

		// clang-format off
		switch (id)
		{
		case PropertyId::MarginTop:
			values.margin_top(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::MarginRight:
			values.margin_right(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::MarginBottom:
			values.margin_bottom(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::MarginLeft:
			values.margin_left(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::PaddingTop:
			values.padding_top(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::PaddingRight:
			values.padding_right(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::PaddingBottom:
			values.padding_bottom(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::PaddingLeft:
			values.padding_left(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::BorderTopWidth:
			values.border_top_width(ComputeBorderWidth(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions)));
			break;
		case PropertyId::BorderRightWidth:
			values.border_right_width(
				ComputeBorderWidth(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions)));
			break;
		case PropertyId::BorderBottomWidth:
			values.border_bottom_width(
				ComputeBorderWidth(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions)));
			break;
		case PropertyId::BorderLeftWidth:
			values.border_left_width(ComputeBorderWidth(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions)));
			break;

		case PropertyId::BorderTopColor:
			values.border_top_color(p->Get<Colourb>());
			break;
		case PropertyId::BorderRightColor:
			values.border_right_color(p->Get<Colourb>());
			break;
		case PropertyId::BorderBottomColor:
			values.border_bottom_color(p->Get<Colourb>());
			break;
		case PropertyId::BorderLeftColor:
			values.border_left_color(p->Get<Colourb>());
			break;

		case PropertyId::BorderTopLeftRadius:
			values.border_top_left_radius(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::BorderTopRightRadius:
			values.border_top_right_radius(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::BorderBottomRightRadius:
			values.border_bottom_right_radius(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::BorderBottomLeftRadius:
			values.border_bottom_left_radius(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::Display:
			values.display((Display)p->Get<int>());
			break;
		case PropertyId::Position:
			values.position((Position)p->Get<int>());
			break;

		case PropertyId::Top:
			values.top(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::Right:
			values.right(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::Bottom:
			values.bottom(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::Left:
			values.left(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::Float:
			values.float_((Float)p->Get<int>());
			break;
		case PropertyId::Clear:
			values.clear((Clear)p->Get<int>());
			break;
		case PropertyId::BoxSizing:
			values.box_sizing((BoxSizing)p->Get<int>());
			break;

		case PropertyId::ZIndex:
			values.z_index((p->unit == Unit::KEYWORD ? ZIndex(ZIndex::Auto) : ZIndex(ZIndex::Number, p->Get<float>())));
			break;

		case PropertyId::Width:
			values.width(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::MinWidth:
			values.min_width(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::MaxWidth:
			values.max_width(ComputeMaxSize(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::Height:
			values.height(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::MinHeight:
			values.min_height(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::MaxHeight:
			values.max_height(ComputeMaxSize(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::LineHeight:
			// (Line-height computed above)
			break;
		case PropertyId::VerticalAlign:
			values.vertical_align(ComputeVerticalAlign(p, values.line_height().value, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::OverflowX:
			values.overflow_x((Overflow)p->Get< int >());
			break;
		case PropertyId::OverflowY:
			values.overflow_y((Overflow)p->Get< int >());
			break;
		case PropertyId::Clip:
			values.clip(ComputeClip(p));
			break;
		case PropertyId::Visibility:
			values.visibility((Visibility)p->Get< int >());
			break;

		case PropertyId::BackgroundColor:
			values.background_color(p->Get<Colourb>());
			break;
		case PropertyId::Color:
			values.color(p->Get<Colourb>());
			break;
		case PropertyId::ImageColor:
			values.image_color(p->Get<Colourb>());
			break;
		case PropertyId::Opacity:
			values.opacity(p->Get<float>());
			break;

		case PropertyId::FontFamily:
			// Fetched from element's properties.
			dirty_font_face_handle = true;
			break;
		case PropertyId::FontStyle:
			values.font_style((FontStyle)p->Get< int >());
			dirty_font_face_handle = true;
			break;
		case PropertyId::FontWeight:
			values.font_weight((FontWeight)p->Get< int >());
			dirty_font_face_handle = true;
			break;
		case PropertyId::FontSize:
			// (font-size computed above)
			dirty_font_face_handle = true;
			break;
		case PropertyId::LetterSpacing:
			values.has_letter_spacing(p->unit != Unit::KEYWORD);
			dirty_font_face_handle = true;
			break;

		case PropertyId::TextAlign:
			values.text_align((TextAlign)p->Get<int>());
			break;
		case PropertyId::TextDecoration:
			values.text_decoration((TextDecoration)p->Get<int>());
			break;
		case PropertyId::TextTransform:
			values.text_transform((TextTransform)p->Get<int>());
			break;
		case PropertyId::WhiteSpace:
			values.white_space((WhiteSpace)p->Get<int>());
			break;
		case PropertyId::WordBreak:
			values.word_break((WordBreak)p->Get<int>());
			break;

		case PropertyId::RowGap:
			values.row_gap(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::ColumnGap:
			values.column_gap(ComputeLengthPercentage(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::Drag:
			values.drag((Drag)p->Get< int >());
			break;
		case PropertyId::TabIndex:
			values.tab_index((TabIndex)p->Get< int >());
			break;
		case PropertyId::Focus:
			values.focus((Focus)p->Get<int>());
			break;
		case PropertyId::ScrollbarMargin:
			values.scrollbar_margin(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::OverscrollBehavior:
			values.overscroll_behavior((OverscrollBehavior)p->Get<int>());
			break;
		case PropertyId::PointerEvents:
			values.pointer_events((PointerEvents)p->Get<int>());
			break;

		case PropertyId::Perspective:
			values.perspective(p->unit == Unit::KEYWORD ? 0.f : ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
			values.has_local_perspective(values.perspective() > 0.f);
			break;
		case PropertyId::PerspectiveOriginX:
			values.perspective_origin_x(ComputeOrigin(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::PerspectiveOriginY:
			values.perspective_origin_y(ComputeOrigin(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::Transform:
			values.has_local_transform(p->Get<TransformPtr>() != nullptr);
			break;
		case PropertyId::TransformOriginX:
			values.transform_origin_x(ComputeOrigin(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::TransformOriginY:
			values.transform_origin_y(ComputeOrigin(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;
		case PropertyId::TransformOriginZ:
			values.transform_origin_z(ComputeLength(p->GetNumericValue(), font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::Decorator:
			values.has_decorator(p->unit == Unit::DECORATOR && p->value.GetType() == Variant::DECORATORSPTR && p->value.GetReference<DecoratorsPtr>());
			break;
		case PropertyId::MaskImage:
			values.has_mask_image(p->unit == Unit::DECORATOR && p->value.GetType() == Variant::DECORATORSPTR && p->value.GetReference<DecoratorsPtr>());
			break;
		case PropertyId::FontEffect:
			values.has_font_effect(p->unit == Unit::FONTEFFECT && p->value.GetType() == Variant::FONTEFFECTSPTR && p->value.GetReference<FontEffectsPtr>());
			break;
		case PropertyId::Filter:
			values.has_filter(p->unit == Unit::FILTER && p->value.GetType() == Variant::FILTERSPTR && p->value.GetReference<FiltersPtr>());
			break;
		case PropertyId::BackdropFilter:
			values.has_backdrop_filter(p->unit == Unit::FILTER && p->value.GetType() == Variant::FILTERSPTR && p->value.GetReference<FiltersPtr>());
			break;
		case PropertyId::BoxShadow:
			values.has_box_shadow(p->unit == Unit::BOXSHADOWLIST && p->value.GetType() == Variant::BOXSHADOWLIST && !p->value.GetReference<BoxShadowList>().empty());
			break;

		case PropertyId::FlexBasis:
			values.flex_basis(ComputeLengthPercentageAuto(p, font_size, document_font_size, dp_ratio, vp_dimensions));
			break;

		case PropertyId::RmlUi_Language:
			values.language(p->Get<String>());
			break;
		case PropertyId::RmlUi_Direction:
			values.direction(p->Get<Direction>());
			break;

		// Fetched from element's properties.
		case PropertyId::Cursor:
		case PropertyId::Transition:
		case PropertyId::Animation:
		case PropertyId::AlignContent:
		case PropertyId::AlignItems:
		case PropertyId::AlignSelf:
		case PropertyId::FlexDirection:
		case PropertyId::FlexGrow:
		case PropertyId::FlexShrink:
		case PropertyId::FlexWrap:
		case PropertyId::JustifyContent:
			break;
		// Navigation properties. Must be manually retrieved with 'GetProperty()'.
		case PropertyId::NavUp:
		case PropertyId::NavDown:
		case PropertyId::NavLeft:
		case PropertyId::NavRight:
			break;
		// Unhandled properties. Must be manually retrieved with 'GetProperty()'.
		case PropertyId::FillImage:
		case PropertyId::CaretColor:
			break;
		// Invalid properties
		case PropertyId::Invalid:
		case PropertyId::NumDefinedIds:
		case PropertyId::MaxNumIds:
			break;
		}
		// clang-format on
	}

	// The font-face handle is nulled when local font properties are set. In that case we need to retrieve a new handle.
	if (dirty_font_face_handle)
	{
		RMLUI_ZoneScopedN("FontFaceHandle");
		values.font_face_handle(
			GetFontEngineInterface()->GetFontFaceHandle(values.font_family(), values.font_style(), values.font_weight(), (int)values.font_size()));
	}

	// Next, pass inheritable dirty properties onto our children
	PropertyIdSet dirty_inherited_properties = (dirty_properties & StyleSheetSpecification::GetRegisteredInheritedProperties());

	if (!dirty_inherited_properties.Empty())
	{
		for (int i = 0; i < element->GetNumChildren(true); i++)
		{
			auto child = element->GetChild(i);
			child->GetStyle()->dirty_properties |= dirty_inherited_properties;
		}
	}

	PropertyIdSet result(std::move(dirty_properties));
	dirty_properties.Clear();
	return result;
}

} // namespace Rml
