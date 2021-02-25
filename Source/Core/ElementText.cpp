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

#include "../../Include/RmlUi/Core/ElementText.h"
#include "ElementDefinition.h"
#include "ElementStyle.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/Event.h"
#include "../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Property.h"
#include "../../Include/RmlUi/Core/Profiling.h"

namespace Rml {

static bool BuildToken(String& token, const char*& token_begin, const char* string_end, bool first_token, bool collapse_white_space, bool break_at_endline, Style::TextTransform text_transformation, bool decode_escape_characters);
static bool LastToken(const char* token_begin, const char* string_end, bool collapse_white_space, bool break_at_endline);

ElementText::ElementText(const String& tag) : Element(tag), colour(255, 255, 255), decoration(this)
{
	dirty_layout_on_change = true;

	generated_decoration = Style::TextDecoration::None;
	decoration_property = Style::TextDecoration::None;

	geometry_dirty = true;

	font_effects_handle = 0;
	font_effects_dirty = true;
	font_handle_version = 0;
}

ElementText::~ElementText()
{
}

void ElementText::SetText(const String& _text)
{
	if (text != _text)
	{
		text = _text;

		if (dirty_layout_on_change)
			DirtyLayout();
	}
}

const String& ElementText::GetText() const
{
	return text;
}

void ElementText::OnRender()
{
	RMLUI_ZoneScoped;

	FontFaceHandle font_face_handle = GetFontFaceHandle();
	if (font_face_handle == 0)
		return;
	
	// If our font effects have potentially changed, update it and force a geometry generation if necessary.
	if (font_effects_dirty && UpdateFontEffects())
		geometry_dirty = true;

	// Dirty geometry if font version has changed.
	int new_version = GetFontEngineInterface()->GetVersion(font_face_handle);
	if (new_version != font_handle_version)
	{
		font_handle_version = new_version;
		geometry_dirty = true;
	}

	// Regenerate the geometry if the colour or font configuration has altered.
	if (geometry_dirty)
		GenerateGeometry(font_face_handle);

	// Regenerate text decoration if necessary.
	if (decoration_property != generated_decoration)
	{
		decoration.Release(true);

		if (decoration_property != Style::TextDecoration::None)
			GenerateDecoration(font_face_handle);

		generated_decoration = decoration_property;
	}

	const Vector2f translation = GetAbsoluteOffset();
	
	bool render = true;
	Vector2i clip_origin;
	Vector2i clip_dimensions;
	if (GetContext()->GetActiveClipRegion(clip_origin, clip_dimensions))
	{
		float clip_top = (float)clip_origin.y;
		float clip_left = (float)clip_origin.x;
		float clip_right = (float)(clip_origin.x + clip_dimensions.x);
		float clip_bottom = (float)(clip_origin.y + clip_dimensions.y);
		float line_height = (float)GetFontEngineInterface()->GetLineHeight(GetFontFaceHandle());
		
		render = false;
		for (size_t i = 0; i < lines.size(); ++i)
		{			
			const Line& line = lines[i];
			float x = translation.x + line.position.x;
			float y = translation.y + line.position.y;
			
			bool render_line = !(x > clip_right);
			render_line = render_line && !(x + line.width < clip_left);
			
			render_line = render_line && !(y - line_height > clip_bottom);
			render_line = render_line && !(y < clip_top);
			
			if (render_line)
			{
				render = true;
				break;
			}
		}
	}
	
	if (render)
	{
		for (size_t i = 0; i < geometry.size(); ++i)
			geometry[i].Render(translation);
	}

	if (decoration_property != Style::TextDecoration::None)
		decoration.Render(translation);
}

// Generates a token of text from this element, returning only the width.
bool ElementText::GenerateToken(float& token_width, int line_begin)
{
	RMLUI_ZoneScoped;

	// Bail if we don't have a valid font face.
	FontFaceHandle font_face_handle = GetFontFaceHandle();
	if (font_face_handle == 0 ||
		line_begin >= (int) text.size())
		return 0;

	// Determine how we are processing white-space while formatting the text.
	using namespace Style;
	auto& computed = GetComputedValues();
	WhiteSpace white_space_property = computed.white_space;
	bool collapse_white_space = white_space_property == WhiteSpace::Normal ||
								white_space_property == WhiteSpace::Nowrap ||
								white_space_property == WhiteSpace::Preline;
	bool break_at_endline = white_space_property == WhiteSpace::Pre ||
							white_space_property == WhiteSpace::Prewrap ||
							white_space_property == WhiteSpace::Preline;

	const char* token_begin = text.c_str() + line_begin;
	String token;

	BuildToken(token, token_begin, text.c_str() + text.size(), true, collapse_white_space, break_at_endline, computed.text_transform, true);
	token_width = (float) GetFontEngineInterface()->GetStringWidth(font_face_handle, token);

	return LastToken(token_begin, text.c_str() + text.size(), collapse_white_space, break_at_endline);
}

// Generates a line of text rendered from this element
bool ElementText::GenerateLine(String& line, int& line_length, float& line_width, int line_begin, float maximum_line_width, float right_spacing_width, bool trim_whitespace_prefix, bool decode_escape_characters)
{
	RMLUI_ZoneScoped;

	FontFaceHandle font_face_handle = GetFontFaceHandle();

	// Initialise the output variables.
	line.clear();
	line_length = 0;
	line_width = 0;

	// Bail if we don't have a valid font face.
	if (font_face_handle == 0)
		return true;

	// Determine how we are processing white-space while formatting the text.
	using namespace Style;
	auto& computed = GetComputedValues();
	WhiteSpace white_space_property = computed.white_space;
	bool collapse_white_space = white_space_property == WhiteSpace::Normal ||
								white_space_property == WhiteSpace::Nowrap ||
								white_space_property == WhiteSpace::Preline;
	bool break_at_line = (maximum_line_width >= 0) && 
		                   (white_space_property == WhiteSpace::Normal ||
							white_space_property == WhiteSpace::Prewrap ||
							white_space_property == WhiteSpace::Preline);
	bool break_at_endline = white_space_property == WhiteSpace::Pre ||
							white_space_property == WhiteSpace::Prewrap ||
							white_space_property == WhiteSpace::Preline;

	TextTransform text_transform_property = computed.text_transform;
	WordBreak word_break = computed.word_break;

	FontEngineInterface* font_engine_interface = GetFontEngineInterface();

	// Starting at the line_begin character, we generate sections of the text (we'll call them tokens) depending on the
	// white-space parsing parameters. Each section is then appended to the line if it can fit. If not, or if an
	// endline is found (and we're processing them), then the line is ended. kthxbai!
	const char* token_begin = text.c_str() + line_begin;
	const char* string_end = text.c_str() + text.size();
	while (token_begin != string_end)
	{
		String token;
		const char* next_token_begin = token_begin;
		Character previous_codepoint = Character::Null;
		if (!line.empty())
			previous_codepoint = StringUtilities::ToCharacter(StringUtilities::SeekBackwardUTF8(&line.back(), line.data()));

		// Generate the next token and determine its pixel-length.
		bool break_line = BuildToken(token, next_token_begin, string_end, line.empty() && trim_whitespace_prefix, collapse_white_space, break_at_endline, text_transform_property, decode_escape_characters);
		int token_width = font_engine_interface->GetStringWidth(font_face_handle, token, previous_codepoint);

		// If we're breaking to fit a line box, check if the token can fit on the line before we add it.
		if (break_at_line)
		{
			const bool is_last_token = LastToken(next_token_begin, string_end, collapse_white_space, break_at_endline);
			int max_token_width = int(maximum_line_width - (is_last_token ? line_width + right_spacing_width : line_width));

			if (token_width > max_token_width)
			{
				if (word_break == WordBreak::BreakAll || (word_break == WordBreak::BreakWord && line.empty()))
				{
					// Try to break up the word
					max_token_width = int(maximum_line_width - line_width);
					const int token_max_size = int(next_token_begin - token_begin);
					bool force_loop_break_after_next = false;

					// @performance: Can be made much faster. Use string width heuristics and logarithmic search.
					for (int i = token_max_size - 1; i > 0; --i)
					{
						token.clear();
						next_token_begin = token_begin;
						const char* partial_string_end = StringUtilities::SeekBackwardUTF8(token_begin + i, token_begin);
						break_line = BuildToken(token, next_token_begin, partial_string_end, line.empty() && trim_whitespace_prefix, collapse_white_space, break_at_endline, text_transform_property, decode_escape_characters);
						token_width = font_engine_interface->GetStringWidth(font_face_handle, token, previous_codepoint);

						if (force_loop_break_after_next || token_width <= max_token_width)
						{
							break;
						}
						else if (next_token_begin == token_begin)
						{
							// This means the first character of the token doesn't fit. Let it overflow into the next line if we can.
							if (!line.empty())
								return false;

							// Not even the first character of the line fits. Go back to consume the first character even though it will overflow.
							i += 2;
							force_loop_break_after_next = true;
						}
					}

					break_line = true;
				}
				else if (!line.empty())
				{
					// Let the token overflow into the next line.
					return false;
				}
			}
		}

		// The token can fit on the end of the line, so add it onto the end and increment our width and length counters.
		line += token;
		line_length += (int)(next_token_begin - token_begin);
		line_width += token_width;

		// Break out of the loop if an endline was forced.
		if (break_line)
			return false;

		// Set the beginning of the next token.
		token_begin = next_token_begin;
	}

	return true;
}

// Clears all lines of generated text and prepares the element for generating new lines.
void ElementText::ClearLines()
{
	// Clear the rendering information.
	for (size_t i = 0; i < geometry.size(); ++i)
		geometry[i].Release(true);

	lines.clear();
	generated_decoration = Style::TextDecoration::None;
	decoration.Release(true);
}

// Adds a new line into the text element.
void ElementText::AddLine(Vector2f line_position, const String& line)
{
	FontFaceHandle font_face_handle = GetFontFaceHandle();

	if (font_face_handle == 0)
		return;

	if (font_effects_dirty)
		UpdateFontEffects();

	Vector2f baseline_position = line_position + Vector2f(0.0f, (float)GetFontEngineInterface()->GetLineHeight(font_face_handle) - GetFontEngineInterface()->GetBaseline(font_face_handle));
	lines.emplace_back(line, baseline_position);

	geometry_dirty = true;
}

// Prevents the element from dirtying its document's layout when its text is changed.
void ElementText::SuppressAutoLayout()
{
	dirty_layout_on_change = false;
}

void ElementText::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	RMLUI_ZoneScoped;

