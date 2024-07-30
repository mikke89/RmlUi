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

#ifndef RMLUI_CORE_ELEMENTS_WIDGETTEXTINPUT_H
#define RMLUI_CORE_ELEMENTS_WIDGETTEXTINPUT_H

#include "../../../Include/RmlUi/Core/EventListener.h"
#include "../../../Include/RmlUi/Core/Geometry.h"
#include "../../../Include/RmlUi/Core/Vertex.h"
#include <float.h>

namespace Rml {

class ElementText;
class ElementFormControl;
class TextInputHandler;
class WidgetTextInputContext;

/**
    An abstract widget for editing and navigating around a text field.

    @author Peter Curry
 */

class WidgetTextInput : public EventListener {
public:
	WidgetTextInput(ElementFormControl* parent);
	virtual ~WidgetTextInput();

	/// Sets the value of the text field.
	/// @param[in] value The new value to set on the text field.
	/// @note The value will be sanitized and synchronized with the element's value attribute.
	void SetValue(String value);
	/// Returns the underlying text from the element's value attribute.
	String GetAttributeValue() const;

	/// Sets the maximum length (in characters) of this text field.
	/// @param[in] max_length The new maximum length of the text field. A number lower than zero will mean infinite characters.
	void SetMaxLength(int max_length);
	/// Returns the maximum length (in characters) of this text field.
	/// @return The maximum number of characters allowed in this text field.
	int GetMaxLength() const;
	/// Returns the current length (in characters) of this text field.
	int GetLength() const;

	/// Selects all text.
	void Select();
	/// Selects the text in the given character range.
	/// @param[in] selection_start The first character to be selected.
	/// @param[in] selection_end The first character *after* the selection.
	void SetSelectionRange(int selection_start, int selection_end);
	/// Retrieves the selection range and text.
	/// @param[out] selection_start The first character selected.
	/// @param[out] selection_end The first character *after* the selection.
	/// @param[out] selected_text The selected text.
	void GetSelection(int* selection_start, int* selection_end, String* selected_text) const;

	/// Sets visual feedback used for the IME composition in the range.
	/// @param[in] range_start The first character to be selected.
	/// @param[in] range_end The first character *after* the selection.
	void SetCompositionRange(int range_start, int range_end);
	/// Obtains the IME composition byte range relative to the current value.
	void GetCompositionRange(int& range_start, int& range_end) const;

	/// Update the colours of the selected text.
	void UpdateSelectionColours();
	/// Generates the text cursor.
	void GenerateCursor();
	/// Force text formatting on the next layout update.
	void ForceFormattingOnNextLayout();

	/// Updates the cursor, if necessary.
	void OnUpdate();
	/// Renders the cursor, if it is visible.
	void OnRender();
	/// Formats the widget's internal content.
	void OnLayout();
	/// Called when the parent element's size changes.
	void OnResize();

protected:
	enum class CursorMovement { Begin = -4, BeginLine = -3, PreviousWord = -2, Left = -1, Right = 1, NextWord = 2, EndLine = 3, End = 4 };

	/// Processes the "keydown" and "textinput" event to write to the input field, and the "focus" and
	/// "blur" to set the state of the cursor.
	void ProcessEvent(Event& event) override;

	/// Adds new characters to the string at the cursor position.
	/// @param[in] string The characters to add.
	/// @return True if at least one character was successfully added, false otherwise.
	bool AddCharacters(String string);
	/// Deletes characters from the string.
	/// @param[in] direction Movement of cursor for deletion.
	/// @return True if a character was deleted, false otherwise.
	bool DeleteCharacters(CursorMovement direction);

	/// Removes any invalid characters from the string.
	virtual void SanitizeValue(String& value) = 0;
	/// Transforms the displayed value of the text box, typically used for password fields.
	/// @note Only use this for transforming characters, do not modify the length of the string.
	virtual void TransformValue(String& value);
	/// Called when the user pressed enter.
	virtual void LineBreak() = 0;

	/// Gets the parent element containing the widget.
	Element* GetElement() const;

	/// Obtains the text input handler of the parent element's context.
	TextInputHandler* GetTextInputHandler() const;

	/// Returns true if the text input element is currently focused.
	bool IsFocused() const;

	/// Dispatches a change event to the widget's element.
	void DispatchChangeEvent(bool linebreak = false);

private:
	struct Line {
		// Offset into the text field's value.
		int value_offset;
		// The size of the contents of the line (including the trailing endline, if that terminated the line).
		int size;
		// The length of the editable characters on the line (excluding any trailing endline).
		int editable_length;
	};

	/// Returns the displayed value of the text field.
	/// @note For password fields this would only return the displayed asterisks '****', while the attribute value below contains the underlying text.
	const String& GetValue() const;

	/// Moves the cursor along the current line.
	/// @param[in] movement Cursor movement operation.
	/// @param[in] select True if the movement will also move the selection cursor, false if not.
	/// @param[out] out_of_bounds Set to true if the resulting line position is out of bounds, false if not.
	/// @return True if selection was changed.
	bool MoveCursorHorizontal(CursorMovement movement, bool select, bool& out_of_bounds);
	/// Moves the cursor up and down the text field.
	/// @param[in] x How far to move the cursor.
	/// @param[in] select True if the movement will also move the selection cursor, false if not.
	/// @param[out] out_of_bounds Set to true if the resulting line position is out of bounds, false if not.
	/// @return True if selection was changed.
	bool MoveCursorVertical(int distance, bool select, bool& out_of_bounds);
	// Move the cursor to utf-8 boundaries, in case it was moved into the middle of a multibyte character.
	/// @param[in] forward True to seek forward, else back.
	void MoveCursorToCharacterBoundaries(bool forward);
	// Expands the cursor, selecting the current word or nearby whitespace.
	void ExpandSelection();

