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

#ifndef RMLUICOREELEMENTTEXTDEFAULT_H
#define RMLUICOREELEMENTTEXTDEFAULT_H

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/Geometry.h"

namespace Rml {
namespace Core {

/**
	@author Peter Curry
 */

class RMLUICORE_API ElementTextDefault : public ElementText
{
public:
	RMLUI_RTTI_DefineWithParent(ElementTextDefault, ElementText)

	ElementTextDefault(const String& tag);
	virtual ~ElementTextDefault();

	void SetText(const String& text) override;
	const String& GetText() const override;

	void OnRender() override;

	/// Generates a token of text from this element, returning only the width.
	/// @param[out] token_width The window (in pixels) of the token.
	/// @param[in] token_begin The first character to be included in the token.
	/// @return True if the token is the end of the element's text, false if not.
	bool GenerateToken(float& token_width, int token_begin) override;
	/// Generates a line of text rendered from this element
	/// @param[out] line The characters making up the line, with white-space characters collapsed and endlines processed appropriately.
	/// @param[out] line_length The number of characters from the source string consumed making up this string; this may very well be different from line.size()!
	/// @param[out] line_width The width (in pixels) of the generated line.
	/// @param[in] line_begin The first character to be rendered in the line.
	/// @param[in] maximum_line_width The width (in pixels) of space allowed for the line, or -1 for unlimited space.
	/// @param[in] right_spacing_width The width (in pixels) of the spacing (consisting of margins, padding, etc) that must be remaining on the right of the line if the last of the text is rendered onto this line.
	/// @param[in] trim_whitespace_prefix If we're collapsing whitespace, whether or remove all prefixing whitespace or collapse it down to a single space.
	/// @param[in] decode_escape_characters Decode escaped characters such as &amp; into &.
	/// @return True if the line reached the end of the element's text, false if not.
	bool GenerateLine(String& line, int& line_length, float& line_width, int line_begin, float maximum_line_width, float right_spacing_width, bool trim_whitespace_prefix, bool decode_escape_characters) override;

	/// Clears all lines of generated text and prepares the element for generating new lines.
	void ClearLines() override;
	/// Adds a new line into the text element.
	/// @param[in] line_position The position of this line, as an offset from the first line.
	/// @param[in] line The contents of the line..
	void AddLine(const Vector2f& line_position, const String& line) override;

	/// Prevents the element from dirtying its document's layout when its text is changed.
	void SuppressAutoLayout() override;

protected:
	void OnPropertyChange(const PropertyIdSet& properties) override;

	/// Returns the RML of this element
	/// @param content[out] The raw text.
	void GetRML(String& content) override;

private:
	// Prepares the font effects this element uses for its font.
	bool UpdateFontEffects();

	// Used to store the position and length of each line we have geometry for.
	struct Line
	{
		Line(const String& text, const Vector2f& position) : text(text), position(position), width(0) {}
		String text;
		Vector2f position;
		int width;
	};

	// Clears and regenerates all of the text's geometry.
	void GenerateGeometry(const FontFaceHandle font_face_handle);
	// Generates the geometry for a single line of text.
	void GenerateGeometry(const FontFaceHandle font_face_handle, Line& line);
	// Generates any geometry necessary for rendering a line decoration (underline, strike-through, etc).
	void GenerateLineDecoration(const FontFaceHandle font_face_handle, const Line& line);

	String text;

	typedef std::vector< Line > LineList;
	LineList lines;

	bool dirty_layout_on_change;

	GeometryList geometry;
	bool geometry_dirty;

	Colourb colour;

	// The decoration geometry we've generated for this string.
	Geometry decoration;
	// What the decoration type is that we have generated.
	Style::TextDecoration generated_decoration;
	// What the element's actual text-decoration property is; this may be different from the generated decoration
	// if it is set to none; this means we can keep generated decoration and simply toggle it on or off as long as
	// it isn't being changed.
	Style::TextDecoration decoration_property;

	FontEffectsHandle font_effects_handle;
	bool font_effects_dirty;

	int font_handle_version;
};

}
}

#endif
