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

#include "StyleSheetParser.h"
#include "ComputeProperty.h"
#include "StyleSheetFactory.h"
#include "StyleSheetNode.h"
#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/StyleSheetContainer.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include <algorithm>
#include <string.h>

namespace Rml {

class AbstractPropertyParser {
public:
	virtual bool Parse(const String& name, const String& value) = 0;
};

/*
 *  PropertySpecificationParser just passes the parsing to a property specification. Usually
 *    the main stylesheet specification, except for e.g. @decorator blocks.
*/
class PropertySpecificationParser final : public AbstractPropertyParser {
private:
	// The dictionary to store the properties in.
	PropertyDictionary& properties;

	// The specification used to parse the values. Normally the default stylesheet specification, but not for e.g. all at-rules such as decorators.
	const PropertySpecification& specification;

public:
	PropertySpecificationParser(PropertyDictionary& properties, const PropertySpecification& specification) : properties(properties), specification(specification) {}

	bool Parse(const String& name, const String& value) override
	{
		return specification.ParsePropertyDeclaration(properties, name, value);
	}
};

/*
 *  Spritesheets need a special parser because its property names are arbitrary keys,
 *    while its values are always rectangles. Thus, it must be parsed with a special "rectangle" parser
 *    for every name-value pair. We can probably optimize this for @performance.
*/
class SpritesheetPropertyParser final : public AbstractPropertyParser {
private:
	String image_source;
	float image_resolution_factor = 1.f;
	SpriteDefinitionList sprite_definitions;

	PropertyDictionary properties;
	PropertySpecification specification;
	PropertyId id_rx, id_ry, id_rw, id_rh, id_resolution;
	ShorthandId id_rectangle;

public:
	SpritesheetPropertyParser() : specification(4, 1) 
	{
		id_rx = specification.RegisterProperty("rectangle-x", "", false, false).AddParser("length").GetId();
		id_ry = specification.RegisterProperty("rectangle-y", "", false, false).AddParser("length").GetId();
		id_rw = specification.RegisterProperty("rectangle-w", "", false, false).AddParser("length").GetId();
		id_rh = specification.RegisterProperty("rectangle-h", "", false, false).AddParser("length").GetId();
		id_rectangle = specification.RegisterShorthand("rectangle", "rectangle-x, rectangle-y, rectangle-w, rectangle-h", ShorthandType::FallThrough);
		id_resolution = specification.RegisterProperty("resolution", "", false, false).AddParser("resolution").GetId();
	}

	const String& GetImageSource() const
	{
		return image_source;
	}
	const SpriteDefinitionList& GetSpriteDefinitions() const
	{
		return sprite_definitions;
	}
	float GetImageResolutionFactor() const
	{
		return image_resolution_factor;
	}

	void Clear() {
		image_resolution_factor = 1.f;
		image_source.clear();
		sprite_definitions.clear();
	}

	bool Parse(const String& name, const String& value) override
	{
		if (name == "src")
		{
			image_source = value;
		}
		else if (name == "resolution")
		{
			if (!specification.ParsePropertyDeclaration(properties, id_resolution, value))
				return false;

			if (const Property* property = properties.GetProperty(id_resolution))
			{
				if (property->unit == Property::X)
					image_resolution_factor = property->Get<float>();
			}
		}
		else
		{
			if (!specification.ParseShorthandDeclaration(properties, id_rectangle, value))
				return false;

			Rectangle rectangle;
			if (auto property = properties.GetProperty(id_rx))
				rectangle.x = ComputeAbsoluteLength(*property, 1.f, Vector2f(1.f));
			if (auto property = properties.GetProperty(id_ry))
				rectangle.y = ComputeAbsoluteLength(*property, 1.f, Vector2f(1.f));
			if (auto property = properties.GetProperty(id_rw))
				rectangle.width = ComputeAbsoluteLength(*property, 1.f, Vector2f(1.f));
			if (auto property = properties.GetProperty(id_rh))
				rectangle.height = ComputeAbsoluteLength(*property, 1.f, Vector2f(1.f));

			sprite_definitions.emplace_back(name, rectangle);
		}

		return true;
	}
};


static UniquePtr<SpritesheetPropertyParser> spritesheet_property_parser;

/*
 * Media queries need a special parser because they have unique properties that 
 * aren't admissible in other property declaration contexts and the syntax of
*/
class MediaQueryPropertyParser final : public AbstractPropertyParser {
private:
	// The dictionary to store the properties in.
	PropertyDictionary* properties;
	PropertySpecification specification;