	/// Returns the relative indices from the current absolute index.
	void GetRelativeCursorIndices(int& out_cursor_line_index, int& out_cursor_character_index) const;
	/// Sets the absolute cursor index from the given relative indices.
	void SetCursorFromRelativeIndices(int line_index, int character_index);

	/// Calculates the line index under a specific vertical position.
	/// @param[in] position The position to query.
	/// @return The index of the line under the mouse cursor.
	int CalculateLineIndex(float position) const;
	/// Calculates the character index along a line under a specific horizontal position.
	/// @param[in] line_index The line to query.
	/// @param[in] position The position to query.
	/// @return The index of the character under the mouse cursor.
	int CalculateCharacterIndex(int line_index, float position);

	/// Shows or hides the cursor.
	/// @param[in] show True to show the cursor, false to hide it.
	/// @param[in] move_to_cursor True to force the cursor to be visible, false to not scroll the widget.
	void ShowCursor(bool show, bool move_to_cursor = true);

	/// Formats the element, laying out the text and inserting scrollbars as appropriate.
	void FormatElement();
	/// Formats the input element's text field.
	/// @param[in] height_constraint Abort formatting when the formatted size grows larger than this height.
	/// @return The content area of the element.
	Vector2f FormatText(float height_constraint = FLT_MAX);

	/// Updates the position to render the cursor.
	/// @param[in] update_ideal_cursor_position Generally should be true on horizontal movement and false on vertical movement.
	void UpdateCursorPosition(bool update_ideal_cursor_position);

	/// Expand or shrink the text selection to the position of the cursor.
	/// @param[in] selecting True if the new position of the cursor should expand / contract the selection area, false if it should only set the
	/// anchor for future selections.
	/// @return True if selection was changed.
	bool UpdateSelection(bool selecting);
	/// Removes the selection of text.
	/// @return True if selection was changed.
	bool ClearSelection();
	/// Deletes all selected text and removes the selection.
	void DeleteSelection();
	/// Copies the selection (if any) to the clipboard.
	void CopySelection();

	/// Split one line of text into three parts, based on the current selection.
	/// @param[out] pre_selection The section of unselected text before any selected text on the line.
	/// @param[out] selection The section of selected text on the line.
	/// @param[out] post_selection The section of unselected text after any selected text on the line. If there is no selection on the line, then this
	/// will be empty.
	/// @param[in] line The text making up the line.
	/// @param[in] line_begin The absolute index at the beginning of the line.
	/// @lifetime The returned string views are tied to the lifetime of the line's data.
	void GetLineSelection(StringView& pre_selection, StringView& selection, StringView& post_selection, const String& line, int line_begin) const;
	/// Fetch the IME composition range on the line.
	/// @param[out] pre_composition The section of text before the IME composition string on the line.
	/// @param[out] ime_composition The IME composition string on the line.
	/// @param[in] line The text making up the line.
	/// @param[in] line_begin The absolute index at the beginning of the line.
	void GetLineIMEComposition(StringView& pre_composition, StringView& ime_composition, const String& line, int line_begin) const;

	/// Returns the offset that aligns the contents of the line according to the 'text-align' property.
	float GetAlignmentSpecificTextOffset(const Line& line) const;

	/// Returns the used line height.
	float GetLineHeight() const;
	/// Returns the width available for the text contents without overflowing, that is, the content area subtracted by any scrollbar.
	float GetAvailableWidth() const;
	/// Returns the height available for the text contents without overflowing, that is, the content area subtracted by any scrollbar.
	float GetAvailableHeight() const;

	ElementFormControl* parent;

	ElementText* text_element;
	ElementText* selected_text_element;
	Vector2f internal_dimensions;
	Vector2f scroll_offset;

	using LineList = Vector<Line>;
	LineList lines;

	// Length in number of characters.
	int max_length;

	// -- All indices are in bytes: Should always be moved along UTF-8 start bytes. --

	// Absolute cursor index. Byte index into the text field's value.
	int absolute_cursor_index;
	// When the cursor is located at the very end of a word-wrapped line there are two valid positions for the same absolute index: at the end of the
	// line and at the beginning of the next line. This state determines which of these lines the cursor is placed on visually.
	bool cursor_wrap_down;

	bool ideal_cursor_position_to_the_right_of_cursor;
	bool cancel_next_drag;
	bool force_formatting_on_next_layout;

	// Selection. The start and end indices of the selection are in absolute coordinates.
	Element* selection_element;
	int selection_anchor_index;
	int selection_begin_index;
	int selection_length;

	// The colour of the background of selected text.
	ColourbPremultiplied selection_colour;
	// The selection background.
	Geometry selection_composition_geometry;

	// IME composition range. The start and end indices are in absolute coordinates.
	int ime_composition_begin_index;
	int ime_composition_end_index;

	// The IME context for this widget.
	UniquePtr<WidgetTextInputContext> text_input_context;

	// Cursor visibility and timings.
	float cursor_timer;
	bool cursor_visible;
	bool keyboard_showed;
	/// Activate or deactivate keyboard (for touchscreen devices)
	/// @param[in] active True if need activate keyboard, false if need deactivate.
	void SetKeyboardActive(bool active);

	bool ink_overflow;

	double last_update_time;

	// The cursor geometry.
	float ideal_cursor_position;
	Vector2f cursor_position;
	Vector2f cursor_size;
	Geometry cursor_geometry;
};

} // namespace Rml
#endif