	Element::OnPropertyChange(changed_properties);

	bool colour_changed = false;
	bool font_face_changed = false;
	auto& computed = GetComputedValues();

	if (changed_properties.Contains(PropertyId::Color) ||
		changed_properties.Contains(PropertyId::Opacity))
	{
		// Fetch our (potentially) new colour.
		Colourb new_colour = computed.color;
		float opacity = computed.opacity;
		new_colour.alpha = byte(opacity * float(new_colour.alpha));
		colour_changed = colour != new_colour;
		if (colour_changed)
			colour = new_colour;
	}

	if (changed_properties.Contains(PropertyId::FontFamily) ||
		changed_properties.Contains(PropertyId::FontWeight) ||
		changed_properties.Contains(PropertyId::FontStyle) ||
		changed_properties.Contains(PropertyId::FontSize))
	{
		font_face_changed = true;

		geometry.clear();
		font_effects_dirty = true;
	}

	if (changed_properties.Contains(PropertyId::FontEffect))
	{
		font_effects_dirty = true;
	}

	if (changed_properties.Contains(PropertyId::TextDecoration))
	{
		decoration_property = computed.text_decoration;
	}

	if (font_face_changed)
	{
		// We have to let our document know we need to be regenerated.
		if (dirty_layout_on_change)
			DirtyLayout();
	}
	else if (colour_changed)
	{
		// Force the geometry to be regenerated.
		geometry_dirty = true;

		// Re-colour the decoration geometry.
		Vector< Vertex >& vertices = decoration.GetVertices();
		for (size_t i = 0; i < vertices.size(); ++i)
			vertices[i].colour = colour;

		decoration.Release();
	}
}