	static inline PropertyId CastId(MediaQueryId id)
	{
		return static_cast<PropertyId>(id);
	}

public:
	MediaQueryPropertyParser() : specification(14, 0) 
	{	
		specification.RegisterProperty("width", "", false, false, CastId(MediaQueryId::Width)).AddParser("length");
		specification.RegisterProperty("min-width", "", false, false, CastId(MediaQueryId::MinWidth)).AddParser("length");
		specification.RegisterProperty("max-width", "", false, false, CastId(MediaQueryId::MaxWidth)).AddParser("length");

		specification.RegisterProperty("height", "", false, false, CastId(MediaQueryId::Height)).AddParser("length");
		specification.RegisterProperty("min-height", "", false, false, CastId(MediaQueryId::MinHeight)).AddParser("length");
		specification.RegisterProperty("max-height", "", false, false, CastId(MediaQueryId::MaxHeight)).AddParser("length");

		specification.RegisterProperty("aspect-ratio", "", false, false, CastId(MediaQueryId::AspectRatio)).AddParser("ratio");
		specification.RegisterProperty("min-aspect-ratio", "", false, false, CastId(MediaQueryId::MinAspectRatio)).AddParser("ratio");
		specification.RegisterProperty("max-aspect-ratio", "", false, false, CastId(MediaQueryId::MaxAspectRatio)).AddParser("ratio");

		specification.RegisterProperty("resolution", "", false, false, CastId(MediaQueryId::Resolution)).AddParser("resolution");
		specification.RegisterProperty("min-resolution", "", false, false, CastId(MediaQueryId::MinResolution)).AddParser("resolution");
		specification.RegisterProperty("max-resolution", "", false, false, CastId(MediaQueryId::MaxResolution)).AddParser("resolution");

		specification.RegisterProperty("orientation", "", false, false, CastId(MediaQueryId::Orientation)).AddParser("keyword", "landscape, portrait");

		specification.RegisterProperty("theme", "", false, false, CastId(MediaQueryId::Theme)).AddParser("string");
	}

	void SetTargetProperties(PropertyDictionary* _properties)
	{
		properties = _properties;
	}

	void Clear() {
		properties = nullptr;
	}

