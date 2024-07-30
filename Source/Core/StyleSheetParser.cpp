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

#include "StyleSheetParser.h"
#include "../../Include/RmlUi/Core/Decorator.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/PropertyDefinition.h"
#include "../../Include/RmlUi/Core/PropertySpecification.h"
#include "../../Include/RmlUi/Core/StreamMemory.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"
#include "../../Include/RmlUi/Core/StyleSheetContainer.h"
#include "../../Include/RmlUi/Core/StyleSheetSpecification.h"
#include "ComputeProperty.h"
#include "StyleSheetFactory.h"
#include "StyleSheetNode.h"
#include <algorithm>
#include <string.h>

namespace Rml {

class AbstractPropertyParser : NonCopyMoveable {
protected:
	~AbstractPropertyParser() = default;

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
	PropertySpecificationParser(PropertyDictionary& properties, const PropertySpecification& specification) :
		properties(properties), specification(specification)
	{}

	bool Parse(const String& name, const String& value) override { return specification.ParsePropertyDeclaration(properties, name, value); }
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
	PropertyId id_src, id_rx, id_ry, id_rw, id_rh, id_resolution;
	ShorthandId id_rectangle;

public:
	SpritesheetPropertyParser() : specification(4, 1)
	{
		id_src = specification.RegisterProperty("src", "", false, false).AddParser("string").GetId();
		id_rx = specification.RegisterProperty("rectangle-x", "", false, false).AddParser("length").GetId();
		id_ry = specification.RegisterProperty("rectangle-y", "", false, false).AddParser("length").GetId();
		id_rw = specification.RegisterProperty("rectangle-w", "", false, false).AddParser("length").GetId();
		id_rh = specification.RegisterProperty("rectangle-h", "", false, false).AddParser("length").GetId();
		id_rectangle = specification.RegisterShorthand("rectangle", "rectangle-x, rectangle-y, rectangle-w, rectangle-h", ShorthandType::FallThrough);
		id_resolution = specification.RegisterProperty("resolution", "", false, false).AddParser("resolution").GetId();
	}

	const String& GetImageSource() const { return image_source; }
	const SpriteDefinitionList& GetSpriteDefinitions() const { return sprite_definitions; }
	float GetImageResolutionFactor() const { return image_resolution_factor; }

	void Clear()
	{
		image_resolution_factor = 1.f;
		image_source.clear();
		sprite_definitions.clear();
	}

	bool Parse(const String& name, const String& value) override
	{
		if (name == "src")
		{
			if (!specification.ParsePropertyDeclaration(properties, id_src, value))
				return false;

			if (const Property* property = properties.GetProperty(id_src))
			{
				if (property->unit == Unit::STRING)
					image_source = property->Get<String>();
			}
		}
		else if (name == "resolution")
		{
			if (!specification.ParsePropertyDeclaration(properties, id_resolution, value))
				return false;

			if (const Property* property = properties.GetProperty(id_resolution))
			{
				if (property->unit == Unit::X)
					image_resolution_factor = property->Get<float>();
			}
		}
		else
		{
			if (!specification.ParseShorthandDeclaration(properties, id_rectangle, value))
				return false;

			Vector2f position, size;
			if (auto p = properties.GetProperty(id_rx))
				position.x = p->Get<float>();
			if (auto p = properties.GetProperty(id_ry))
				position.y = p->Get<float>();
			if (auto p = properties.GetProperty(id_rw))
				size.x = p->Get<float>();
			if (auto p = properties.GetProperty(id_rh))
				size.y = p->Get<float>();

			sprite_definitions.emplace_back(name, Rectanglef::FromPositionSize(position, size));
		}

		return true;
	}
};

static UniquePtr<SpritesheetPropertyParser> spritesheet_property_parser;

/*
 * Media queries need a special parser because they have unique properties that
 * aren't admissible in other property declaration contexts.
 */
class MediaQueryPropertyParser final : public AbstractPropertyParser {
private:
	// The dictionary to store the properties in.
	PropertyDictionary* properties = nullptr;
	PropertySpecification specification;

	static PropertyId CastId(MediaQueryId id) { return static_cast<PropertyId>(id); }

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