// Returns the RML of this element
void ElementText::GetRML(String& content)
{
	content += text;
}

// Updates the configuration this element uses for its font.
bool ElementText::UpdateFontEffects()
{
	RMLUI_ZoneScoped;

	if (GetFontFaceHandle() == 0)
		return false;

	font_effects_dirty = false;

	static const FontEffectList empty_font_effects;

	// Fetch the font-effect for this text element
	const FontEffectList* font_effects = &empty_font_effects;
	if (GetComputedValues().has_font_effect)
	{
		if (const Property* p = GetProperty(PropertyId::FontEffect))
			if (FontEffectsPtr effects = p->Get<FontEffectsPtr>())
				font_effects = &effects->list;
	}

	// Request a font layer configuration to match this set of effects. If this is different from
	// our old configuration, then return true to indicate we'll need to regenerate geometry.
	FontEffectsHandle new_font_effects_handle = GetFontEngineInterface()->PrepareFontEffects(GetFontFaceHandle(), *font_effects);
	if (new_font_effects_handle != font_effects_handle)
	{
		font_effects_handle = new_font_effects_handle;
		return true;
	}

	return false;
}

// Clears and regenerates all of the text's geometry.
void ElementText::GenerateGeometry(const FontFaceHandle font_face_handle)
{
	RMLUI_ZoneScopedC(0xD2691E);

	// Release the old geometry ...
	for (size_t i = 0; i < geometry.size(); ++i)
		geometry[i].Release(true);

	// ... and generate it all again!
	for (size_t i = 0; i < lines.size(); ++i)
		GenerateGeometry(font_face_handle, lines[i]);

	decoration.Release(true);
	generated_decoration = Style::TextDecoration::None;

	geometry_dirty = false;
}

