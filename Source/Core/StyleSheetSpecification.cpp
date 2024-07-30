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

#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "IdNameMap.h"
#include "PropertyParserAnimation.h"
#include "PropertyParserBoxShadow.h"
#include "PropertyParserColorStopList.h"
#include "PropertyParserColour.h"
#include "PropertyParserDecorator.h"
#include "PropertyParserFilter.h"
#include "PropertyParserFontEffect.h"
#include "PropertyParserKeyword.h"
#include "PropertyParserNumber.h"
#include "PropertyParserRatio.h"
#include "PropertyParserString.h"
#include "PropertyParserTransform.h"
#include "PropertyShorthandDefinition.h"

namespace Rml {

static StyleSheetSpecification* instance = nullptr;

struct DefaultStyleSheetParsers : NonCopyMoveable {
	PropertyParserNumber number = PropertyParserNumber(Unit::NUMBER);
	PropertyParserNumber length = PropertyParserNumber(Unit::LENGTH, Unit::PX);
	PropertyParserNumber length_percent = PropertyParserNumber(Unit::LENGTH_PERCENT, Unit::PX);
	PropertyParserNumber number_percent = PropertyParserNumber(Unit::NUMBER_PERCENT);
	PropertyParserNumber number_length_percent = PropertyParserNumber(Unit::NUMBER_LENGTH_PERCENT, Unit::PX);
	PropertyParserNumber angle = PropertyParserNumber(Unit::ANGLE, Unit::RAD);
	PropertyParserKeyword keyword = PropertyParserKeyword();
	PropertyParserString string = PropertyParserString();
	PropertyParserAnimation animation = PropertyParserAnimation(PropertyParserAnimation::ANIMATION_PARSER);
	PropertyParserAnimation transition = PropertyParserAnimation(PropertyParserAnimation::TRANSITION_PARSER);
	PropertyParserColour color = PropertyParserColour();
	PropertyParserColorStopList color_stop_list = PropertyParserColorStopList(&color);
	PropertyParserDecorator decorator = PropertyParserDecorator();
	PropertyParserFilter filter = PropertyParserFilter();
	PropertyParserFontEffect font_effect = PropertyParserFontEffect();
	PropertyParserTransform transform = PropertyParserTransform();
	PropertyParserRatio ratio = PropertyParserRatio();
	PropertyParserNumber resolution = PropertyParserNumber(Unit::X);
	PropertyParserBoxShadow box_shadow = PropertyParserBoxShadow(&color, &length);
};

StyleSheetSpecification::StyleSheetSpecification() :
	// Reserve space for all defined ids and some more for custom properties
	properties((size_t)PropertyId::MaxNumIds, 2 * (size_t)ShorthandId::NumDefinedIds)
{
	RMLUI_ASSERT(instance == nullptr);
	instance = this;

	default_parsers.reset(new DefaultStyleSheetParsers);
}

StyleSheetSpecification::~StyleSheetSpecification()
{
	RMLUI_ASSERT(instance == this);
	instance = nullptr;
}

PropertyDefinition& StyleSheetSpecification::RegisterProperty(PropertyId id, const String& property_name, const String& default_value, bool inherited,
	bool forces_layout)
{
	return properties.RegisterProperty(property_name, default_value, inherited, forces_layout, id);
}

ShorthandId StyleSheetSpecification::RegisterShorthand(ShorthandId id, const String& shorthand_name, const String& property_names, ShorthandType type)
{
	return properties.RegisterShorthand(shorthand_name, property_names, type, id);
}

bool StyleSheetSpecification::Initialise()
{
	if (instance == nullptr)
	{
		new StyleSheetSpecification();

		instance->RegisterDefaultParsers();
		instance->RegisterDefaultProperties();
	}

	return true;
}

void StyleSheetSpecification::Shutdown()
{
	if (instance != nullptr)
	{
		delete instance;
	}
}

bool StyleSheetSpecification::RegisterParser(const String& parser_name, PropertyParser* parser)
{
	ParserMap::iterator iterator = instance->parsers.find(parser_name);
	if (iterator != instance->parsers.end())
	{
		Log::Message(Log::LT_WARNING, "Parser with name %s already exists!", parser_name.c_str());
		return false;
	}

	instance->parsers[parser_name] = parser;
	return true;
}

PropertyParser* StyleSheetSpecification::GetParser(const String& parser_name)
{
	ParserMap::iterator iterator = instance->parsers.find(parser_name);
	if (iterator == instance->parsers.end())
		return nullptr;

	return (*iterator).second;
}

PropertyDefinition& StyleSheetSpecification::RegisterProperty(const String& property_name, const String& default_value, bool inherited,
	bool forces_layout)
{
	RMLUI_ASSERTMSG((size_t)instance->properties.property_map->GetId(property_name) < (size_t)PropertyId::FirstCustomId,
		"Custom property name matches an internal property, please make a unique name for the given property.");
	return instance->RegisterProperty(PropertyId::Invalid, property_name, default_value, inherited, forces_layout);
}

const PropertyDefinition* StyleSheetSpecification::GetProperty(const String& property_name)
{
	return instance->properties.GetProperty(property_name);
}

const PropertyDefinition* StyleSheetSpecification::GetProperty(PropertyId id)
{
	return instance->properties.GetProperty(id);
}

const PropertyIdSet& StyleSheetSpecification::GetRegisteredProperties()
{
	return instance->properties.GetRegisteredProperties();
}

const PropertyIdSet& StyleSheetSpecification::GetRegisteredInheritedProperties()
{
	return instance->properties.GetRegisteredInheritedProperties();
}

const PropertyIdSet& StyleSheetSpecification::GetRegisteredPropertiesForcingLayout()
{
	return instance->properties.GetRegisteredPropertiesForcingLayout();
}

ShorthandId StyleSheetSpecification::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type)
{
	RMLUI_ASSERTMSG(instance->properties.property_map->GetId(shorthand_name) == PropertyId::Invalid,
		"Custom shorthand name matches a property name, please make a unique name.");
	RMLUI_ASSERTMSG((size_t)instance->properties.shorthand_map->GetId(shorthand_name) < (size_t)ShorthandId::FirstCustomId,
		"Custom shorthand name matches an internal shorthand, please make a unique name for the given shorthand property.");
	return instance->properties.RegisterShorthand(shorthand_name, property_names, type);
}

