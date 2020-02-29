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

#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "PropertyParserNumber.h"
#include "PropertyParserAnimation.h"
#include "PropertyParserColour.h"
#include "PropertyParserKeyword.h"
#include "PropertyParserString.h"
#include "PropertyParserTransform.h"
#include "PropertyShorthandDefinition.h"
#include "IdNameMap.h"
#include "StringCache.h"

namespace Rml {
namespace Core {


static StyleSheetSpecification* instance = nullptr;


struct DefaultStyleSheetParsers {
	PropertyParserNumber number = PropertyParserNumber(Property::NUMBER);
	PropertyParserNumber length = PropertyParserNumber(Property::LENGTH, Property::PX);
	PropertyParserNumber length_percent = PropertyParserNumber(Property::LENGTH_PERCENT, Property::PX);
	PropertyParserNumber number_length_percent = PropertyParserNumber(Property::NUMBER_LENGTH_PERCENT, Property::PX);
	PropertyParserNumber angle = PropertyParserNumber(Property::ANGLE, Property::RAD);
	PropertyParserKeyword keyword = PropertyParserKeyword();
	PropertyParserString string = PropertyParserString();
	PropertyParserAnimation animation = PropertyParserAnimation(PropertyParserAnimation::ANIMATION_PARSER);
	PropertyParserAnimation transition = PropertyParserAnimation(PropertyParserAnimation::TRANSITION_PARSER);
	PropertyParserColour color = PropertyParserColour();
	PropertyParserTransform transform = PropertyParserTransform();
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

PropertyDefinition& StyleSheetSpecification::RegisterProperty(PropertyId id, const String& property_name, const String& default_value, bool inherited, bool forces_layout)
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

// Registers a parser for use in property definitions.
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

// Returns the parser registered with a specific name.
PropertyParser* StyleSheetSpecification::GetParser(const String& parser_name)
{
	ParserMap::iterator iterator = instance->parsers.find(parser_name);
	if (iterator == instance->parsers.end())
		return nullptr;

	return (*iterator).second;
}

// Registers a property with a new definition.
PropertyDefinition& StyleSheetSpecification::RegisterProperty(const String& property_name, const String& default_value, bool inherited, bool forces_layout)
{
	RMLUI_ASSERTMSG((size_t)instance->properties.property_map->GetId(property_name) < (size_t)PropertyId::FirstCustomId, "Custom property name matches an internal property, please make a unique name for the given property.");
	return instance->RegisterProperty(PropertyId::Invalid, property_name, default_value, inherited, forces_layout); 
}

// Returns a property definition.
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

const PropertyIdSet & StyleSheetSpecification::GetRegisteredInheritedProperties()
{
	return instance->properties.GetRegisteredInheritedProperties();
}

const PropertyIdSet& StyleSheetSpecification::GetRegisteredPropertiesForcingLayout()
{
	return instance->properties.GetRegisteredPropertiesForcingLayout();
}

// Registers a shorthand property definition.
ShorthandId StyleSheetSpecification::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type)
{
	RMLUI_ASSERTMSG(instance->properties.property_map->GetId(shorthand_name) == PropertyId::Invalid, "Custom shorthand name matches a property name, please make a unique name.");
	RMLUI_ASSERTMSG((size_t)instance->properties.shorthand_map->GetId(shorthand_name) < (size_t)ShorthandId::FirstCustomId, "Custom shorthand name matches an internal shorthand, please make a unique name for the given shorthand property.");
	return instance->properties.RegisterShorthand(shorthand_name, property_names, type);
}

// Returns a shorthand definition.
const ShorthandDefinition* StyleSheetSpecification::GetShorthand(const String& shorthand_name)
{
	return instance->properties.GetShorthand(shorthand_name);
}

const ShorthandDefinition* StyleSheetSpecification::GetShorthand(ShorthandId id)
{
	return instance->properties.GetShorthand(id);
}

// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
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

// Registers RmlUi's default parsers.
void StyleSheetSpecification::RegisterDefaultParsers()
{
	RegisterParser("number", &default_parsers->number);
	RegisterParser("length", &default_parsers->length);
	RegisterParser("length_percent", &default_parsers->length_percent);
	RegisterParser("number_length_percent", &default_parsers->number_length_percent);
	RegisterParser("angle", &default_parsers->angle);
	RegisterParser("keyword", &default_parsers->keyword);
	RegisterParser("string", &default_parsers->string);
	RegisterParser("animation", &default_parsers->animation);
	RegisterParser("transition", &default_parsers->transition);
	RegisterParser("color", &default_parsers->color);
	RegisterParser("transform", &default_parsers->transform);
}


// Registers RmlUi's default style properties.
void StyleSheetSpecification::RegisterDefaultProperties()
{
	/* 
		Style property specifications (ala RCSS).

		Note: Whenever keywords or default values are changed, make sure its computed value is
		changed correspondingly, see `ComputedValues.h`.

		When adding new properties, it may be desirable to add it to the computed values as well.
		Then, make sure to resolve it as appropriate in `ElementStyle.cpp`.

	*/

	RegisterProperty(PropertyId::MarginTop, MARGIN_TOP, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginRight, MARGIN_RIGHT, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginBottom, MARGIN_BOTTOM, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MarginLeft, MARGIN_LEFT, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(ShorthandId::Margin, MARGIN, "margin-top, margin-right, margin-bottom, margin-left", ShorthandType::Box);

	RegisterProperty(PropertyId::PaddingTop, PADDING_TOP, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingRight, PADDING_RIGHT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingBottom, PADDING_BOTTOM, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::PaddingLeft, PADDING_LEFT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(ShorthandId::Padding, PADDING, "padding-top, padding-right, padding-bottom, padding-left", ShorthandType::Box);

	RegisterProperty(PropertyId::BorderTopWidth, BORDER_TOP_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderRightWidth, BORDER_RIGHT_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderBottomWidth, BORDER_BOTTOM_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(PropertyId::BorderLeftWidth, BORDER_LEFT_WIDTH, "0px", false, true).AddParser("length");
	RegisterShorthand(ShorthandId::BorderWidth, BORDER_WIDTH, "border-top-width, border-right-width, border-bottom-width, border-left-width", ShorthandType::Box);

	RegisterProperty(PropertyId::BorderTopColor, BORDER_TOP_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::BorderRightColor, BORDER_RIGHT_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::BorderBottomColor, BORDER_BOTTOM_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::BorderLeftColor, BORDER_LEFT_COLOR, "black", false, false).AddParser(COLOR);
	RegisterShorthand(ShorthandId::BorderColor, BORDER_COLOR, "border-top-color, border-right-color, border-bottom-color, border-left-color", ShorthandType::Box);

	RegisterShorthand(ShorthandId::BorderTop, BORDER_TOP, "border-top-width, border-top-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderRight, BORDER_RIGHT, "border-right-width, border-right-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderBottom, BORDER_BOTTOM, "border-bottom-width, border-bottom-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::BorderLeft, BORDER_LEFT, "border-left-width, border-left-color", ShorthandType::FallThrough);
	RegisterShorthand(ShorthandId::Border, BORDER, "border-top, border-right, border-bottom, border-left", ShorthandType::RecursiveRepeat);

	RegisterProperty(PropertyId::Display, DISPLAY, "inline", false, true).AddParser("keyword", "none, block, inline, inline-block");
	RegisterProperty(PropertyId::Position, POSITION, "static", false, true).AddParser("keyword", "static, relative, absolute, fixed");
	RegisterProperty(PropertyId::Top, TOP, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::Right, RIGHT, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::Bottom, BOTTOM, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::Left, LEFT, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(PropertyId::Float, FLOAT, "none", false, true).AddParser("keyword", "none, left, right");
	RegisterProperty(PropertyId::Clear, CLEAR, "none", false, true).AddParser("keyword", "none, left, right, both");

	RegisterProperty(PropertyId::ZIndex, Z_INDEX, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("number");

	RegisterProperty(PropertyId::Width, WIDTH, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MinWidth, MIN_WIDTH, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PropertyId::MaxWidth, MAX_WIDTH, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(PropertyId::Height, HEIGHT, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::MinHeight, MIN_HEIGHT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(PropertyId::MaxHeight, MAX_HEIGHT, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);

	RegisterProperty(PropertyId::LineHeight, LINE_HEIGHT, "1.2", true, true).AddParser("number_length_percent").SetRelativeTarget(RelativeTarget::FontSize);
	RegisterProperty(PropertyId::VerticalAlign, VERTICAL_ALIGN, "baseline", false, true)
		.AddParser("keyword", "baseline, middle, sub, super, text-top, text-bottom, top, bottom")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::LineHeight);

	RegisterProperty(PropertyId::OverflowX, OVERFLOW_X, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterProperty(PropertyId::OverflowY, OVERFLOW_Y, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterShorthand(ShorthandId::Overflow, "overflow", "overflow-x, overflow-y", ShorthandType::Replicate);
	RegisterProperty(PropertyId::Clip, CLIP, "auto", true, false).AddParser("keyword", "auto, none").AddParser("number");
	RegisterProperty(PropertyId::Visibility, VISIBILITY, "visible", false, false).AddParser("keyword", "visible, hidden");

	// Need some work on this if we are to include images.
	RegisterProperty(PropertyId::BackgroundColor, BACKGROUND_COLOR, "transparent", false, false).AddParser(COLOR);
	RegisterShorthand(ShorthandId::Background, BACKGROUND, BACKGROUND_COLOR, ShorthandType::FallThrough);

	RegisterProperty(PropertyId::Color, COLOR, "white", true, false).AddParser(COLOR);

	RegisterProperty(PropertyId::ImageColor, IMAGE_COLOR, "white", false, false).AddParser(COLOR);
	RegisterProperty(PropertyId::Opacity, OPACITY, "1", true, false).AddParser("number");

	RegisterProperty(PropertyId::FontFamily, FONT_FAMILY, "", true, true).AddParser("string");
	RegisterProperty(PropertyId::FontStyle, FONT_STYLE, "normal", true, true).AddParser("keyword", "normal, italic");
	RegisterProperty(PropertyId::FontWeight, FONT_WEIGHT, "normal", true, true).AddParser("keyword", "normal, bold");
	RegisterProperty(PropertyId::FontSize, FONT_SIZE, "12px", true, true).AddParser("length").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ParentFontSize);
	RegisterShorthand(ShorthandId::Font, FONT, "font-style, font-weight, font-size, font-family", ShorthandType::FallThrough);

	RegisterProperty(PropertyId::TextAlign, TEXT_ALIGN, LEFT, true, true).AddParser("keyword", "left, right, center, justify");
	RegisterProperty(PropertyId::TextDecoration, TEXT_DECORATION, "none", true, false).AddParser("keyword", "none, underline, overline, line-through");
	RegisterProperty(PropertyId::TextTransform, TEXT_TRANSFORM, "none", true, true).AddParser("keyword", "none, capitalize, uppercase, lowercase");
	RegisterProperty(PropertyId::WhiteSpace, WHITE_SPACE, "normal", true, true).AddParser("keyword", "normal, pre, nowrap, pre-wrap, pre-line");

	RegisterProperty(PropertyId::Cursor, CURSOR, "", true, false).AddParser("string");

	// Functional property specifications.
	RegisterProperty(PropertyId::Drag, DRAG, "none", false, false).AddParser("keyword", "none, drag, drag-drop, block, clone");
	RegisterProperty(PropertyId::TabIndex, TAB_INDEX, "none", false, false).AddParser("keyword", "none, auto");
	RegisterProperty(PropertyId::Focus, FOCUS, "auto", true, false).AddParser("keyword", "none, auto");
	RegisterProperty(PropertyId::ScrollbarMargin, SCROLLBAR_MARGIN, "0", false, false).AddParser("length");
	RegisterProperty(PropertyId::PointerEvents, POINTER_EVENTS, "auto", true, false).AddParser("keyword", "none, auto");

	// Perspective and Transform specifications
	RegisterProperty(PropertyId::Perspective, PERSPECTIVE, "none", false, false).AddParser("keyword", "none").AddParser("length");
	RegisterProperty(PropertyId::PerspectiveOriginX, PERSPECTIVE_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PropertyId::PerspectiveOriginY, PERSPECTIVE_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterShorthand(ShorthandId::PerspectiveOrigin, PERSPECTIVE_ORIGIN, "perspective-origin-x, perspective-origin-y", ShorthandType::FallThrough);
	RegisterProperty(PropertyId::Transform, TRANSFORM, "none", false, false).AddParser(TRANSFORM);
	RegisterProperty(PropertyId::TransformOriginX, TRANSFORM_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PropertyId::TransformOriginY, TRANSFORM_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterProperty(PropertyId::TransformOriginZ, TRANSFORM_ORIGIN_Z, "0", false, false).AddParser("length");
	RegisterShorthand(ShorthandId::TransformOrigin, TRANSFORM_ORIGIN, "transform-origin-x, transform-origin-y, transform-origin-z", ShorthandType::FallThrough);

	RegisterProperty(PropertyId::Transition, TRANSITION, "none", false, false).AddParser(TRANSITION);
	RegisterProperty(PropertyId::Animation, ANIMATION, "none", false, false).AddParser(ANIMATION);

	RegisterProperty(PropertyId::Decorator, "decorator", "", false, false).AddParser("string");
	RegisterProperty(PropertyId::FontEffect, "font-effect", "", true, false).AddParser("string");

	// Rare properties (not added to computed values)
	RegisterProperty(PropertyId::FillImage, FILL_IMAGE, "", false, false).AddParser("string");

	instance->properties.property_map->AssertAllInserted(PropertyId::NumDefinedIds);
	instance->properties.shorthand_map->AssertAllInserted(ShorthandId::NumDefinedIds);
}

}
}
