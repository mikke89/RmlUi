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

	using Id = PropertyId;

	RegisterProperty(Id::MarginTop, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::MarginRight, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::MarginBottom, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::MarginLeft, "0px", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(Id::Margin, { Id::MarginTop, Id::MarginRight, Id::MarginBottom, Id::MarginLeft });

	RegisterProperty(Id::PaddingTop, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::PaddingRight, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::PaddingBottom, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::PaddingLeft, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterShorthand(Id::Padding, { Id::PaddingTop, Id::PaddingRight, Id::PaddingBottom, Id::PaddingLeft });

	RegisterProperty(Id::BorderTopWidth, "0px", false, true).AddParser("length");
	RegisterProperty(Id::BorderRightWidth, "0px", false, true).AddParser("length");
	RegisterProperty(Id::BorderBottomWidth, "0px", false, true).AddParser("length");
	RegisterProperty(Id::BorderLeftWidth, "0px", false, true).AddParser("length");
	RegisterShorthand(Id::BorderWidth, { Id::BorderTopWidth, Id::BorderRightWidth, Id::BorderBottomWidth, Id::BorderLeftWidth });

	RegisterProperty(Id::BorderTopColor, "black", false, false).AddParser("color");
	RegisterProperty(Id::BorderRightColor, "black", false, false).AddParser("color");
	RegisterProperty(Id::BorderBottomColor, "black", false, false).AddParser("color");
	RegisterProperty(Id::BorderLeftColor, "black", false, false).AddParser("color");
	RegisterShorthand(Id::BorderColor, { Id::BorderTopColor, Id::BorderRightColor, Id::BorderBottomColor, Id::BorderLeftColor });

	RegisterShorthand(Id::BorderTop, { Id::BorderTopWidth, Id::BorderTopColor });
	RegisterShorthand(Id::BorderRight, { Id::BorderRightWidth, Id::BorderRightColor });
	RegisterShorthand(Id::BorderBottom, { Id::BorderBottomWidth, Id::BorderBottomColor });
	RegisterShorthand(Id::BorderLeft, { Id::BorderLeftWidth, Id::BorderLeftColor });
	RegisterShorthand(Id::Border, { Id::BorderTop, Id::BorderRight, Id::BorderBottom, Id::BorderLeft }, PropertySpecification::RECURSIVE);

	RegisterProperty(Id::Display, "inline", false, true).AddParser("keyword", "none, block, inline, inline-block");
	RegisterProperty(Id::Position, "static", false, true).AddParser("keyword", "static, relative, absolute, fixed");
	RegisterProperty(Id::Top, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(Id::Right, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::Bottom, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(Id::Left, "auto", false, false)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(Id::Float, "none", false, true).AddParser("keyword", "none, left, right");
	RegisterProperty(Id::Clear, "none", false, true).AddParser("keyword", "none, left, right, both");

	RegisterProperty(Id::ZIndex, "auto", false, false)
		.AddParser("keyword", "auto, top, bottom")
		.AddParser("number");

	RegisterProperty(Id::Width, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::MinWidth, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);
	RegisterProperty(Id::MaxWidth, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockWidth);

	RegisterProperty(Id::Height, "auto", false, true)
		.AddParser("keyword", "auto")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(Id::MinHeight, "0px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);
	RegisterProperty(Id::MaxHeight, "-1px", false, true).AddParser("length_percent").SetRelativeTarget(RelativeTarget::ContainingBlockHeight);

	RegisterProperty(Id::LineHeight, "1.2", true, true).AddParser("number_length_percent").SetRelativeTarget(RelativeTarget::FontSize);
	RegisterProperty(Id::VerticalAlign, "baseline", false, true)
		.AddParser("keyword", "baseline, middle, sub, super, text-top, text-bottom, top, bottom")
		.AddParser("length_percent").SetRelativeTarget(RelativeTarget::LineHeight);

	RegisterProperty(Id::OverflowX, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterProperty(Id::OverflowY, "visible", false, true).AddParser("keyword", "visible, hidden, auto, scroll");
	RegisterShorthand(Id::Overflow, { Id::OverflowX, Id::OverflowY }, PropertySpecification::REPLICATE);
	RegisterProperty(Id::Clip, "auto", true, false).AddParser("keyword", "auto, none").AddParser("number");
	RegisterProperty(Id::Visibility, "visible", false, false).AddParser("keyword", "visible, hidden");

	// Need some work on this if we are to include images.
	RegisterProperty(Id::BackgroundColor, "transparent", false, false).AddParser("color");
	RegisterShorthand(Id::Background, { Id::BackgroundColor });

	RegisterProperty(Id::Color, "white", true, false).AddParser("color");

	RegisterProperty(Id::ImageColor, "white", false, false).AddParser("color");
	RegisterProperty(Id::Opacity, "1", true, false).AddParser("number");

	RegisterProperty(Id::FontFamily, "", true, true).AddParser("string");
	RegisterProperty(Id::FontCharset, "U+0020-007E", true, false).AddParser("string");
	RegisterProperty(Id::FontStyle, "normal", true, true).AddParser("keyword", "normal, italic");
	RegisterProperty(Id::FontWeight, "normal", true, true).AddParser("keyword", "normal, bold");
	RegisterProperty(Id::FontSize, "12px", true, true).AddParser("length").AddParser("length_percent").SetRelativeTarget(RelativeTarget::ParentFontSize);
	RegisterShorthand(Id::Font, { Id::FontStyle, Id::FontWeight, Id::FontSize, Id::FontFamily, Id::FontCharset });

	RegisterProperty(Id::TextAlign, "left", true, true).AddParser("keyword", "left, right, center, justify");
	RegisterProperty(Id::TextDecoration, "none", true, false).AddParser("keyword", "none, underline"/*"none, underline, overline, line-through"*/);
	RegisterProperty(Id::TextTransform, "none", true, true).AddParser("keyword", "none, capitalize, uppercase, lowercase");
	RegisterProperty(Id::WhiteSpace, "normal", true, true).AddParser("keyword", "normal, pre, nowrap, pre-wrap, pre-line");

	RegisterProperty(Id::Cursor, "auto", true, false).AddParser("keyword", "auto").AddParser("string");

	// Functional property specifications.
	RegisterProperty(Id::DragProperty, "none", false, false).AddParser("keyword", "none, drag, drag-drop, block, clone");
	RegisterProperty(Id::TabIndex, "none", false, false).AddParser("keyword", "none, auto");
	RegisterProperty(Id::Focus, "auto", true, false).AddParser("keyword", "none, auto");
	RegisterProperty(Id::ScrollbarMargin, "0", false, false).AddParser("length");
	RegisterProperty(Id::PointerEvents, "auto", true, false).AddParser("keyword", "auto, none");

	// Perspective and Transform specifications
	RegisterProperty(Id::Perspective, "none", false, false).AddParser("keyword", "none").AddParser("length");
	RegisterProperty(Id::PerspectiveOriginX, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(Id::PerspectiveOriginY, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterShorthand(Id::PerspectiveOrigin, { Id::PerspectiveOriginX, Id::PerspectiveOriginY});
	RegisterProperty(Id::Transform, "none", false, false).AddParser("transform");
	RegisterProperty(Id::TransformOriginX, "50%", false, false).AddParser("keyword", "left, center, right").AddParser("length_percent");
	RegisterProperty(Id::TransformOriginY, "50%", false, false).AddParser("keyword", "top, center, bottom").AddParser("length_percent");
	RegisterProperty(Id::TransformOriginZ, "0", false, false).AddParser("length");
	RegisterShorthand(Id::TransformOrigin, { Id::TransformOriginX, Id::TransformOriginY, Id::TransformOriginZ });

	RegisterProperty(Id::Transition, "none", false, false).AddParser("transition");
	RegisterProperty(Id::Animation, "none", false, false).AddParser("animation");
}

}
}