	bool Parse(const String& name, const String& value) override
	{
		RMLUI_ASSERT(properties);
		return specification.ParsePropertyDeclaration(*properties, name, value);
	}
};


static UniquePtr<MediaQueryPropertyParser> media_query_property_parser;


StyleSheetParser::StyleSheetParser()
{
	line_number = 0;
	stream = nullptr;
	parse_buffer_pos = 0;
}

StyleSheetParser::~StyleSheetParser()
{
}

void StyleSheetParser::Initialise()
{
	spritesheet_property_parser = MakeUnique<SpritesheetPropertyParser>();
	media_query_property_parser = MakeUnique<MediaQueryPropertyParser>();
}

void StyleSheetParser::Shutdown()
{
	spritesheet_property_parser.reset();
	media_query_property_parser.reset();
}

static bool IsValidIdentifier(const String& str)
{
	if (str.empty())
		return false;

	for (size_t i = 0; i < str.size(); i++)
	{
		char c = str[i];
		bool valid = (
			(c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')
			|| (c == '-')
			|| (c == '_')
			);
		if (!valid)
			return false;
	}

	return true;
}


static void PostprocessKeyframes(KeyframesMap& keyframes_map)
{
	for (auto& keyframes_pair : keyframes_map)
	{
		Keyframes& keyframes = keyframes_pair.second;
		auto& blocks = keyframes.blocks;
		auto& property_ids = keyframes.property_ids;

		// Sort keyframes on selector value.
		std::sort(blocks.begin(), blocks.end(), [](const KeyframeBlock& a, const KeyframeBlock& b) { return a.normalized_time < b.normalized_time; });

		// Add all property names specified by any block
		if(blocks.size() > 0) property_ids.reserve(blocks.size() * blocks[0].properties.GetNumProperties());
		for(auto& block : blocks)
		{
			for (auto& property : block.properties.GetProperties())
				property_ids.push_back(property.first);
		}
		// Remove duplicate property names
		std::sort(property_ids.begin(), property_ids.end());
		property_ids.erase(std::unique(property_ids.begin(), property_ids.end()), property_ids.end());
		property_ids.shrink_to_fit();
	}

}


bool StyleSheetParser::ParseKeyframeBlock(KeyframesMap& keyframes_map, const String& identifier, const String& rules, const PropertyDictionary& properties)
{
	if (!IsValidIdentifier(identifier))
	{
		Log::Message(Log::LT_WARNING, "Invalid keyframes identifier '%s' at %s:%d", identifier.c_str(), stream_file_name.c_str(), line_number);
		return false;
	}
	if (properties.GetNumProperties() == 0)
		return true;

	StringList rule_list;
	StringUtilities::ExpandString(rule_list, rules);

	Vector<float> rule_values;
	rule_values.reserve(rule_list.size());

	for (auto rule : rule_list)
	{
		float value = 0.0f;
		int count = 0;
		rule = StringUtilities::ToLower(rule);
		if (rule == "from")
			rule_values.push_back(0.0f);
		else if (rule == "to")
			rule_values.push_back(1.0f);
		else if(sscanf(rule.c_str(), "%f%%%n", &value, &count) == 1)
			if(count > 0 && value >= 0.0f && value <= 100.0f)
				rule_values.push_back(0.01f * value);
	}

	if (rule_values.empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid keyframes rule(s) '%s' at %s:%d", rules.c_str(), stream_file_name.c_str(), line_number);
		return false;
	}

	Keyframes& keyframes = keyframes_map[identifier];

	for(float selector : rule_values)
	{
		auto it = std::find_if(keyframes.blocks.begin(), keyframes.blocks.end(), [selector](const KeyframeBlock& keyframe_block) { return Math::AbsoluteValue(keyframe_block.normalized_time - selector) < 0.0001f; });
		if (it == keyframes.blocks.end())
		{
			keyframes.blocks.emplace_back(selector);
			it = (keyframes.blocks.end() - 1);
		}
		else
		{
			// In case of duplicate keyframes, we only use the latest definition as per CSS rules
			it->properties = PropertyDictionary();
		}

		it->properties.Import(properties);
	}

	return true;
}

bool StyleSheetParser::ParseDecoratorBlock(const String& at_name, DecoratorSpecificationMap& decorator_map, const StyleSheet& style_sheet, const SharedPtr<const PropertySource>& source)
{
	StringList name_type;
	StringUtilities::ExpandString(name_type, at_name, ':');

	if (name_type.size() != 2 || name_type[0].empty() || name_type[1].empty())
	{
		Log::Message(Log::LT_WARNING, "Decorator syntax error at %s:%d. Use syntax: '@decorator name : type { ... }'.", stream_file_name.c_str(), line_number);
		return false;
	}

	const String& name = name_type[0];
	String decorator_type = name_type[1];

	auto it_find = decorator_map.find(name);
	if (it_find != decorator_map.end())
	{
		Log::Message(Log::LT_WARNING, "Decorator with name '%s' already declared, ignoring decorator at %s:%d.", name.c_str(), stream_file_name.c_str(), line_number);
		return false;
	}

	// Get the instancer associated with the decorator type
	DecoratorInstancer* decorator_instancer = Factory::GetDecoratorInstancer(decorator_type);
	PropertyDictionary properties;

	if(!decorator_instancer)
	{
		// Type is not a declared decorator type, instead, see if it is another decorator name, then we inherit its properties.
		auto it = decorator_map.find(decorator_type);
		if (it != decorator_map.end())
		{
			// Yes, try to retrieve the instancer from the parent type, and add its property values.
			decorator_instancer = Factory::GetDecoratorInstancer(it->second.decorator_type);
			properties = it->second.properties;
			decorator_type = it->second.decorator_type;
		}

		// If we still don't have an instancer, we cannot continue.
		if (!decorator_instancer)
		{
			Log::Message(Log::LT_WARNING, "Invalid decorator type '%s' declared at %s:%d.", decorator_type.c_str(), stream_file_name.c_str(), line_number);
			return false;
		}
	}

	const PropertySpecification& property_specification = decorator_instancer->GetPropertySpecification();

	PropertySpecificationParser parser(properties, property_specification);
	if (!ReadProperties(parser))
		return false;

	// Set non-defined properties to their defaults
	property_specification.SetPropertyDefaults(properties);
	properties.SetSourceOfAllProperties(source);

	SharedPtr<Decorator> decorator = decorator_instancer->InstanceDecorator(decorator_type, properties, DecoratorInstancerInterface(style_sheet, source.get()));
	if (!decorator)
	{
		Log::Message(Log::LT_WARNING, "Could not instance decorator of type '%s' declared at %s:%d.", decorator_type.c_str(), stream_file_name.c_str(), line_number);
		return false;
	}

	decorator_map.emplace(name, DecoratorSpecification{ std::move(decorator_type), std::move(properties), std::move(decorator) });

	return true;
}

bool StyleSheetParser::ParseMediaFeatureMap(PropertyDictionary& properties, const String & rules)
{
	media_query_property_parser->SetTargetProperties(&properties);

	enum ParseState { Global, Name, Value };
	ParseState state = Name;

	char character = 0;

	size_t cursor = 0;

	String name;

	String current_string;

	while(cursor++ < rules.length())
	{
		character = rules[cursor];

		switch(character)
		{
		case '(':
		{
			if (state != Global)
			{
				Log::Message(Log::LT_WARNING, "Unexpected '(' in @media query list at %s:%d.", stream_file_name.c_str(), line_number);
				return false;
			}

			current_string = StringUtilities::StripWhitespace(StringUtilities::ToLower(current_string));

			if (current_string != "and")
			{
				Log::Message(Log::LT_WARNING, "Unexpected '%s' in @media query list at %s:%d. Expected 'and'.", current_string.c_str(), stream_file_name.c_str(), line_number);
				return false;
			}

			current_string.clear();
			state = Name;
		}
		break;
		case ')':
		{
			if (state != Value)
			{
				Log::Message(Log::LT_WARNING, "Unexpected ')' in @media query list at %s:%d.", stream_file_name.c_str(), line_number);
				return false;
			}

			current_string = StringUtilities::StripWhitespace(current_string);

			if(!media_query_property_parser->Parse(name, current_string))
				Log::Message(Log::LT_WARNING, "Syntax error parsing media-query property declaration '%s: %s;' in %s: %d.", name.c_str(), current_string.c_str(), stream_file_name.c_str(), line_number);

			current_string.clear();
			state = Global;
		}
		break;
		case ':':
		{
			if (state != Name)
			{
				Log::Message(Log::LT_WARNING, "Unexpected ':' in @media query list at %s:%d.", stream_file_name.c_str(), line_number);
				return false;
			}

			current_string = StringUtilities::StripWhitespace(StringUtilities::ToLower(current_string));

			if (!IsValidIdentifier(current_string))
			{
				Log::Message(Log::LT_WARNING, "Malformed property name '%s' in @media query list at %s:%d.", current_string.c_str(), stream_file_name.c_str(), line_number);
				return false;
			}

			name = current_string;
			current_string.clear();

			state = Value;
		}
		break;
		default:
			current_string += character;
		}
	}

	if (properties.GetNumProperties() == 0)
	{
		Log::Message(Log::LT_WARNING, "Media query list parsing yielded no properties at %s:%d.", stream_file_name.c_str(), line_number);
	}

	return true;
}

bool StyleSheetParser::Parse(MediaBlockList& style_sheets, Stream* _stream, int begin_line_number)
{
	RMLUI_ZoneScoped;

	int rule_count = 0;
	line_number = begin_line_number;
	stream = _stream;
	stream_file_name = StringUtilities::Replace(stream->GetSourceURL().GetURL(), '|', ':');

	enum class State { Global, AtRuleIdentifier, KeyframeBlock, Invalid };
	State state = State::Global;


	MediaBlock current_block = {};

	// Need to track whether currently inside a nested media block or not, since the default scope is also a media block
	bool inside_media_block = false;

	// At-rules given by the following syntax in global space: @identifier name { block }
	String at_rule_name;

	// Look for more styles while data is available
	while (FillBuffer())
	{
		String pre_token_str;
		
		while (char token = FindToken(pre_token_str, "{@}", true))
		{
			switch (state)
			{
			case State::Global:
			{
				if (token == '{')
				{
					// Initialize current block if not present
					if (!current_block.stylesheet)
					{
						current_block = MediaBlock{PropertyDictionary{}, UniquePtr<StyleSheet>(new StyleSheet())};
					}

					const int rule_line_number = line_number;
					
					// Read the attributes
					PropertyDictionary properties;
					PropertySpecificationParser parser(properties, StyleSheetSpecification::GetPropertySpecification());
					if (!ReadProperties(parser))
						continue;

					StringList rule_name_list;
					StringUtilities::ExpandString(rule_name_list, pre_token_str);

					// Add style nodes to the root of the tree
					for (size_t i = 0; i < rule_name_list.size(); i++)
					{
						auto source = MakeShared<PropertySource>(stream_file_name, rule_line_number, rule_name_list[i]);
						properties.SetSourceOfAllProperties(source);
						ImportProperties(current_block.stylesheet->root.get(), rule_name_list[i], properties, rule_count);
					}

					rule_count++;
				}
				else if (token == '@')
				{
					state = State::AtRuleIdentifier;
				}
				else if (inside_media_block && token == '}')
				{
					// Complete current block
					PostprocessKeyframes(current_block.stylesheet->keyframes);
					current_block.stylesheet->specificity_offset = rule_count;
					style_sheets.push_back(std::move(current_block));
					current_block = {};

					inside_media_block = false;
					break;
				}
				else
				{
					Log::Message(Log::LT_WARNING, "Invalid character '%c' found while parsing stylesheet at %s:%d. Trying to proceed.", token, stream_file_name.c_str(), line_number);
				}
			}
			break;
			case State::AtRuleIdentifier:
			{
				if (token == '{')
				{					
					// Initialize current block if not present
					if (!current_block.stylesheet)
					{
						current_block = {PropertyDictionary{}, UniquePtr<StyleSheet>(new StyleSheet())};
					}

					const String at_rule_identifier = StringUtilities::StripWhitespace(pre_token_str.substr(0, pre_token_str.find(' ')));
					at_rule_name = StringUtilities::StripWhitespace(pre_token_str.substr(at_rule_identifier.size()));

					if (at_rule_identifier == "keyframes")
					{
						state = State::KeyframeBlock;
					}
					else if (at_rule_identifier == "decorator")
					{
						auto source = MakeShared<PropertySource>(stream_file_name, (int)line_number, pre_token_str);
						ParseDecoratorBlock(at_rule_name, current_block.stylesheet->decorator_map, *current_block.stylesheet, source);
						
						at_rule_name.clear();
						state = State::Global;
					}
					else if (at_rule_identifier == "spritesheet")
					{
						// The spritesheet parser is reasonably heavy to initialize, so we make it a static global.
						ReadProperties(*spritesheet_property_parser);

						const String& image_source = spritesheet_property_parser->GetImageSource();
						const SpriteDefinitionList& sprite_definitions = spritesheet_property_parser->GetSpriteDefinitions();
						const float image_resolution_factor = spritesheet_property_parser->GetImageResolutionFactor();
						
						if (sprite_definitions.empty())
						{
							Log::Message(Log::LT_WARNING, "Spritesheet '%s' has no sprites defined, ignored. At %s:%d", at_rule_name.c_str(), stream_file_name.c_str(), line_number);
						}
						else if (image_source.empty())
						{
							Log::Message(Log::LT_WARNING, "No image source (property 'src') specified for spritesheet '%s'. At %s:%d", at_rule_name.c_str(), stream_file_name.c_str(), line_number);
						}
						else if (image_resolution_factor <= 0.0f || image_resolution_factor >= 100.f)
						{
							Log::Message(Log::LT_WARNING, "Spritesheet resolution (property 'resolution') value must be larger than 0.0 and smaller than 100.0, given %g. In spritesheet '%s'. At %s:%d", image_resolution_factor, at_rule_name.c_str(), stream_file_name.c_str(), line_number);
						}
						else
						{
							const float display_scale = 1.0f / image_resolution_factor;
							current_block.stylesheet->spritesheet_list.AddSpriteSheet(at_rule_name, image_source, stream_file_name, (int)line_number, display_scale, sprite_definitions);
						}

						spritesheet_property_parser->Clear();
						at_rule_name.clear();
						state = State::Global;
					}
					else if (at_rule_identifier == "media")
					{
						// complete the current "global" block if present and start a new block
						if (current_block.stylesheet)
						{
							PostprocessKeyframes(current_block.stylesheet->keyframes);
							current_block.stylesheet->specificity_offset = rule_count;
							style_sheets.push_back(std::move(current_block));
							current_block = {};
						}

						// parse media query list into block
						PropertyDictionary feature_map;
						ParseMediaFeatureMap(feature_map, at_rule_name);
						current_block = {std::move(feature_map), UniquePtr<StyleSheet>(new StyleSheet())};

						inside_media_block = true;
						state = State::Global;
					}
					else
					{
						// Invalid identifier, should ignore
						at_rule_name.clear();
						state = State::Global;
						Log::Message(Log::LT_WARNING, "Invalid at-rule identifier '%s' found in stylesheet at %s:%d", at_rule_identifier.c_str(), stream_file_name.c_str(), line_number);
					}

				}
				else
				{
					Log::Message(Log::LT_WARNING, "Invalid character '%c' found while parsing at-rule identifier in stylesheet at %s:%d", token, stream_file_name.c_str(), line_number);
					state = State::Invalid;
				}
			}
			break;
			case State::KeyframeBlock:
			{
				if (token == '{')
				{	
					// Initialize current block if not present
					if (!current_block.stylesheet)
					{
						current_block = {PropertyDictionary{}, UniquePtr<StyleSheet>(new StyleSheet())};
					}

					// Each keyframe in keyframes has its own block which is processed here
					PropertyDictionary properties;
					PropertySpecificationParser parser(properties, StyleSheetSpecification::GetPropertySpecification());
					if(!ReadProperties(parser))
						continue;

					if (!ParseKeyframeBlock(current_block.stylesheet->keyframes, at_rule_name, pre_token_str, properties))
						continue;
				}
				else if (token == '}')
				{
					at_rule_name.clear();
					state = State::Global;
				}
				else
				{
					Log::Message(Log::LT_WARNING, "Invalid character '%c' found while parsing keyframe block in stylesheet at %s:%d", token, stream_file_name.c_str(), line_number);
					state = State::Invalid;
				}
			}
			break;
			default:
				RMLUI_ERROR;
				state = State::Invalid;
				break;
			}

			if (state == State::Invalid)
				break;
		}

		if (state == State::Invalid)
			break;
	}	

	// Complete last block if present
	if (current_block.stylesheet)
	{
		PostprocessKeyframes(current_block.stylesheet->keyframes);
		current_block.stylesheet->specificity_offset = rule_count;
		style_sheets.push_back(std::move(current_block));
	}

	return !style_sheets.empty();
}

bool StyleSheetParser::ParseProperties(PropertyDictionary& parsed_properties, const String& properties)
{
	RMLUI_ASSERT(!stream);
	StreamMemory stream_owner((const byte*)properties.c_str(), properties.size());
	stream = &stream_owner;
	PropertySpecificationParser parser(parsed_properties, StyleSheetSpecification::GetPropertySpecification());
	bool success = ReadProperties(parser);
	stream = nullptr;
	return success;
}

StyleSheetNodeListRaw StyleSheetParser::ConstructNodes(StyleSheetNode& root_node, const String& selectors)
{
	const PropertyDictionary empty_properties;

	StringList selector_list;
	StringUtilities::ExpandString(selector_list, selectors);

	StyleSheetNodeListRaw leaf_nodes;

	for (const String& selector : selector_list)
	{
		StyleSheetNode* leaf_node = ImportProperties(&root_node, selector, empty_properties, 0);

		if (leaf_node != &root_node)
			leaf_nodes.push_back(leaf_node);
	}

	return leaf_nodes;
}

bool StyleSheetParser::ReadProperties(AbstractPropertyParser& property_parser)
{
	RMLUI_ZoneScoped;

	String name;
	String value;

	enum ParseState { NAME, VALUE, QUOTE };
	ParseState state = NAME;

	char character;
	char previous_character = 0;
	while (ReadCharacter(character))
	{
		parse_buffer_pos++;

		switch (state)
		{
			case NAME:
			{
				if (character == ';')
				{
					name = StringUtilities::StripWhitespace(name);
					if (!name.empty())
					{
						Log::Message(Log::LT_WARNING, "Found name with no value while parsing property declaration '%s' at %s:%d", name.c_str(), stream_file_name.c_str(), line_number);
						name.clear();
					}
				}
				else if (character == '}')
				{
					name = StringUtilities::StripWhitespace(name);
					if (!name.empty())
						Log::Message(Log::LT_WARNING, "End of rule encountered while parsing property declaration '%s' at %s:%d", name.c_str(), stream_file_name.c_str(), line_number);
					return true;
				}
				else if (character == ':')
				{
					name = StringUtilities::StripWhitespace(name);
					state = VALUE;
				}
				else
					name += character;
			}
			break;
			
			case VALUE:
			{
				if (character == ';')
				{
					value = StringUtilities::StripWhitespace(value);

					if (!property_parser.Parse(name, value))
						Log::Message(Log::LT_WARNING, "Syntax error parsing property declaration '%s: %s;' in %s: %d.", name.c_str(), value.c_str(), stream_file_name.c_str(), line_number);

					name.clear();
					value.clear();
					state = NAME;
				}
				else if (character == '}')
				{
					break;
				}
				else
				{
					value += character;
					if (character == '"')
						state = QUOTE;
				}
			}
			break;

			case QUOTE:
			{
				value += character;
				if (character == '"' && previous_character != '/')
					state = VALUE;
			}
			break;
		}

		if (character == '}')
			break;
		previous_character = character;
	}

	if (state == VALUE && !name.empty() && !value.empty())
	{
		value = StringUtilities::StripWhitespace(value);

		if (!property_parser.Parse(name, value))
			Log::Message(Log::LT_WARNING, "Syntax error parsing property declaration '%s: %s;' in %s: %d.", name.c_str(), value.c_str(), stream_file_name.c_str(), line_number);
	}
	else if (!name.empty() || !value.empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid property declaration '%s':'%s' at %s:%d", name.c_str(), value.c_str(), stream_file_name.c_str(), line_number);
	}
	
	return true;
}

StyleSheetNode* StyleSheetParser::ImportProperties(StyleSheetNode* node, String rule_name, const PropertyDictionary& properties, int rule_specificity)
{
	StyleSheetNode* leaf_node = node;

	StringList nodes;

	// Find child combinators, the RCSS '>' rule.
	size_t i_child = rule_name.find('>');
	while (i_child != String::npos)
	{
		// So we found one! Next, we want to format the rule such that the '>' is located at the 
		// end of the left-hand-side node, and that there is a space to the right-hand-side. This ensures that
		// the selector is applied to the "parent", and that parent and child are expanded properly below.
		size_t i_begin = i_child;
		while (i_begin > 0 && rule_name[i_begin - 1] == ' ')
			i_begin--;

		const size_t i_end = i_child + 1;
		rule_name.replace(i_begin, i_end - i_begin, "> ");
		i_child = rule_name.find('>', i_begin + 1);
	}

	// Expand each individual node separated by spaces. Don't expand inside parenthesis because of structural selectors.
	StringUtilities::ExpandString(nodes, rule_name, ' ', '(', ')', true);

	// Create each node going down the tree
	for (size_t i = 0; i < nodes.size(); i++)
	{
		const String& name = nodes[i];

		String tag;
		String id;
		StringList classes;
		ElementAttributes attributes;
		StringList pseudo_classes;
		StructuralSelectorList structural_pseudo_classes;
		bool child_combinator = false;

		size_t index = 0;
		while (index < name.size())
		{
			size_t start_index = index;
			size_t end_index = index + 1;

			// Read until we hit the next identifier.
			while (end_index < name.size() &&
				   name[end_index] != '#' &&
				   name[end_index] != '.' &&
				   name[end_index] != ':' &&
				   name[end_index] != '>' &&
				   name[end_index] != '[')
				end_index++;

			String identifier = name.substr(start_index, end_index - start_index);
			if (!identifier.empty())
			{
				switch (identifier[0])
				{
					case '#':	id = identifier.substr(1); break;
					case '.':	classes.push_back(identifier.substr(1)); break;
					case '[':
					{
						const size_t attribute_end = identifier.find(']');
						if(attribute_end != String::npos){
						    String attribute_str = identifier.substr(1, attribute_end - 1);
						    attribute_str = StringUtilities::Replace(attribute_str, "\"", "");
							
							if(!attribute_str.empty()) //has attribute selector data
							{
								const size_t equals_index = attribute_str.find('=');
								if(equals_index != String::npos)
								{
									String attribute_name;
									String attribute_value = attribute_str.substr(equals_index-1);
									switch(attribute_value[0])
									{
										case '~':
										case '|': 
										case '^': 
										case '$': 
										case '*':
											attribute_name = attribute_str.substr(0, equals_index -1);
											break;
										default:
											attribute_name = attribute_str.substr(0, equals_index);
											attribute_value[0] = '=';
											break;
									}
									attributes[attribute_name] = attribute_value;
									break;
								}
							}
							attributes[attribute_str] = "";
							break;
						}
					}
					break;
					case ':':
						{
						String pseudo_class_name = identifier.substr(1);
						StructuralSelector node_selector = StyleSheetFactory::GetSelector(pseudo_class_name);
						if (node_selector.selector)
							structural_pseudo_classes.push_back(node_selector);
						else
							pseudo_classes.push_back(pseudo_class_name);
					}
					break;
					case '>':	child_combinator = true; break;

					default:	if(identifier != "*") tag = identifier;
				}
			}

			index = end_index;
		}

		// Sort the classes and pseudo-classes so they are consistent across equivalent declarations that shuffle the order around.
		std::sort(classes.begin(), classes.end());
		//std::sort(attributes.begin(), attributes.end());
		std::sort(pseudo_classes.begin(), pseudo_classes.end());
		std::sort(structural_pseudo_classes.begin(), structural_pseudo_classes.end());

		// Get the named child node.
		leaf_node = leaf_node->GetOrCreateChildNode(std::move(tag), std::move(id), std::move(classes), std::move(attributes), std::move(pseudo_classes), std::move(structural_pseudo_classes), child_combinator);
	}

	// Merge the new properties with those already on the leaf node.
	leaf_node->ImportProperties(properties, rule_specificity);

	return leaf_node;
}

char StyleSheetParser::FindToken(String& buffer, const char* tokens, bool remove_token)
{
	buffer.clear();
	char character;
	while (ReadCharacter(character))
	{
		if (strchr(tokens, character) != nullptr)
		{
			if (remove_token)
				parse_buffer_pos++;
			return character;
		}
		else
		{
			buffer += character;
			parse_buffer_pos++;
		}
	}

	return 0;
}

// Attempts to find the next character in the active stream.
bool StyleSheetParser::ReadCharacter(char& buffer)
{
	bool comment = false;

	// Continuously fill the buffer until either we run out of
	// stream or we find the requested token
	do
	{
		while (parse_buffer_pos < parse_buffer.size())
		{
			if (parse_buffer[parse_buffer_pos] == '\n')
				line_number++;
			else if (comment)
			{
				// Check for closing comment
				if (parse_buffer[parse_buffer_pos] == '*')
				{
					parse_buffer_pos++;
					if (parse_buffer_pos >= parse_buffer.size())
					{
						if (!FillBuffer())
							return false;
					}

					if (parse_buffer[parse_buffer_pos] == '/')
						comment = false;
				}
			}
			else
			{
				// Check for an opening comment
				if (parse_buffer[parse_buffer_pos] == '/')
				{
					parse_buffer_pos++;
					if (parse_buffer_pos >= parse_buffer.size())
					{
						if (!FillBuffer())
						{
							buffer = '/';
							parse_buffer = "/";
							return true;
						}
					}
					
					if (parse_buffer[parse_buffer_pos] == '*')
						comment = true;
					else
					{
						buffer = '/';
						if (parse_buffer_pos == 0)
							parse_buffer.insert(parse_buffer_pos, 1, '/');
						else
							parse_buffer_pos--;
						return true;
					}
				}

				if (!comment)
				{
					// If we find a character, return it
					buffer = parse_buffer[parse_buffer_pos];
					return true;
				}
			}

			parse_buffer_pos++;
		}
	}
	while (FillBuffer());

	return false;
}

// Fills the internal buffer with more content
bool StyleSheetParser::FillBuffer()
{
	// If theres no data to process, abort
	if (stream->IsEOS())
		return false;

	// Read in some data (4092 instead of 4096 to avoid the buffer growing when we have to add back
	// a character after a failed comment parse.)
	parse_buffer.clear();
	bool read = stream->Read(parse_buffer, 4092) > 0;
	parse_buffer_pos = 0;

	return read;
}

} // namespace Rml
