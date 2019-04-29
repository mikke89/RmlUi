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
PropertyDefinition& StyleSheetSpecification::RegisterProperty(PropertyId property_id, const String& default_value, bool inherited, bool forces_layout)
{
	return instance->properties.RegisterProperty(property_id, default_value, inherited, forces_layout);
}

// Returns a property definition.
const PropertyDefinition* StyleSheetSpecification::GetProperty(PropertyId id)
{
	return instance->properties.GetProperty(id);
}

// Fetches a list of the names of all registered property definitions.
const PropertyIdList& StyleSheetSpecification::GetRegisteredProperties()
{
	return instance->properties.GetRegisteredProperties();
}

const PropertyIdList & StyleSheetSpecification::GetRegisteredInheritedProperties()
{
	return instance->properties.GetRegisteredInheritedProperties();
}

// Registers a shorthand property definition.
bool StyleSheetSpecification::RegisterShorthand(PropertyId shorthand_id, const PropertyIdList& property_ids, PropertySpecification::ShorthandType type)
{
	return instance->properties.RegisterShorthand(shorthand_id, property_ids, type);
}

// Returns a shorthand definition.
const PropertyShorthandDefinition* StyleSheetSpecification::GetShorthand(PropertyId id)
{
	return instance->properties.GetShorthand(id);
}

// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
bool StyleSheetSpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, PropertyId property_id, const String& property_value, const String& source_file, int source_line_number)
{
	return instance->properties.ParsePropertyDeclaration(dictionary, property_id, property_value, source_file, source_line_number);
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
	RegisterParser("animation", new PropertyParserAnimation(PropertyParserAnimation::ANIMATION_PARSER));
	RegisterParser("transition", new PropertyParserAnimation(PropertyParserAnimation::TRANSITION_PARSER));
	RegisterParser("color", new PropertyParserColour());
	RegisterParser("transform", new PropertyParserTransform());
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
	RegisterShorthand(MARGIN, { MARGIN_TOP, MARGIN_RIGHT, MARGIN_BOTTOM, MARGIN_LEFT });

	RegisterProperty(PADDING_TOP, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PADDING_RIGHT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PADDING_BOTTOM, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(PADDING_LEFT, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(PADDING, { PADDING_TOP, PADDING_RIGHT, PADDING_BOTTOM, PADDING_LEFT });

	RegisterProperty(BORDER_TOP_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(BORDER_RIGHT_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(BORDER_BOTTOM_WIDTH, "0px", false, true).AddParser("length");
	RegisterProperty(BORDER_LEFT_WIDTH, "0px", false, true).AddParser("length");
	RegisterShorthand(BORDER_WIDTH, { BORDER_TOP_WIDTH, BORDER_RIGHT_WIDTH, BORDER_BOTTOM_WIDTH, BORDER_LEFT_WIDTH });

	RegisterProperty(BORDER_TOP_COLOR, "black", false, false).AddParser("color");
	RegisterProperty(BORDER_RIGHT_COLOR, "black", false, false).AddParser("color");
	RegisterProperty(BORDER_BOTTOM_COLOR, "black", false, false).AddParser("color");
	RegisterProperty(BORDER_LEFT_COLOR, "black", false, false).AddParser("color");
	RegisterShorthand(BORDER_COLOR, { BORDER_TOP_COLOR, BORDER_RIGHT_COLOR, BORDER_BOTTOM_COLOR, BORDER_LEFT_COLOR });

	RegisterShorthand(BORDER_TOP, { BORDER_TOP_WIDTH, BORDER_TOP_COLOR });
	RegisterShorthand(BORDER_RIGHT, { BORDER_RIGHT_WIDTH, BORDER_RIGHT_COLOR });
	RegisterShorthand(BORDER_BOTTOM, { BORDER_BOTTOM_WIDTH, BORDER_BOTTOM_COLOR });
	RegisterShorthand(BORDER_LEFT, { BORDER_LEFT_WIDTH, BORDER_LEFT_COLOR });
	RegisterShorthand(BORDER, { BORDER_TOP, BORDER_RIGHT, BORDER_BOTTOM, BORDER_LEFT }, PropertySpecification::RECURSIVE);

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
	RegisterShorthand(OVERFLOW_, { OVERFLOW_X, OVERFLOW_Y }, PropertySpecification::REPLICATE);
	RegisterProperty(CLIP, "auto", true, false).AddParser("keyword", "auto, none").AddParser("number");
	RegisterProperty(VISIBILITY, "visible", false, false).AddParser("keyword", "visible, hidden");

	// Need some work on this if we are to include images.
	RegisterProperty(BACKGROUND_COLOR, "transparent", false, false).AddParser("color");
	RegisterShorthand(BACKGROUND, { BACKGROUND_COLOR });

	RegisterProperty(COLOR, "white", true, false).AddParser("color");

	RegisterProperty(IMAGE_COLOR, "white", false, false).AddParser("color");
	RegisterProperty(OPACITY, "1", true, false).AddParser("number");

	RegisterProperty(FONT_FAMILY, "", true, true).AddParser("string");
	RegisterProperty(FONT_CHARSET, "U+0020-007E", true, false).AddParser("string");
	RegisterProperty(FONT_STYLE, "normal", true, true).AddParser("keyword", "normal, italic");
	RegisterProperty(FONT_WEIGHT, "normal", true, true).AddParser("keyword", "normal, bold");
	RegisterProperty(FONT_SIZE, "12px", true, true).AddParser("length").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ParentFontSize);
	RegisterShorthand(FONT, { FONT_STYLE, FONT_WEIGHT, FONT_SIZE, FONT_FAMILY, FONT_CHARSET });

	RegisterProperty(TEXT_ALIGN, "left", true, true).AddParser("keyword", "left, right, center, justify");
	RegisterProperty(TEXT_DECORATION, "none", true, false).AddParser("keyword", "none, underline"/*"none, underline, overline, line-through"*/);
	RegisterProperty(TEXT_TRANSFORM, "none", true, true).AddParser("keyword", "none, capitalize, uppercase, lowercase");
	RegisterProperty(WHITE_SPACE, "normal", true, true).AddParser("keyword", "normal, pre, nowrap, pre-wrap, pre-line");

	RegisterProperty(CURSOR, "auto", true, false).AddParser("keyword", "auto").AddParser("string");

	// Functional property specifications.
	RegisterProperty(DRAG_PROPERTY, "none", false, false).AddParser("keyword", "none, drag, drag-drop, block, clone");
	RegisterProperty(TAB_INDEX, "none", false, false).AddParser("keyword", "none, auto");
	RegisterProperty(FOCUS_PROPERTY, "auto", true, false).AddParser("keyword", "none, auto");
	RegisterProperty(SCROLLBAR_MARGIN, "0", false, false).AddParser("length");
	RegisterProperty(POINTER_EVENTS, "auto", true, false).AddParser("keyword", "auto, none");

	// Perspective and Transform specifications
	RegisterProperty(PERSPECTIVE, "none", false, false).AddParser("keyword", "none").AddParser("length");
	RegisterProperty(PERSPECTIVE_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(PERSPECTIVE_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterShorthand(PERSPECTIVE_ORIGIN, { PERSPECTIVE_ORIGIN_X, PERSPECTIVE_ORIGIN_Y });
	RegisterProperty(TRANSFORM, "none", false, false).AddParser("transform");
	RegisterProperty(TRANSFORM_ORIGIN_X, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(TRANSFORM_ORIGIN_Y, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterProperty(TRANSFORM_ORIGIN_Z, "0", false, false).AddParser("length");
	RegisterShorthand(TRANSFORM_ORIGIN, { TRANSFORM_ORIGIN_X, TRANSFORM_ORIGIN_Y, TRANSFORM_ORIGIN_Z });

	RegisterProperty(TRANSITION, "none", false, false).AddParser("transition");
	RegisterProperty(ANIMATION, "none", false, false).AddParser("animation");
}

}
}