void ElementText::GenerateGeometry(const FontFaceHandle font_face_handle, Line& line)
{
	line.width = GetFontEngineInterface()->GenerateString(font_face_handle, font_effects_handle, line.text, line.position, colour, geometry);
	for (size_t i = 0; i < geometry.size(); ++i)
		geometry[i].SetHostElement(this);
}

// Generates any geometry necessary for rendering a line decoration (underline, strike-through, etc).
void ElementText::GenerateDecoration(const FontFaceHandle font_face_handle)
{
	RMLUI_ZoneScopedC(0xA52A2A);
	
	for(const Line& line : lines)
		GeometryUtilities::GenerateLine(font_face_handle, &decoration, line.position, line.width, decoration_property, colour);
}

static bool BuildToken(String& token, const char*& token_begin, const char* string_end, bool first_token, bool collapse_white_space, bool break_at_endline, Style::TextTransform text_transformation, bool decode_escape_characters)
{
	RMLUI_ASSERT(token_begin != string_end);

	token.reserve(string_end - token_begin + token.size());

	// Check what the first character of the token is; all we need to know is if it is white-space or not.
	bool parsing_white_space = StringUtilities::IsWhitespace(*token_begin);

	// Loop through the string from the token's beginning until we find an end to the token. This can occur in various
	// places, depending on the white-space processing;
	//  - at the end of a section of non-white-space characters,
	//  - at the end of a section of white-space characters, if we're not collapsing white-space,
	//  - at an endline token, if we're breaking on endlines.
	while (token_begin != string_end)
	{
		bool force_non_whitespace = false;
		char character = *token_begin;

		const char* escape_begin = token_begin;

		// Check for an ampersand; if we find one, we've got an HTML escaped character.
		if (decode_escape_characters && character == '&')
		{
			// Find the terminating ';'.
			while (token_begin != string_end &&
				   *token_begin != ';')
				++token_begin;

			// If we couldn't find the ';', print the token like normal text.
			if (token_begin == string_end)
			{
				token_begin = escape_begin;
			}
			// We could find a ';', parse the escape code. If the escape code is recognised, set the parsed character
			// to the appropriate one. If it is a non-breaking space, prevent it being picked up as whitespace. If it
			// is not recognised, print the token like normal text.
			else
			{
				String escape_code(escape_begin + 1, token_begin);

				if (escape_code == "lt")
					character = '<';
				else if (escape_code == "gt")
					character = '>';
				else if (escape_code == "amp")
					character = '&';
				else if (escape_code == "quot")
					character = '"';
				else if (escape_code == "nbsp")
				{
					character = ' ';
					force_non_whitespace = true;
				}
				else
					token_begin = escape_begin;
			}
		}

		// Check for an endline token; if we're breaking on endlines and we find one, then return true to indicate a
		// forced break.
		if (break_at_endline &&
			character == '\n')
		{
			token += '\n';
			token_begin++;
			return true;
		}

		// If we've transitioned from white-space characters to non-white-space characters, or vice-versa, then check
		// if should terminate the token; if we're not collapsing white-space, then yes (as sections of white-space are
		// non-breaking), otherwise only if we've transitioned from characters to white-space.
		bool white_space = !force_non_whitespace && StringUtilities::IsWhitespace(character);
		if (white_space != parsing_white_space)
		{
			if (!collapse_white_space)
			{
				// Restore pointer to the beginning of the escaped token, if we processed an escape code.
				token_begin = escape_begin;
				return false;
			}

			// We're collapsing white-space; we only tokenise words, not white-space, so we're only done tokenising
			// once we've begun parsing non-white-space and then found white-space.
			if (!parsing_white_space)
			{
				// However, if we are the last non-whitespace character in the string, and there are trailing
				// whitespace characters after this token, then we need to append a single space to the end of this
				// token.
				if (token_begin != string_end &&
					LastToken(token_begin, string_end, collapse_white_space, break_at_endline))
					token += ' ';

				return false;
			}

			// We've transitioned from white-space to non-white-space, so we append a single white-space character.
			if (!first_token)
				token += ' ';

			parsing_white_space = false;
		}

		// If the current character is white-space, we'll append a space character to the token if we're not collapsing
		// sections of white-space.
		if (white_space)
		{
			if (!collapse_white_space)
				token += ' ';
		}
		else
		{
			if (text_transformation == Style::TextTransform::Uppercase)
			{
				if (character >= 'a' && character <= 'z')
					character += ('A' - 'a');
			}
			else if (text_transformation == Style::TextTransform::Lowercase)
			{
				if (character >= 'A' && character <= 'Z')
					character -= ('A' - 'a');
			}

			token += character;
		}

		++token_begin;
	}

	return false;
}

static bool LastToken(const char* token_begin, const char* string_end, bool collapse_white_space, bool break_at_endline)
{
	bool last_token = (token_begin == string_end);
	if (collapse_white_space &&
		!last_token)
	{
		last_token = true;
		const char* character = token_begin;

		while (character != string_end)
		{
			if (!StringUtilities::IsWhitespace(*character) ||
				(break_at_endline && *character == '\n'))
			{
				last_token = false;
				break;
			}

			character++;
		}
	}

	return last_token;
}

} // namespace Rml