const ShorthandDefinition* StyleSheetSpecification::GetShorthand(const String& shorthand_name)
{
	return instance->properties.GetShorthand(shorthand_name);
}

const ShorthandDefinition* StyleSheetSpecification::GetShorthand(ShorthandId id)
{
	return instance->properties.GetShorthand(id);
}

bool StyleSheetSpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value)
{
	return instance->properties.ParsePropertyDeclaration(dictionary, property_name, property_value);
}

PropertyId StyleSheetSpecification::GetPropertyId(const String& property_name)
{
	return instance->properties.property_map->GetId(property_name);
}

ShorthandId StyleSheetSpecification::GetShorthandId(const String& shorthand_name)
{
	return instance->properties.shorthand_map->GetId(shorthand_name);
}

const String& StyleSheetSpecification::GetPropertyName(PropertyId id)
{
	return instance->properties.property_map->GetName(id);
}

const String& StyleSheetSpecification::GetShorthandName(ShorthandId id)
{
	return instance->properties.shorthand_map->GetName(id);
}

PropertyIdSet StyleSheetSpecification::GetShorthandUnderlyingProperties(ShorthandId id)
{
	PropertyIdSet result;
	const ShorthandDefinition* shorthand = instance->properties.GetShorthand(id);
	if (!shorthand)
		return result;

	for (auto& item : shorthand->items)
	{
		if (item.type == ShorthandItemType::Property)
		{
			result.Insert(item.property_id);
		}
		else if (item.type == ShorthandItemType::Shorthand)
		{
			// When we have a shorthand pointing to another shorthands, call us recursively. Add the union of the previous result and new properties.
			result |= GetShorthandUnderlyingProperties(item.shorthand_id);
		}
	}
	return result;
}

