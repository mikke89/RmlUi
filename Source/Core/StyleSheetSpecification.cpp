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
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"
#include "PropertyParserNumber.h"
#include "PropertyParserAnimation.h"
#include "PropertyParserColour.h"
#include "PropertyParserKeyword.h"
#include "PropertyParserString.h"
#include "PropertyParserTransform.h"

namespace Rocket {
namespace Core {

static StyleSheetSpecification* instance = NULL;

StyleSheetSpecification::StyleSheetSpecification()
{
	ROCKET_ASSERT(instance == NULL);
	instance = this;
}

StyleSheetSpecification::~StyleSheetSpecification()
{
	ROCKET_ASSERT(instance == this);
	instance = NULL;
}

bool StyleSheetSpecification::Initialise()
{
	if (instance == NULL)
	{
		new StyleSheetSpecification();

		instance->RegisterDefaultParsers();
		instance->RegisterDefaultProperties();
	}

	return true;
}

void StyleSheetSpecification::Shutdown()
{
	if (instance != NULL)
	{
		for (ParserMap::iterator iterator = instance->parsers.begin(); iterator != instance->parsers.end(); ++iterator)
			(*iterator).second->Release();

		delete instance;
	}
}

// Registers a parser for use in property definitions.
bool StyleSheetSpecification::RegisterParser(const String& parser_name, PropertyParser* parser)
{
	ParserMap::iterator iterator = instance->parsers.find(parser_name);
	if (iterator != instance->parsers.end())
		(*iterator).second->Release();

	instance->parsers[parser_name] = parser;
	return true;
}

// Returns the parser registered with a specific name.
PropertyParser* StyleSheetSpecification::GetParser(const String& parser_name)
{
	ParserMap::iterator iterator = instance->parsers.find(parser_name);
	if (iterator == instance->parsers.end())
		return NULL;

	return (*iterator).second;
}

// Registers a property with a new definition.
PropertyDefinition& StyleSheetSpecification::RegisterProperty(const String& property_name, const String& default_value, bool inherited, bool forces_layout)
{
	return instance->properties.RegisterProperty(property_name, default_value, inherited, forces_layout);
}

// Returns a property definition.
const PropertyDefinition* StyleSheetSpecification::GetProperty(const String& property_name)
{
	return instance->properties.GetProperty(property_name);
}

// Fetches a list of the names of all registered property definitions.
const PropertyNameList& StyleSheetSpecification::GetRegisteredProperties()
{
	return instance->properties.GetRegisteredProperties();
}

const PropertyNameList & StyleSheetSpecification::GetRegisteredInheritedProperties()
{
	return instance->properties.GetRegisteredInheritedProperties();
}

// Registers a shorthand property definition.
bool StyleSheetSpecification::RegisterShorthand(const String& shorthand_name, const String& property_names, PropertySpecification::ShorthandType type)
{
	return instance->properties.RegisterShorthand(shorthand_name, property_names, type);
}

// Returns a shorthand definition.
const PropertyShorthandDefinition* StyleSheetSpecification::GetShorthand(const String& shorthand_name)
{
	return instance->properties.GetShorthand(shorthand_name);
}

// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
bool StyleSheetSpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value, const String& source_file, int source_line_number)
{
	return instance->properties.ParsePropertyDeclaration(dictionary, property_name, property_value, source_file, source_line_number);
}

// Registers Rocket's default parsers.
void StyleSheetSpecification::RegisterDefaultParsers()
{
	RegisterParser("number", new PropertyParserNumber(Property::NUMBER));
	RegisterParser("length", new PropertyParserNumber(Property::LENGTH, Property::PX));
	RegisterParser("length_percent", new PropertyParserNumber(Property::LENGTH_PERCENT, Property::PX));
	RegisterParser("number_length_percent", new PropertyParserNumber(Property::NUMBER_LENGTH_PERCENT, Property::PX));
	RegisterParser("angle", new PropertyParserNumber(Property::ANGLE, Property::RAD));
	RegisterParser("keyword", new PropertyParserKeyword());
	RegisterParser("string", new PropertyParserString());
	RegisterParser(ANIMATION, new PropertyParserAnimation(PropertyParserAnimation::ANIMATION_PARSER));
	RegisterParser(TRANSITION, new PropertyParserAnimation(PropertyParserAnimation::TRANSITION_PARSER));
	RegisterParser(COLOR, new PropertyParserColour());
	RegisterParser(TRANSFORM, new PropertyParserTransform());
}


// Registers Rocket's default style properties.
void StyleSheetSpecification::RegisterDefaultProperties()
{
	// Style property specifications (ala RCSS).

	RegisterProperty(MARGIN_TOP, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(MARGIN_RIGHT, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(MARGIN_BOTTOM, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(MARGIN_LEFT, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(MARGIN, "margin-top, margin-right, margin-bottom, margin-left");

	RegisterProperty(PADDING_TOP, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PADDING_RIGHT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PADDING_BOTTOM, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PADDING_LEFT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(PADDING, "padding-top, padding-right, padding-bottom, padding-left");

	RegisterProperty(BORDER_TOP_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(BORDER_RIGHT_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(BORDER_BOTTOM_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(BORDER_LEFT_WIDTH, "0px", false, true).AddParser("length");
	RegisterShorthand(BORDER_WIDTH, "border-top-width, border-right-width, border-bottom-width, border-left-width");

	RegisterProperty(BORDER_TOP_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(BORDER_RIGHT_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(BORDER_BOTTOM_COLOR, "black", false, false).AddParser(COLOR);
	RegisterProperty(BORDER_LEFT_COLOR, "black", false, false).AddParser(COLOR);
	RegisterShorthand(BORDER_COLOR, "border-top-color, border-right-color, border-bottom-color, border-left-color");

	RegisterShorthand(BORDER_TOP, "border-top-width, border-top-color");
	RegisterShorthand(BORDER_RIGHT, "border-right-width, border-right-color");
	RegisterShorthand(BORDER_BOTTOM, "border-bottom-width, border-bottom-color");
	RegisterShorthand(BORDER_LEFT, "border-left-width, border-left-color");
	RegisterShorthand(BORDER, "border-top, border-right, border-bottom, border-left", PropertySpecification::RECURSIVE);

	RegisterProperty(DISPLAY, "inline", false, true).AddParser("keyword", "none, block, inline, inline-block");
	RegisterProperty(POSITION, "static", false, true).AddParser("keyword", "static, relative, absolute, fixed");
	RegisterProperty(TOP, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(RIGHT, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(BOTTOM, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(LEFT, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(FLOAT, "none", false, true).AddParser("keyword", "none, left, right");
	RegisterProperty(CLEAR, "none", false, true).AddParser("keyword", "none, left, right, both");

	RegisterProperty(Z_INDEX, "auto", false, false)
		.AddParser("keyword", "auto, top, bottom")
		.AddParser("number");

	RegisterProperty(WIDTH, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(MIN_WIDTH, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(MAX_WIDTH, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(HEIGHT, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(MIN_HEIGHT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(MAX_HEIGHT, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);

	RegisterProperty(LINE_HEIGHT, "1.2", true, true).AddParser("number_length_percent").SetRelativeTarget(RelativeTarget::FontSize);
	RegisterProperty(VERTICAL_ALIGN, "baseline", false, true)
		.AddParser("keyword", "baseline, middle, sub, super, text-top, text-bottom, top, bottom")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::LineHeight);

	RegisterProperty(OVERFLOW_X, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterProperty(OVERFLOW_Y, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterShorthand("overflow", "overflow-x, overflow-y", PropertySpecification::REPLICATE);
	RegisterProperty(CLIP, "auto", true, false).AddParser("keyword", "auto, none").AddParser("number");
	RegisterProperty(VISIBILITY, "visible", false, false).AddParser("keyword", "visible, hidden");

	// Need some work on this if we are to include images.
	RegisterProperty(BACKGROUND_COLOR, "transparent", false, false).AddParser(COLOR);
	RegisterShorthand(BACKGROUND, BACKGROUND_COLOR);

	RegisterProperty(COLOR, "white", true, false).AddParser(COLOR);

	RegisterProperty(IMAGE_COLOR, "white", false, false).AddParser(COLOR);
	RegisterProperty(OPACITY, "1", true, false).AddParser("number");

	RegisterProperty(FONT_FAMILY, "", true, true).AddParser("string");
	RegisterProperty(FONT_CHARSET, "U+0020-007E", true, false).AddParser("string");
	RegisterProperty(FONT_STYLE, "normal", true, true).AddParser("keyword", "normal, italic");
	RegisterProperty(FONT_WEIGHT, "normal", true, true).AddParser("keyword", "normal, bold");
	RegisterProperty(FONT_SIZE, "12px", true, true).AddParser("length").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ParentFontSize);
	RegisterShorthand(FONT, "font-style, font-weight, font-size, font-family, font-charset");

	RegisterProperty(TEXT_ALIGN, LEFT, true, true).AddParser("keyword", "left, right, center, justify");
	RegisterProperty(TEXT_DECORATION, "none", true, false).AddParser("keyword", "none, underline"/*"none, underline, overline, line-through"*/);
	RegisterProperty(TEXT_TRANSFORM, "none", true, true).AddParser("keyword", "none, capitalize, uppercase, lowercase");
	RegisterProperty(WHITE_SPACE, "normal", true, true).AddParser("keyword", "normal, pre, nowrap, pre-wrap, pre-line");

	RegisterProperty(CURSOR, "auto", true, false).AddParser("keyword", "auto").AddParser("string");

	// Functional property specifications.
	RegisterProperty(DRAG, "none", false, false).AddParser("keyword", "none, drag, drag-drop, block, clone");
	RegisterProperty(TAB_INDEX, "none", false, false).AddParser("keyword", "none, auto");
	RegisterProperty(FOCUS, "auto", true, false).AddParser("keyword", "none, auto");
	RegisterProperty(SCROLLBAR_MARGIN, "0", false, false).AddParser("length");
	RegisterProperty(POINTER_EVENTS, "auto", true, false).AddParser("keyword", "auto, none");

	// Perspective and Transform specifications
	RegisterProperty(PERSPECTIVE, "none", false, false).AddParser("keyword", "none").AddParser("length");
	RegisterProperty(PERSPECTIVE_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PERSPECTIVE_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterShorthand(PERSPECTIVE_ORIGIN, "perspective-origin-x, perspective-origin-y");
	RegisterProperty(TRANSFORM, "none", false, false).AddParser(TRANSFORM);
	RegisterProperty(TRANSFORM_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(TRANSFORM_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterProperty(TRANSFORM_ORIGIN_Z, "0", false, false).AddParser("length");
	RegisterShorthand(TRANSFORM_ORIGIN, "transform-origin-x, transform-origin-y, transform-origin-z");

	RegisterProperty(TRANSITION, "none", false, false).AddParser(TRANSITION);
	RegisterProperty(ANIMATION, "none", false, false).AddParser(ANIMATION);
}

}
}