		specification.RegisterProperty("orientation", "", false, false, CastId(MediaQueryId::Orientation))
			.AddParser("keyword", "landscape, portrait");

		specification.RegisterProperty("theme", "", false, false, CastId(MediaQueryId::Theme)).AddParser("string");
	}

	void SetTargetProperties(PropertyDictionary* _properties) { properties = _properties; }

	void Clear() { properties = nullptr; }

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

StyleSheetParser::~StyleSheetParser() {}

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
		bool valid = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '-') || (c == '_'));
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
		if (blocks.size() > 0)
			property_ids.reserve(blocks.size() * blocks[0].properties.GetNumProperties());
		for (auto& block : blocks)
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

bool StyleSheetParser::ParseKeyframeBlock(KeyframesMap& keyframes_map, const String& identifier, const String& rules,
	const PropertyDictionary& properties)
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
		rule = StringUtilities::ToLower(std::move(rule));
		if (rule == "from")
			rule_values.push_back(0.0f);
		else if (rule == "to")
			rule_values.push_back(1.0f);
		else if (sscanf(rule.c_str(), "%f%%%n", &value, &count) == 1)
			if (count > 0 && value >= 0.0f && value <= 100.0f)
				rule_values.push_back(0.01f * value);
	}

	if (rule_values.empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid keyframes rule(s) '%s' at %s:%d", rules.c_str(), stream_file_name.c_str(), line_number);
		return false;
	}

	Keyframes& keyframes = keyframes_map[identifier];

	for (float selector : rule_values)
	{
		auto it = std::find_if(keyframes.blocks.begin(), keyframes.blocks.end(),
			[selector](const KeyframeBlock& keyframe_block) { return Math::Absolute(keyframe_block.normalized_time - selector) < 0.0001f; });
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

bool StyleSheetParser::ParseDecoratorBlock(const String& at_name, NamedDecoratorMap& named_decorator_map,
	const SharedPtr<const PropertySource>& source)
{
	StringList name_type;
	StringUtilities::ExpandString(name_type, at_name, ':');

	if (name_type.size() != 2 || name_type[0].empty() || name_type[1].empty())
	{
		Log::Message(Log::LT_WARNING, "Decorator syntax error at %s:%d. Use syntax: '@decorator name : type { ... }'.", stream_file_name.c_str(),
			line_number);
		return false;
	}

	const String& name = name_type[0];
	String decorator_type = name_type[1];

	auto it_find = named_decorator_map.find(name);
	if (it_find != named_decorator_map.end())
	{
		Log::Message(Log::LT_WARNING, "Decorator with name '%s' already declared, ignoring decorator at %s:%d.", name.c_str(),
			stream_file_name.c_str(), line_number);
		return false;
	}

	// Get the instancer associated with the decorator type
	DecoratorInstancer* decorator_instancer = Factory::GetDecoratorInstancer(decorator_type);
	PropertyDictionary properties;

	if (!decorator_instancer)
	{
		// Type is not a declared decorator type, instead, see if it is another decorator name, then we inherit its properties.
		auto it = named_decorator_map.find(decorator_type);
		if (it != named_decorator_map.end())
		{
			// Yes, try to retrieve the instancer from the parent type, and add its property values.
			decorator_instancer = Factory::GetDecoratorInstancer(it->second.type);
			properties = it->second.properties;
			decorator_type = it->second.type;
		}

		// If we still don't have an instancer, we cannot continue.
		if (!decorator_instancer)
		{
			Log::Message(Log::LT_WARNING, "Invalid decorator type '%s' declared at %s:%d.", decorator_type.c_str(), stream_file_name.c_str(),
				line_number);
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

	named_decorator_map.emplace(name, NamedDecorator{std::move(decorator_type), decorator_instancer, std::move(properties)});

	return true;
}

bool StyleSheetParser::ParseMediaFeatureMap(const String& rules, PropertyDictionary& properties, MediaQueryModifier& modifier)
{
	media_query_property_parser->SetTargetProperties(&properties);

	enum ParseState { Global, Name, Value };
	ParseState state = Global;

	char character = 0;

	String name;

	String current_string;

	modifier = MediaQueryModifier::None;

	for (size_t cursor = 0; cursor < rules.length(); cursor++)
	{
		character = rules[cursor];

		switch (character)
		{
		case ' ':
		{
			if (state == Global)
			{
				current_string = StringUtilities::StripWhitespace(StringUtilities::ToLower(std::move(current_string)));

				if (current_string == "not")
				{
					// we can only ever see one "not" on the entire global query.
					if (modifier != MediaQueryModifier::None)
					{
						Log::Message(Log::LT_WARNING, "Unexpected '%s' in @media query list at %s:%d.", current_string.c_str(), stream_file_name.c_str(), line_number);
						return false;
					}

					modifier = MediaQueryModifier::Not;
					current_string.clear();
				}
			}

			break;
		}
		case '(':
		{
			if (state != Global)
			{
				Log::Message(Log::LT_WARNING, "Unexpected '(' in @media query list at %s:%d.", stream_file_name.c_str(), line_number);
				return false;
			}

			current_string = StringUtilities::StripWhitespace(StringUtilities::ToLower(std::move(current_string)));

			// allow an empty string to pass through only if we had just parsed a modifier.
			if (current_string != "and" &&
				(properties.GetNumProperties() != 0 || !current_string.empty()))
			{
				Log::Message(Log::LT_WARNING, "Unexpected '%s' in @media query list at %s:%d. Expected 'and'.", current_string.c_str(),
					stream_file_name.c_str(), line_number);
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

			if (!media_query_property_parser->Parse(name, current_string))
				Log::Message(Log::LT_WARNING, "Syntax error parsing media-query property declaration '%s: %s;' in %s: %d.", name.c_str(),
					current_string.c_str(), stream_file_name.c_str(), line_number);

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

			current_string = StringUtilities::StripWhitespace(StringUtilities::ToLower(std::move(current_string)));

			if (!IsValidIdentifier(current_string))
			{
				Log::Message(Log::LT_WARNING, "Malformed property name '%s' in @media query list at %s:%d.", current_string.c_str(),
					stream_file_name.c_str(), line_number);
				return false;
			}

			name = current_string;
			current_string.clear();

			state = Value;
		}
		break;
		default: current_string += character;
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
						current_block = MediaBlock{PropertyDictionary{}, UniquePtr<StyleSheet>(new StyleSheet()), MediaQueryModifier::None};
					}

					const int rule_line_number = line_number;

					// Read the attributes
					PropertyDictionary properties;
					PropertySpecificationParser parser(properties, StyleSheetSpecification::GetPropertySpecification());
					if (!ReadProperties(parser))
						continue;

					StringList rule_name_list;
					StringUtilities::ExpandString(rule_name_list, pre_token_str, ',', '(', ')');

					// Add style nodes to the root of the tree
					for (size_t i = 0; i < rule_name_list.size(); i++)
					{
						auto source = MakeShared<PropertySource>(stream_file_name, rule_line_number, rule_name_list[i]);
						properties.SetSourceOfAllProperties(source);
						if (!ImportProperties(current_block.stylesheet->root.get(), rule_name_list[i], properties, rule_count))
						{
							Log::Message(Log::LT_WARNING, "Invalid selector '%s' encountered while parsing stylesheet at %s:%d.",
								rule_name_list[i].c_str(), stream_file_name.c_str(), line_number);
						}
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
					Log::Message(Log::LT_WARNING, "Invalid character '%c' found while parsing stylesheet at %s:%d. Trying to proceed.", token,
						stream_file_name.c_str(), line_number);
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
						current_block = {PropertyDictionary{}, UniquePtr<StyleSheet>(new StyleSheet()), MediaQueryModifier::None};
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
						ParseDecoratorBlock(at_rule_name, current_block.stylesheet->named_decorator_map, source);

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
							Log::Message(Log::LT_WARNING, "Spritesheet '%s' has no sprites defined, ignored. At %s:%d", at_rule_name.c_str(),
								stream_file_name.c_str(), line_number);
						}
						else if (image_source.empty())
						{
							Log::Message(Log::LT_WARNING, "No image source (property 'src') specified for spritesheet '%s'. At %s:%d",
								at_rule_name.c_str(), stream_file_name.c_str(), line_number);
						}
						else if (image_resolution_factor <= 0.0f || image_resolution_factor >= 100.f)
						{
							Log::Message(Log::LT_WARNING,
								"Spritesheet resolution (property 'resolution') value must be larger than 0.0 and smaller than 100.0, given %g. In "
								"spritesheet '%s'. At %s:%d",
								image_resolution_factor, at_rule_name.c_str(), stream_file_name.c_str(), line_number);
						}
						else
						{
							const float display_scale = 1.0f / image_resolution_factor;
							current_block.stylesheet->spritesheet_list.AddSpriteSheet(at_rule_name, image_source, stream_file_name, (int)line_number,
								display_scale, sprite_definitions);
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
						MediaQueryModifier modifier;
						ParseMediaFeatureMap(at_rule_name, feature_map, modifier);
						current_block = {std::move(feature_map), UniquePtr<StyleSheet>(new StyleSheet()), modifier};

						inside_media_block = true;
						state = State::Global;
					}
					else
					{
						// Invalid identifier, should ignore
						at_rule_name.clear();
						state = State::Global;
						Log::Message(Log::LT_WARNING, "Invalid at-rule identifier '%s' found in stylesheet at %s:%d", at_rule_identifier.c_str(),
							stream_file_name.c_str(), line_number);
					}
				}
				else
				{
					Log::Message(Log::LT_WARNING, "Invalid character '%c' found while parsing at-rule identifier in stylesheet at %s:%d", token,
						stream_file_name.c_str(), line_number);
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
						current_block = {PropertyDictionary{}, UniquePtr<StyleSheet>(new StyleSheet()), MediaQueryModifier::None};
					}

					// Each keyframe in keyframes has its own block which is processed here
					PropertyDictionary properties;
					PropertySpecificationParser parser(properties, StyleSheetSpecification::GetPropertySpecification());
					if (!ReadProperties(parser))
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
					Log::Message(Log::LT_WARNING, "Invalid character '%c' found while parsing keyframe block in stylesheet at %s:%d", token,
						stream_file_name.c_str(), line_number);
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
	StringUtilities::ExpandString(selector_list, selectors, ',', '(', ')');

	StyleSheetNodeListRaw leaf_nodes;

	for (const String& selector : selector_list)
	{
		StyleSheetNode* leaf_node = ImportProperties(&root_node, selector, empty_properties, 0);

		if (!leaf_node)
			Log::Message(Log::LT_WARNING, "Invalid selector '%s' encountered.", selector.c_str());
		else if (leaf_node != &root_node)
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
					Log::Message(Log::LT_WARNING, "Found name with no value while parsing property declaration '%s' at %s:%d", name.c_str(),
						stream_file_name.c_str(), line_number);
					name.clear();
				}
			}
			else if (character == '}')
			{
				name = StringUtilities::StripWhitespace(name);
				if (!name.empty())
					Log::Message(Log::LT_WARNING, "End of rule encountered while parsing property declaration '%s' at %s:%d", name.c_str(),
						stream_file_name.c_str(), line_number);
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
					Log::Message(Log::LT_WARNING, "Syntax error parsing property declaration '%s: %s;' in %s: %d.", name.c_str(), value.c_str(),
						stream_file_name.c_str(), line_number);

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
			if (character == '"' && previous_character != '\\')
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
			Log::Message(Log::LT_WARNING, "Syntax error parsing property declaration '%s: %s;' in %s: %d.", name.c_str(), value.c_str(),
				stream_file_name.c_str(), line_number);
	}
	else if (!StringUtilities::StripWhitespace(name).empty() || !value.empty())
	{
		Log::Message(Log::LT_WARNING, "Invalid property declaration '%s':'%s' at %s:%d", name.c_str(), value.c_str(), stream_file_name.c_str(),
			line_number);
	}

	return true;
}

StyleSheetNode* StyleSheetParser::ImportProperties(StyleSheetNode* node, const String& rule, const PropertyDictionary& properties,
	int rule_specificity)
{
	StyleSheetNode* leaf_node = node;

	// Create each node going down the tree.
	for (size_t index = 0; index < rule.size();)
	{
		CompoundSelector selector;

		// Determine the combinator connecting the previous node if any.
		for (; index > 0 && index < rule.size(); index++)
		{
			bool reached_end_of_combinators = false;
			switch (rule[index])
			{
			case ' ': break;
			case '>': selector.combinator = SelectorCombinator::Child; break;
			case '+': selector.combinator = SelectorCombinator::NextSibling; break;
			case '~': selector.combinator = SelectorCombinator::SubsequentSibling; break;
			default: reached_end_of_combinators = true; break;
			}
			if (reached_end_of_combinators)
				break;
		}

		// Determine the node's requirements.
		while (index < rule.size())
		{
			size_t start_index = index;
			size_t end_index = index + 1;

			if (rule[start_index] == '*')
				start_index += 1;

			if (rule[start_index] == '[')
			{
				end_index = rule.find(']', start_index + 1);
				if (end_index == String::npos)
					return nullptr;
				end_index += 1;
			}
			else
			{
				int parenthesis_count = 0;

				// Read until we hit the next identifier. Don't match inside parenthesis in case of structural selectors.
				for (; end_index < rule.size(); end_index++)
				{
					static const String identifiers = "#.:[ >+~";
					if (parenthesis_count == 0 && identifiers.find(rule[end_index]) != String::npos)
						break;

					if (rule[end_index] == '(')
						parenthesis_count += 1;
					else if (rule[end_index] == ')')
						parenthesis_count -= 1;
				}
			}

			if (end_index > start_index)
			{
				const char* p_begin = rule.data() + start_index;
				const char* p_end = rule.data() + end_index;

				switch (rule[start_index])
				{
				case '#': selector.id = String(p_begin + 1, p_end); break;
				case '.': selector.class_names.push_back(String(p_begin + 1, p_end)); break;
				case ':':
				{
					String pseudo_class_name = String(p_begin + 1, p_end);
					StructuralSelector node_selector = StyleSheetFactory::GetSelector(pseudo_class_name);
					if (node_selector.type != StructuralSelectorType::Invalid)
						selector.structural_selectors.push_back(node_selector);
					else
						selector.pseudo_class_names.push_back(std::move(pseudo_class_name));
				}
				break;
				case '[':
				{
					const size_t i_attr_begin = start_index + 1;
					const size_t i_attr_end = end_index - 1;
					if (i_attr_end <= i_attr_begin)
						return nullptr;

					AttributeSelector attribute;

					static const String attribute_operators = "=~|^$*]";
					size_t i_cursor = Math::Min(static_cast<size_t>(rule.find_first_of(attribute_operators, i_attr_begin)), i_attr_end);
					attribute.name = rule.substr(i_attr_begin, i_cursor - i_attr_begin);

					if (i_cursor < i_attr_end)
					{
						const char c = rule[i_cursor];
						attribute.type = AttributeSelectorType(c);

						// Move cursor past operator. Non-'=' symbols are always followed by '=' so move two characters.
						i_cursor += (c == '=' ? 1 : 2);

						size_t i_value_end = i_attr_end;
						if (i_cursor < i_attr_end && (rule[i_cursor] == '"' || rule[i_cursor] == '\''))
						{
							i_cursor += 1;
							i_value_end -= 1;
						}

						if (i_cursor < i_value_end)
							attribute.value = rule.substr(i_cursor, i_value_end - i_cursor);
					}

					selector.attributes.push_back(std::move(attribute));
				}
				break;
				default: selector.tag = String(p_begin, p_end); break;
				}
			}

			index = end_index;

			// If we reached a combinator then we submit the current node and start fresh with a new node.
			static const String combinators(" >+~");
			if (combinators.find(rule[index]) != String::npos)
				break;
		}

		// Sort the classes and pseudo-classes so they are consistent across equivalent declarations that shuffle the order around.
		std::sort(selector.class_names.begin(), selector.class_names.end());
		std::sort(selector.attributes.begin(), selector.attributes.end());
		std::sort(selector.pseudo_class_names.begin(), selector.pseudo_class_names.end());
		std::sort(selector.structural_selectors.begin(), selector.structural_selectors.end());

		// Add the new child node, or retrieve the existing child if we have an exact match.
		leaf_node = leaf_node->GetOrCreateChildNode(std::move(selector));
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
	} while (FillBuffer());

	return false;
}

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