const PropertySpecification& StyleSheetSpecification::GetPropertySpecification()
{
	return instance->properties;
}

void StyleSheetSpecification::RegisterDefaultParsers()
{
	RegisterParser("number", &default_parsers->number);
	RegisterParser("length", &default_parsers->length);
	RegisterParser("length_percent", &default_parsers->length_percent);
	RegisterParser("number_percent", &default_parsers->number_percent);
	RegisterParser("number_length_percent", &default_parsers->number_length_percent);
	RegisterParser("angle", &default_parsers->angle);
	RegisterParser("keyword", &default_parsers->keyword);
	RegisterParser("string", &default_parsers->string);
	RegisterParser("animation", &default_parsers->animation);
	RegisterParser("transition", &default_parsers->transition);
	RegisterParser("color", &default_parsers->color);
	RegisterParser("color_stop_list", &default_parsers->color_stop_list);
	RegisterParser("decorator", &default_parsers->decorator);
	RegisterParser("filter", &default_parsers->filter);
	RegisterParser("font_effect", &default_parsers->font_effect);
	RegisterParser("transform", &default_parsers->transform);
	RegisterParser("ratio", &default_parsers->ratio);
	RegisterParser("resolution", &default_parsers->resolution);
	RegisterParser("box_shadow", &default_parsers->box_shadow);
}

void StyleSheetSpecification::RegisterDefaultProperties()
{
	/*
	    Style property specifications (ala RCSS).

	    Note: Whenever keywords or default values are changed, make sure its computed value is
	    changed correspondingly, see `ComputedValues.h`.

	    When adding new properties, it may be desirable to add it to the computed values as well.
	    Then, make sure to resolve it as appropriate in `ElementStyle.cpp`.

	*/

	// clang-format off
	RegisterProperty(PropertyId::MarginTop, "margin-top", "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginRight, "margin-right", "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginBottom, "margin-bottom", "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginLeft, "margin-left", "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(ShorthandId::Margin, "margin", "margin-top, margin-right, margin-bottom, margin-left", ShorthandType::Box);

	RegisterProperty(PropertyId::PaddingTop, "padding-top", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingRight, "padding-right", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingBottom, "padding-bottom", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingLeft, "padding-left", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(ShorthandId::Padding, "padding", "padding-top, padding-right, padding-bottom, padding-left", ShorthandType::Box);

	RegisterProperty(PropertyId::BorderTopWidth, "border-top-width", "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderRightWidth, "border-right-width", "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderBottomWidth, "border-bottom-width", "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderLeftWidth, "border-left-width", "0px", false, true).AddParser("length");
	RegisterShorthand(ShorthandId::BorderWidth, "border-width", "border-top-width, border-right-width, border-bottom-width, border-left-width", ShorthandType::Box);

	RegisterProperty(PropertyId::BorderTopColor, "border-top-color", "black", false, false).AddParser("color");
	RegisterProperty(PropertyId::BorderRightColor, "border-right-color", "black", false, false).AddParser("color");
	RegisterProperty(PropertyId::BorderBottomColor, "border-bottom-color", "black", false, false).AddParser("color");
	RegisterProperty(PropertyId::BorderLeftColor, "border-left-color", "black", false, false).AddParser("color");
	RegisterShorthand(ShorthandId::BorderColor, "border-color", "border-top-color, border-right-color, border-bottom-color, border-left-color", ShorthandType::Box);

	RegisterShorthand(ShorthandId::BorderTop, "border-top", "border-top-width, border-top-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderRight, "border-right", "border-right-width, border-right-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderBottom, "border-bottom", "border-bottom-width, border-bottom-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderLeft, "border-left", "border-left-width, border-left-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::Border, "border", "border-top, border-right, border-bottom, border-left", ShorthandType::RecursiveRepeat);

	RegisterProperty(PropertyId::BorderTopLeftRadius, "border-top-left-radius", "0px", false, false).AddParser("length");
	RegisterProperty(PropertyId::BorderTopRightRadius, "border-top-right-radius", "0px", false, false).AddParser("length");
	RegisterProperty(PropertyId::BorderBottomRightRadius, "border-bottom-right-radius", "0px", false, false).AddParser("length");
	RegisterProperty(PropertyId::BorderBottomLeftRadius, "border-bottom-left-radius", "0px", false, false).AddParser("length");
	RegisterShorthand(ShorthandId::BorderRadius, "border-radius", "border-top-left-radius, border-top-right-radius, border-bottom-right-radius, border-bottom-left-radius", ShorthandType::Box);

	RegisterProperty(PropertyId::Display, "display", "inline", false, true)
		.AddParser("keyword", "none, block, inline, inline-block, flow-root, flex, inline-flex, table, inline-table, table-row, table-row-group, table-column, table-column-group, table-cell");
	RegisterProperty(PropertyId::Position, "position", "static", false, true).AddParser("keyword", "static, relative, absolute, fixed");
	RegisterProperty(PropertyId::Top, "top", "auto", false, false).AddParser("keyword", "auto").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::Right, "right", "auto", false, false).AddParser("keyword", "auto").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::Bottom, "bottom", "auto", false, false).AddParser("keyword", "auto").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::Left, "left", "auto", false, false).AddParser("keyword", "auto").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(PropertyId::Float, "float", "none", false, true).AddParser("keyword", "none, left, right");
	RegisterProperty(PropertyId::Clear, "clear", "none", false, true).AddParser("keyword", "none, left, right, both");

	RegisterProperty(PropertyId::BoxSizing, "box-sizing", "content-box", false, true).AddParser("keyword", "content-box, border-box");

	RegisterProperty(PropertyId::ZIndex, "z-index", "auto", false, false).AddParser("keyword", "auto").AddParser("number");

	RegisterProperty(PropertyId::Width, "width", "auto", false, true).AddParser("keyword", "auto").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MinWidth, "min-width", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MaxWidth, "max-width", "none", false, true).AddParser("keyword", "none").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(PropertyId::Height, "height", "auto", false, true).AddParser("keyword", "auto").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::MinHeight, "min-height", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::MaxHeight, "max-height", "none", false, true).AddParser("keyword", "none").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);

	RegisterProperty(PropertyId::LineHeight, "line-height", "1.2", true, true).AddParser("number_length_percent").SetRelativeTarget(RelativeTarget::FontSize);
	RegisterProperty(PropertyId::VerticalAlign, "vertical-align", "baseline", false, true)
		.AddParser("keyword", "baseline, middle, sub, super, text-top, text-bottom, top, center, bottom")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::LineHeight);

	RegisterProperty(PropertyId::OverflowX, "overflow-x", "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterProperty(PropertyId::OverflowY, "overflow-y", "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterShorthand(ShorthandId::Overflow, "overflow", "overflow-x, overflow-y", ShorthandType::Replicate);
	RegisterProperty(PropertyId::Clip, "clip", "auto", false, false).AddParser("keyword", "auto, none, always").AddParser("number");
	RegisterProperty(PropertyId::Visibility, "visibility", "visible", false, false).AddParser("keyword", "visible, hidden");

	// Need some work on this if we are to include images.
	RegisterProperty(PropertyId::BackgroundColor, "background-color", "transparent", false, false).AddParser("color");
	RegisterShorthand(ShorthandId::Background, "background", "background-color", ShorthandType::FallThrough);

	RegisterProperty(PropertyId::Color, "color", "white", true, false).AddParser("color");

	RegisterProperty(PropertyId::CaretColor, "caret-color", "auto", true, false).AddParser("keyword", "auto").AddParser("color");

	RegisterProperty(PropertyId::ImageColor, "image-color", "white", false, false).AddParser("color");
	RegisterProperty(PropertyId::Opacity, "opacity", "1", true, false).AddParser("number");

	RegisterProperty(PropertyId::FontFamily, "font-family", "", true, true).AddParser("string");
	RegisterProperty(PropertyId::FontStyle, "font-style", "normal", true, true).AddParser("keyword", "normal, italic");
	RegisterProperty(PropertyId::FontWeight, "font-weight", "normal", true, true).AddParser("keyword", "normal=400, bold=700").AddParser("number");
	RegisterProperty(PropertyId::FontSize, "font-size", "12px", true, true).AddParser("length").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ParentFontSize);
	RegisterProperty(PropertyId::LetterSpacing, "letter-spacing", "normal", true, true).AddParser("keyword", "normal").AddParser("length");
	RegisterShorthand(ShorthandId::Font, "font", "font-style, font-weight, font-size, font-family", ShorthandType::FallThrough);

	RegisterProperty(PropertyId::TextAlign, "text-align", "left", true, true).AddParser("keyword", "left, right, center, justify");
	RegisterProperty(PropertyId::TextDecoration, "text-decoration", "none", true, false).AddParser("keyword", "none, underline, overline, line-through");
	RegisterProperty(PropertyId::TextTransform, "text-transform", "none", true, true).AddParser("keyword", "none, capitalize, uppercase, lowercase");
	RegisterProperty(PropertyId::WhiteSpace, "white-space", "normal", true, true).AddParser("keyword", "normal, pre, nowrap, pre-wrap, pre-line");
	RegisterProperty(PropertyId::WordBreak, "word-break", "normal", true, true).AddParser("keyword", "normal, break-all, break-word");

	RegisterProperty(PropertyId::RowGap, "row-gap", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::ColumnGap, "column-gap", "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterShorthand(ShorthandId::Gap, "gap", "row-gap, column-gap", ShorthandType::Replicate);

	RegisterProperty(PropertyId::Cursor, "cursor", "", true, false).AddParser("string");

	// Functional property specifications.
	RegisterProperty(PropertyId::Drag, "drag", "none", false, false).AddParser("keyword", "none, drag, drag-drop, block, clone");
	RegisterProperty(PropertyId::TabIndex, "tab-index", "none", false, false).AddParser("keyword", "none, auto");
	RegisterProperty(PropertyId::Focus, "focus", "auto", true, false).AddParser("keyword", "none, auto");

	RegisterProperty(PropertyId::NavUp, "nav-up", "none", false, false).AddParser("keyword", "none, auto, horizontal, vertical").AddParser("string");
	RegisterProperty(PropertyId::NavRight, "nav-right", "none", false, false).AddParser("keyword", "none, auto, horizontal, vertical").AddParser("string");
	RegisterProperty(PropertyId::NavDown, "nav-down", "none", false, false).AddParser("keyword", "none, auto, horizontal, vertical").AddParser("string");
	RegisterProperty(PropertyId::NavLeft, "nav-left", "none", false, false).AddParser("keyword", "none, auto, horizontal, vertical").AddParser("string");
	RegisterShorthand(ShorthandId::Nav, "nav", "nav-up, nav-right, nav-down, nav-left", ShorthandType::Box);

	RegisterProperty(PropertyId::ScrollbarMargin, "scrollbar-margin", "0", false, false).AddParser("length");
	RegisterProperty(PropertyId::OverscrollBehavior, "overscroll-behavior", "auto", false, false).AddParser("keyword", "auto, contain");
	RegisterProperty(PropertyId::PointerEvents, "pointer-events", "auto", true, false).AddParser("keyword", "none, auto");

	// Perspective and Transform specifications
	RegisterProperty(PropertyId::Perspective, "perspective", "none", false, false).AddParser("keyword", "none").AddParser("length");
	RegisterProperty(PropertyId::PerspectiveOriginX, "perspective-origin-x", "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PropertyId::PerspectiveOriginY, "perspective-origin-y", "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterShorthand(ShorthandId::PerspectiveOrigin, "perspective-origin", "perspective-origin-x, perspective-origin-y", ShorthandType::FallThrough);
	RegisterProperty(PropertyId::Transform, "transform", "none", false, false).AddParser("transform");
	RegisterProperty(PropertyId::TransformOriginX, "transform-origin-x", "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PropertyId::TransformOriginY, "transform-origin-y", "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterProperty(PropertyId::TransformOriginZ, "transform-origin-z", "0", false, false).AddParser("length");
	RegisterShorthand(ShorthandId::TransformOrigin, "transform-origin", "transform-origin-x, transform-origin-y, transform-origin-z", ShorthandType::FallThrough);

	RegisterProperty(PropertyId::Transition, "transition", "none", false, false).AddParser("transition");
	RegisterProperty(PropertyId::Animation, "animation", "none", false, false).AddParser("animation");

	// Decorators and effects
	RegisterProperty(PropertyId::Decorator, "decorator", "", false, false).AddParser("decorator");
	RegisterProperty(PropertyId::MaskImage, "mask-image", "", false, false).AddParser("decorator");
	RegisterProperty(PropertyId::FontEffect, "font-effect", "", true, false).AddParser("font_effect");
		
	RegisterProperty(PropertyId::Filter, "filter", "", false, false).AddParser("filter", "filter");
	RegisterProperty(PropertyId::BackdropFilter, "backdrop-filter", "", false, false).AddParser("filter");
	
	RegisterProperty(PropertyId::BoxShadow, "box-shadow", "none", false, false).AddParser("box_shadow");

	// Rare properties (not added to computed values)
	RegisterProperty(PropertyId::FillImage, "fill-image", "", false, false).AddParser("string");

	// Flexbox
	RegisterProperty(PropertyId::AlignContent, "align-content", "stretch", false, true).AddParser("keyword", "flex-start, flex-end, center, space-between, space-around, space-evenly, stretch");
	RegisterProperty(PropertyId::AlignItems, "align-items", "stretch", false, true).AddParser("keyword", "flex-start, flex-end, center, baseline, stretch");
	RegisterProperty(PropertyId::AlignSelf, "align-self", "auto", false, true).AddParser("keyword", "auto, flex-start, flex-end, center, baseline, stretch");
	
	RegisterProperty(PropertyId::FlexBasis, "flex-basis", "auto", false, true).AddParser("keyword", "auto").AddParser("length_percent");
	RegisterProperty(PropertyId::FlexDirection, "flex-direction", "row", false, true).AddParser("keyword", "row, row-reverse, column, column-reverse");

	RegisterProperty(PropertyId::FlexGrow, "flex-grow", "0", false, true).AddParser("number");
	RegisterProperty(PropertyId::FlexShrink, "flex-shrink", "1", false, true).AddParser("number");
	RegisterProperty(PropertyId::FlexWrap, "flex-wrap", "nowrap", false, true).AddParser("keyword", "nowrap, wrap, wrap-reverse");
	RegisterProperty(PropertyId::JustifyContent, "justify-content", "flex-start", false, true).AddParser("keyword", "flex-start, flex-end, center, space-between, space-around, space-evenly");

	RegisterShorthand(ShorthandId::Flex, "flex", "flex-grow, flex-shrink, flex-basis", ShorthandType::Flex);
	RegisterShorthand(ShorthandId::FlexFlow, "flex-flow", "flex-direction, flex-wrap", ShorthandType::FallThrough);

	// Internationalization properties (internal)
	RegisterProperty(PropertyId::RmlUi_Language, "--rmlui-language", "", true, true).AddParser("string");
	RegisterProperty(PropertyId::RmlUi_Direction, "--rmlui-direction", "auto", true, true).AddParser("keyword", "auto, ltr, rtl");

	RMLUI_ASSERTMSG(instance->properties.shorthand_map->AssertAllInserted(ShorthandId::NumDefinedIds), "Missing specification for one or more Shorthand IDs.");
	RMLUI_ASSERTMSG(instance->properties.property_map->AssertAllInserted(PropertyId::NumDefinedIds), "Missing specification for one or more Property IDs.");
	// clang-format on
}

} // namespace Rml
