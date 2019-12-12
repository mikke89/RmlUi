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

#include "WidgetTextInput.h"
#include "ElementTextSelection.h"
#include "../../Include/RmlUi/Controls/ElementFormControl.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementScroll.h"
#include "../../Include/RmlUi/Core/ElementText.h"
#include "../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Input.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../Core/Clock.h"
#include <algorithm>
#include <limits.h>

namespace Rml {
namespace Controls {

static constexpr float CURSOR_BLINK_TIME = 0.7f;

static bool IsWordCharacter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || ((unsigned char)c >= 128);
}

WidgetTextInput::WidgetTextInput(ElementFormControl* _parent) : internal_dimensions(0, 0), scroll_offset(0, 0), selection_geometry(_parent), cursor_position(0, 0), cursor_size(0, 0), cursor_geometry(_parent)
{
	keyboard_showed = false;
	
	parent = _parent;
	parent->SetProperty(Core::PropertyId::WhiteSpace, Core::Property(Core::Style::WhiteSpace::Pre));
	parent->SetProperty(Core::PropertyId::OverflowX, Core::Property(Core::Style::Overflow::Hidden));
	parent->SetProperty(Core::PropertyId::OverflowY, Core::Property(Core::Style::Overflow::Hidden));
	parent->SetProperty(Core::PropertyId::Drag, Core::Property(Core::Style::Drag::Drag));
	parent->SetClientArea(Rml::Core::Box::CONTENT);

	parent->AddEventListener(Core::EventId::Keydown, this, true);
	parent->AddEventListener(Core::EventId::Textinput, this, true);
	parent->AddEventListener(Core::EventId::Focus, this, true);
	parent->AddEventListener(Core::EventId::Blur, this, true);
	parent->AddEventListener(Core::EventId::Mousedown, this, true);
	parent->AddEventListener(Core::EventId::Dblclick, this, true);
	parent->AddEventListener(Core::EventId::Drag, this, true);

	Core::ElementPtr unique_text = Core::Factory::InstanceElement(parent, "#text", "#text", Rml::Core::XMLAttributes());
	text_element = rmlui_dynamic_cast< Core::ElementText* >(unique_text.get());
	Core::ElementPtr unique_selected_text = Core::Factory::InstanceElement(parent, "#text", "#text", Rml::Core::XMLAttributes());
	selected_text_element = rmlui_dynamic_cast< Core::ElementText* >(unique_selected_text.get());
	if (text_element)
	{
		text_element->SuppressAutoLayout();
		parent->AppendChild(std::move(unique_text), false);

		selected_text_element->SuppressAutoLayout();
		parent->AppendChild(std::move(unique_selected_text), false);
	}

	// Create the dummy selection element.
	Core::ElementPtr unique_selection = Core::Factory::InstanceElement(parent, "#selection", "selection", Rml::Core::XMLAttributes());
	if (ElementTextSelection* text_selection_element = rmlui_dynamic_cast<ElementTextSelection*>(unique_selection.get()))
	{
		selection_element = text_selection_element;
		text_selection_element->SetWidget(this);
		parent->AppendChild(std::move(unique_selection), false);
	}

	edit_index = 0;
	absolute_cursor_index = 0;
	cursor_line_index = 0;
	cursor_character_index = 0;
	cursor_on_right_side_of_character = true;
	cancel_next_drag = false;

	ideal_cursor_position = 0;

	max_length = -1;

	selection_anchor_index = 0;
	selection_begin_index = 0;
	selection_length = 0;

	last_update_time = 0;

	ShowCursor(false);
}

WidgetTextInput::~WidgetTextInput()
{
	parent->RemoveEventListener(Core::EventId::Keydown, this, true);
	parent->RemoveEventListener(Core::EventId::Textinput, this, true);
	parent->RemoveEventListener(Core::EventId::Focus, this, true);
	parent->RemoveEventListener(Core::EventId::Blur, this, true);
	parent->RemoveEventListener(Core::EventId::Mousedown, this, true);
	parent->RemoveEventListener(Core::EventId::Dblclick, this, true);
	parent->RemoveEventListener(Core::EventId::Drag, this, true);

	// Remove all the children added by the text widget.
	parent->RemoveChild(text_element);
	parent->RemoveChild(selected_text_element);
	parent->RemoveChild(selection_element);
}

// Sets the value of the text field.
void WidgetTextInput::SetValue(const Core::String& value)
{
	text_element->SetText(value);
	FormatElement();

	UpdateRelativeCursor();
}

// Sets the maximum length (in characters) of this text field.
void WidgetTextInput::SetMaxLength(int _max_length)
{
	if (max_length != _max_length)
	{
		max_length = _max_length;
		if (max_length >= 0)
		{
			Core::String value = GetElement()->GetAttribute< Rml::Core::String >("value", "");

			int num_characters = 0;
			size_t i_erase = value.size();

			for (auto it = Core::StringIteratorU8(value); it; ++it)
			{
				num_characters += 1;
				if (num_characters > max_length)
				{
					i_erase = size_t(it.offset());
					break;
				}
			}

			if(i_erase < value.size())
			{
				value.erase(i_erase);
				GetElement()->SetAttribute("value", value);
			}
		}
	}
}

// Returns the maximum length (in characters) of this text field.
int WidgetTextInput::GetMaxLength() const
{
	return max_length;
}

int WidgetTextInput::GetLength() const
{
	Core::String value = GetElement()->GetAttribute< Core::String >("value", "");
	size_t result = Core::StringUtilities::LengthUTF8(value);
	return (int)result;
}

// Update the colours of the selected text.
void WidgetTextInput::UpdateSelectionColours()
{
	// Determine what the colour of the selected text is. If our 'selection' element has the 'color'
	// attribute set, then use that. Otherwise, use the inverse of our own text colour.
	Rml::Core::Colourb colour;
	const Rml::Core::Property* colour_property = selection_element->GetLocalProperty("color");
	if (colour_property != nullptr)
		colour = colour_property->Get< Rml::Core::Colourb >();
	else
	{
		colour = parent->GetComputedValues().color;
		colour.red = 255 - colour.red;
		colour.green = 255 - colour.green;
		colour.blue = 255 - colour.blue;
	}

	// Set the computed text colour on the element holding the selected text.
	selected_text_element->SetProperty(Core::PropertyId::Color, Rml::Core::Property(colour, Rml::Core::Property::COLOUR));

	// If the 'background-color' property has been set on the 'selection' element, use that as the
	// background colour for the selected text. Otherwise, use the inverse of the selected text
	// colour.
	colour_property = selection_element->GetLocalProperty("background-color");
	if (colour_property != nullptr)
		selection_colour = colour_property->Get< Rml::Core::Colourb >();
	else
		selection_colour = Rml::Core::Colourb(255 - colour.red, 255 - colour.green, 255 - colour.blue, colour.alpha);
}

// Updates the cursor, if necessary.
void WidgetTextInput::OnUpdate()
{
	if (cursor_timer > 0)
	{
		double current_time = Core::Clock::GetElapsedTime();
		cursor_timer -= float(current_time - last_update_time);
		last_update_time = current_time;

		while (cursor_timer <= 0)
		{
			cursor_timer += CURSOR_BLINK_TIME;
			cursor_visible = !cursor_visible;
		}
	}
}

void WidgetTextInput::OnResize()
{
	GenerateCursor();

	Rml::Core::Vector2f text_position = parent->GetBox().GetPosition(Core::Box::CONTENT);
	text_element->SetOffset(text_position, parent);
	selected_text_element->SetOffset(text_position, parent);

	Rml::Core::Vector2f new_internal_dimensions = parent->GetBox().GetSize(Core::Box::CONTENT);
	if (new_internal_dimensions != internal_dimensions)
	{
		internal_dimensions = new_internal_dimensions;

		FormatElement();
		UpdateCursorPosition();
	}
}

// Renders the cursor, if it is visible.
void WidgetTextInput::OnRender()
{
	Core::ElementUtilities::SetClippingRegion(text_element);

	Rml::Core::Vector2f text_translation = parent->GetAbsoluteOffset() - Rml::Core::Vector2f(parent->GetScrollLeft(), parent->GetScrollTop());
	selection_geometry.Render(text_translation);

	if (cursor_visible &&
		!parent->IsDisabled())
	{
		cursor_geometry.Render(text_translation + cursor_position);
	}
}

// Formats the widget's internal content.
void WidgetTextInput::OnLayout()
{
	FormatElement();
	parent->SetScrollLeft(scroll_offset.x);
	parent->SetScrollTop(scroll_offset.y);
}

// Returns the input element's underlying text element.
Core::ElementText* WidgetTextInput::GetTextElement()
{
	return text_element;
}

// Returns the input element's maximum allowed text dimensions.
const Rml::Core::Vector2f& WidgetTextInput::GetTextDimensions() const
{
	return internal_dimensions;
}

// Gets the parent element containing the widget.
Core::Element* WidgetTextInput::GetElement() const
{
	return parent;
}

// Dispatches a change event to the widget's element.
void WidgetTextInput::DispatchChangeEvent(bool linebreak)
{
	Rml::Core::Dictionary parameters;
	parameters["value"] = GetElement()->GetAttribute< Rml::Core::String >("value", "");
	parameters["linebreak"] = Core::Variant(linebreak);
	GetElement()->DispatchEvent(Core::EventId::Change, parameters);
}

// Processes the "keydown" and "textinput" event to write to the input field, and the "focus" and "blur" to set
// the state of the cursor.
void WidgetTextInput::ProcessEvent(Core::Event& event)
{
	if (parent->IsDisabled())
		return;

	using Rml::Core::EventId;

	switch (event.GetId())
	{
	case EventId::Keydown:
	{
		Core::Input::KeyIdentifier key_identifier = (Core::Input::KeyIdentifier) event.GetParameter< int >("key_identifier", 0);
		bool numlock = event.GetParameter< int >("num_lock_key", 0) > 0;
		bool shift = event.GetParameter< int >("shift_key", 0) > 0;
		bool ctrl = event.GetParameter< int >("ctrl_key", 0) > 0;

		switch (key_identifier)
		{
		case Core::Input::KI_NUMPAD4:	if (numlock) break;
		case Core::Input::KI_LEFT:		MoveCursorHorizontal(ctrl ? CursorMovement::PreviousWord : CursorMovement::Left, shift); break;

		case Core::Input::KI_NUMPAD6:	if (numlock) break;
		case Core::Input::KI_RIGHT:		MoveCursorHorizontal(ctrl ? CursorMovement::NextWord : CursorMovement::Right, shift); break;

		case Core::Input::KI_NUMPAD8:	if (numlock) break;
		case Core::Input::KI_UP:		MoveCursorVertical(-1, shift); break;

		case Core::Input::KI_NUMPAD2:	if (numlock) break;
		case Core::Input::KI_DOWN:		MoveCursorVertical(1, shift); break;

		case Core::Input::KI_NUMPAD7:	if (numlock) break;
		case Core::Input::KI_HOME:		MoveCursorHorizontal(ctrl ? CursorMovement::Begin : CursorMovement::BeginLine, shift); break;

		case Core::Input::KI_NUMPAD1:	if (numlock) break;
		case Core::Input::KI_END:		MoveCursorHorizontal(ctrl ? CursorMovement::End : CursorMovement::EndLine, shift); break;

		case Core::Input::KI_NUMPAD3:	if (numlock) break;
		case Core::Input::KI_PRIOR:		MoveCursorVertical(-int(internal_dimensions.y / parent->GetLineHeight()) + 1, shift); break;

		case Core::Input::KI_NUMPAD9:	if (numlock) break;
		case Core::Input::KI_NEXT:		MoveCursorVertical(int(internal_dimensions.y / parent->GetLineHeight()) - 1, shift); break;

		case Core::Input::KI_BACK:
		{
			CursorMovement direction = (ctrl ? CursorMovement::PreviousWord : CursorMovement::Left);
			if (DeleteCharacters(direction))
			{
				FormatElement();
				UpdateRelativeCursor();
			}

			ShowCursor(true);
		}
		break;

		case Core::Input::KI_DECIMAL:	if (numlock) break;
		case Core::Input::KI_DELETE:
		{
			CursorMovement direction = (ctrl ? CursorMovement::NextWord : CursorMovement::Right);
			if (DeleteCharacters(direction))
			{
				FormatElement();
				UpdateRelativeCursor();
			}

			ShowCursor(true);
		}
		break;

		case Core::Input::KI_NUMPADENTER:
		case Core::Input::KI_RETURN:
		{
			LineBreak();
		}
		break;

		case Core::Input::KI_A:
		{
			if (ctrl)
			{
				MoveCursorHorizontal(CursorMovement::Begin, false);
				MoveCursorHorizontal(CursorMovement::End, true);
			}
		}
		break;

		case Core::Input::KI_C:
		{
			if (ctrl)
				CopySelection();
		}
		break;

		case Core::Input::KI_X:
		{
			if (ctrl)
			{
				CopySelection();
				DeleteSelection();
			}
		}
		break;

		case Core::Input::KI_V:
		{
			if (ctrl)
			{
				Core::String clipboard_text;
				Core::GetSystemInterface()->GetClipboardText(clipboard_text);

				AddCharacters(clipboard_text);
			}
		}
		break;

		// Ignore tabs so input fields can be navigated through with keys.
		case Core::Input::KI_TAB:
			return;

		default:
		break;
		}

		event.StopPropagation();
	}
	break;

	case EventId::Textinput:
	{
		// Only process the text if no modifier keys are pressed.
		if (event.GetParameter< int >("ctrl_key", 0) == 0 &&
			event.GetParameter< int >("alt_key", 0) == 0 &&
			event.GetParameter< int >("meta_key", 0) == 0)
		{
			Core::String text = event.GetParameter("text", Core::String{});
			AddCharacters(text);
		}

		ShowCursor(true);
		event.StopPropagation();
	}
	break;
	case EventId::Focus:
	{
		if (event.GetTargetElement() == parent)
		{
			UpdateSelection(false);
			ShowCursor(true, false);
		}
	}
	break;
	case EventId::Blur:
	{
		if (event.GetTargetElement() == parent)
		{
			ClearSelection();
			ShowCursor(false, false);
		}
	}
	break;
	case EventId::Drag:
		if (cancel_next_drag)
		{
			// We currently ignore drag events right after a double click. They would need to be handled
			// specially by selecting whole words at a time, which is not yet implemented.
			break;
		}
		// Else, fall through:
	case EventId::Mousedown:
	{
		if (event.GetTargetElement() == parent)
		{
			Core::Vector2f mouse_position = Core::Vector2f(event.GetParameter< float >("mouse_x", 0), event.GetParameter< float >("mouse_y", 0));
			mouse_position -= text_element->GetAbsoluteOffset();

			cursor_line_index = CalculateLineIndex(mouse_position.y);
			cursor_character_index = CalculateCharacterIndex(cursor_line_index, mouse_position.x);

			UpdateAbsoluteCursor();
			MoveCursorToCharacterBoundaries(false);

			UpdateCursorPosition();
			ideal_cursor_position = cursor_position.x;

			UpdateSelection(event == Core::EventId::Drag || event.GetParameter< int >("shift_key", 0) > 0);

			ShowCursor(true); 
			cancel_next_drag = false;
		}
	}
	break;
	case EventId::Dblclick:
	{
		if (event.GetTargetElement() == parent)
		{
			ExpandSelection();
			cancel_next_drag = true;
		}
	}
	break;

	default:
		break;
	}

}

// Adds a new character to the string at the cursor position.
bool WidgetTextInput::AddCharacters(Rml::Core::String string)
{
	// Erase invalid characters from string
	auto invalid_character = [this](char c) {
		return ((unsigned char)c <= 127 && !IsCharacterValid(c));
	};
	string.erase(
		std::remove_if(string.begin(), string.end(), invalid_character),
		string.end()
	);

	if (string.empty())
		return false;

	if (selection_length > 0)
		DeleteSelection();

	if (max_length >= 0 && GetLength() >= max_length)
		return false;

	Core::String value = GetElement()->GetAttribute< Rml::Core::String >("value", "");
	
	value.insert(std::min(size_t(GetCursorIndex()), value.size()), string);

	edit_index += (int)string.size();

	GetElement()->SetAttribute("value", value);
	DispatchChangeEvent();

	UpdateSelection(false);

	return true;
}

// Deletes a character from the string.
bool WidgetTextInput::DeleteCharacters(CursorMovement direction)
{
	// We set a selection of characters according to direction, and then delete it.
	// If we already have a selection, we delete that first.
	if (selection_length <= 0)
		MoveCursorHorizontal(direction, true);

	if (selection_length > 0)
	{
		DeleteSelection();
		DispatchChangeEvent();

		UpdateSelection(false);

		return true;
	}

	return false;
}

// Copies the selection (if any) to the clipboard.
void WidgetTextInput::CopySelection()
{
	const Core::String& value = GetElement()->GetAttribute< Rml::Core::String >("value", "");
	const Core::String snippet = value.substr(std::min(size_t(selection_begin_index), value.size()), selection_length);
	Core::GetSystemInterface()->SetClipboardText(snippet);
}

// Returns the absolute index of the cursor.
int WidgetTextInput::GetCursorIndex() const
{
	return edit_index;
}

// Moves the cursor along the current line.
void WidgetTextInput::MoveCursorHorizontal(CursorMovement movement, bool select)
{
	// Whether to seek forward or back to align to utf8 boundaries later.
	bool seek_forward = false;

	switch (movement)
	{
	case CursorMovement::Begin:
		absolute_cursor_index = 0;
		break;
	case CursorMovement::BeginLine:
		absolute_cursor_index -= cursor_character_index;
		break;
	case CursorMovement::PreviousWord:
		if (cursor_character_index <= 1)
		{
			absolute_cursor_index -= 1;
		}
		else
		{
			bool word_character_found = false;
			const char* p_rend = lines[cursor_line_index].content.data();
			const char* p_rbegin = p_rend + cursor_character_index;
			const char* p = p_rbegin - 1;
			for (; p > p_rend; --p)
			{
				bool is_word_character = IsWordCharacter(*p);
				if(word_character_found && !is_word_character)
					break;
				else if(is_word_character)
					word_character_found = true;
			}
			if (p != p_rend) ++p;
			absolute_cursor_index += int(p - p_rbegin);
		}
		break;
	case CursorMovement::Left:
		if (!select && selection_length > 0)
			absolute_cursor_index = selection_begin_index;
		else
			absolute_cursor_index -= 1;
		break;
	case CursorMovement::Right:
		seek_forward = true;
		if (!select && selection_length > 0)
			absolute_cursor_index = selection_begin_index + selection_length;
		else
			absolute_cursor_index += 1;
		break;
	case CursorMovement::NextWord:
		if (cursor_character_index >= lines[cursor_line_index].content_length)
		{
			absolute_cursor_index += 1;
		}
		else
		{
			bool whitespace_found = false;
			const char* p_begin = lines[cursor_line_index].content.data() + cursor_character_index;
			const char* p_end = lines[cursor_line_index].content.data() + lines[cursor_line_index].content_length;
			const char* p = p_begin;
			for (; p < p_end; ++p)
			{
				bool is_whitespace = !IsWordCharacter(*p);
				if (whitespace_found && !is_whitespace)
					break;
				else if (is_whitespace)
					whitespace_found = true;
			}
			absolute_cursor_index += int(p - p_begin);
		}
		break;
	case CursorMovement::EndLine:
		absolute_cursor_index += lines[cursor_line_index].content_length - cursor_character_index;
		break;
	case CursorMovement::End:
		absolute_cursor_index = INT_MAX;
		break;
	default:
		break;
	}
	
	absolute_cursor_index = Rml::Core::Math::Max(0, absolute_cursor_index);

	UpdateRelativeCursor();
	MoveCursorToCharacterBoundaries(seek_forward);

	ideal_cursor_position = cursor_position.x;
	UpdateSelection(select);
	ShowCursor(true);
}

// Moves the cursor up and down the text field.
void WidgetTextInput::MoveCursorVertical(int distance, bool select)
{
	bool update_ideal_cursor_position = false;
	cursor_line_index += distance;

	if (cursor_line_index < 0)
	{
		cursor_line_index = 0;
		cursor_character_index = 0;

		update_ideal_cursor_position = true;
	}
	else if (cursor_line_index >= (int) lines.size())
	{
		cursor_line_index = (int) lines.size() - 1;
		cursor_character_index = (int) lines[cursor_line_index].content_length;

		update_ideal_cursor_position = true;
	}
	else
		cursor_character_index = CalculateCharacterIndex(cursor_line_index, ideal_cursor_position);

	UpdateAbsoluteCursor();

	MoveCursorToCharacterBoundaries(false);

	UpdateCursorPosition();

	if (update_ideal_cursor_position)
		ideal_cursor_position = cursor_position.x;

	UpdateSelection(select);

	ShowCursor(true);
}

void WidgetTextInput::MoveCursorToCharacterBoundaries(bool forward)
{
	const char* p_line_begin = lines[cursor_line_index].content.data();
	const char* p_line_end = p_line_begin + lines[cursor_line_index].content_length;
	const char* p_cursor = p_line_begin + cursor_character_index;
	const char* p = p_cursor;

	if (forward)
		p = Core::StringUtilities::SeekForwardUTF8(p_cursor, p_line_end);
	else
		p = Core::StringUtilities::SeekBackwardUTF8(p_cursor, p_line_begin);

	if (p != p_cursor)
	{
		absolute_cursor_index += int(p - p_cursor);
		UpdateRelativeCursor();
	}
}

void WidgetTextInput::ExpandSelection()
{
	const char* const p_begin = lines[cursor_line_index].content.data();
	const char* const p_end = p_begin + lines[cursor_line_index].content_length;
	const char* const p_index = p_begin + cursor_character_index;

	// If true, we are expanding word characters, if false, whitespace characters.
	// The first character encountered defines the bool.
	bool expanding_word = false;
	bool expanding_word_set = false;

	auto character_is_wrong_type = [&expanding_word_set, &expanding_word](const char* p) -> bool {
		bool is_word_character = IsWordCharacter(*p);
		if (expanding_word_set && (expanding_word != is_word_character))
			return true;
		if (!expanding_word_set)
		{
			expanding_word = is_word_character;
			expanding_word_set = true;
		}
		return false;
	};

	auto search_left = [&]() -> const char* {
		const char* p = p_index;
		for (; p > p_begin; p--)
			if (character_is_wrong_type(p - 1))
				break;
		return p;
	};
	auto search_right = [&]() -> const char* {
		const char* p = p_index;
		for (; p < p_end; p++)
			if (character_is_wrong_type(p))
				break;
		return p;
	};

	const char* p_left = p_index;
	const char* p_right = p_index;

	if (cursor_on_right_side_of_character)
	{
		p_right = search_right();
		p_left = search_left();
	}
	else
	{
		p_left = search_left();
		p_right = search_right();
	}

	absolute_cursor_index -= int(p_index - p_left);
	UpdateRelativeCursor();
	MoveCursorToCharacterBoundaries(false);
	UpdateSelection(false);

	absolute_cursor_index += int(p_right - p_left);
	UpdateRelativeCursor();
	MoveCursorToCharacterBoundaries(true);
	UpdateSelection(true);
}

// Updates the absolute cursor index from the relative cursor indices.
void WidgetTextInput::UpdateAbsoluteCursor()
{
	RMLUI_ASSERT(cursor_line_index < (int) lines.size())

	absolute_cursor_index = cursor_character_index;
	edit_index = cursor_character_index;

	for (int i = 0; i < cursor_line_index; i++)
	{
		absolute_cursor_index += (int)lines[i].content.size();
		edit_index += (int)lines[i].content.size() + lines[i].extra_characters;
	}
}

// Updates the relative cursor indices from the absolute cursor index.
void WidgetTextInput::UpdateRelativeCursor()
{
	int num_characters = 0;
	edit_index = absolute_cursor_index;

	for (size_t i = 0; i < lines.size(); i++)
	{
		if (num_characters + lines[i].content_length >= absolute_cursor_index)
		{
			cursor_line_index = (int) i;
			cursor_character_index = absolute_cursor_index - num_characters;

			UpdateCursorPosition();

			return;
		}

		num_characters += (int) lines[i].content.size();
		edit_index += lines[i].extra_characters;
	}

	// We shouldn't ever get here; this means we actually couldn't find where the absolute cursor said it was. So we'll
	// just set the relative cursors to the very end of the text field, and update the absolute cursor to point here.
	cursor_line_index = (int) lines.size() - 1;
	cursor_character_index = lines[cursor_line_index].content_length;
	absolute_cursor_index = num_characters;
	edit_index = num_characters;

	UpdateCursorPosition();
}

// Calculates the line index under a specific vertical position.
int WidgetTextInput::CalculateLineIndex(float position)
{
	float line_height = parent->GetLineHeight();
	int line_index = Rml::Core::Math::RealToInteger(position / line_height);
	return Rml::Core::Math::Clamp(line_index, 0, (int) (lines.size() - 1));
}

// Calculates the character index along a line under a specific horizontal position.
int WidgetTextInput::CalculateCharacterIndex(int line_index, float position)
{
	int prev_offset = 0;
	float prev_line_width = 0;

	cursor_on_right_side_of_character = true;

	for(auto it = Core::StringIteratorU8(lines[line_index].content, 0, lines[line_index].content_length); it; )
	{
		++it;
		int offset = (int)it.offset();

		float line_width = (float) Core::ElementUtilities::GetStringWidth(text_element, lines[line_index].content.substr(0, offset));
		if (line_width > position)
		{
			if (position - prev_line_width < line_width - position)
			{
				return prev_offset;
			}
			else
			{
				cursor_on_right_side_of_character = false;
				return offset;
			}
		}

		prev_line_width = line_width;
		prev_offset = offset;
	}

	return prev_offset;
}

// Shows or hides the cursor.
void WidgetTextInput::ShowCursor(bool show, bool move_to_cursor)
{
	if (show)
	{
		cursor_visible = true;
		SetKeyboardActive(true);
		keyboard_showed = true;
		
		cursor_timer = CURSOR_BLINK_TIME;
		last_update_time = Core::GetSystemInterface()->GetElapsedTime();

		// Shift the cursor into view.
		if (move_to_cursor)
		{
			float minimum_scroll_top = (cursor_position.y + cursor_size.y) - parent->GetClientHeight();
			if (parent->GetScrollTop() < minimum_scroll_top)
				parent->SetScrollTop(minimum_scroll_top);
			else if (parent->GetScrollTop() > cursor_position.y)
				parent->SetScrollTop(cursor_position.y);

			float minimum_scroll_left = (cursor_position.x + cursor_size.x) - parent->GetClientWidth();
			if (parent->GetScrollLeft() < minimum_scroll_left)
				parent->SetScrollLeft(minimum_scroll_left);
			else if (parent->GetScrollLeft() > cursor_position.x)
				parent->SetScrollLeft(cursor_position.x);

			scroll_offset.x = parent->GetScrollLeft();
			scroll_offset.y = parent->GetScrollTop();
		}
	}
	else
	{
		cursor_visible = false;
		cursor_timer = -1;
		last_update_time = 0;
		if (keyboard_showed)
		{
			SetKeyboardActive(false);
			keyboard_showed = false;
		}
	}
}

// Formats the element, laying out the text and inserting scrollbars as appropriate.
void WidgetTextInput::FormatElement()
{
	using namespace Core::Style;
	Core::ElementScroll* scroll = parent->GetElementScroll();
	float width = parent->GetBox().GetSize(Core::Box::PADDING).x;

	Overflow x_overflow_property = parent->GetComputedValues().overflow_x;
	Overflow y_overflow_property = parent->GetComputedValues().overflow_y;

	if (x_overflow_property == Overflow::Scroll)
		scroll->EnableScrollbar(Core::ElementScroll::HORIZONTAL, width);
	else
		scroll->DisableScrollbar(Core::ElementScroll::HORIZONTAL);

	if (y_overflow_property == Overflow::Scroll)
		scroll->EnableScrollbar(Core::ElementScroll::VERTICAL, width);
	else
		scroll->DisableScrollbar(Core::ElementScroll::VERTICAL);

	// Format the text and determine its total area.
	Rml::Core::Vector2f content_area = FormatText();

	// If we're set to automatically generate horizontal scrollbars, check for that now.
	if (x_overflow_property == Overflow::Auto)
	{
		if (parent->GetClientWidth() < content_area.x)
			scroll->EnableScrollbar(Core::ElementScroll::HORIZONTAL, width);
	}

	// Now check for vertical overflow. If we do turn on the scrollbar, this will cause a reflow.
	if (y_overflow_property == Overflow::Auto)
	{
		if (parent->GetClientHeight() < content_area.y)
		{
			scroll->EnableScrollbar(Core::ElementScroll::VERTICAL, width);
			content_area = FormatText();

			if (x_overflow_property == Overflow::Auto &&
				parent->GetClientWidth() < content_area.y)
			{
				scroll->EnableScrollbar(Core::ElementScroll::HORIZONTAL, width);
			}
		}
	}

	parent->SetContentBox(Rml::Core::Vector2f(0, 0), content_area);
	scroll->FormatScrollbars();
}

// Formats the input element's text field.
Rml::Core::Vector2f WidgetTextInput::FormatText()
{
	absolute_cursor_index = edit_index;

	Rml::Core::Vector2f content_area(0, 0);

	// Clear the old lines, and all the lines in the text elements.
	lines.clear();
	text_element->ClearLines();
	selected_text_element->ClearLines();

	// Clear the selection background geometry, and get the vertices and indices so the new geo can
	// be generated.
	selection_geometry.Release(true);
	std::vector< Core::Vertex >& selection_vertices = selection_geometry.GetVertices();
	std::vector< int >& selection_indices = selection_geometry.GetIndices();

	// Determine the line-height of the text element.
	float line_height = parent->GetLineHeight();

	int line_begin = 0;
	Rml::Core::Vector2f line_position(0, 0);
	bool last_line = false;

	// Keep generating lines until all the text content is placed.
	do
	{
		Line line;
		line.extra_characters = 0;
		float line_width;

		// Generate the next line.
		last_line = text_element->GenerateLine(line.content, line.content_length, line_width, line_begin, parent->GetClientWidth() - cursor_size.x, 0, false, false);

		// If this line terminates in a soft-return, then the line may be leaving a space or two behind as an orphan.
		// If so, we must append the orphan onto the line even though it will push the line outside of the input
		// field's bounds.
		bool soft_return = false;
		if (!last_line &&
			(line.content.empty() ||
			 line.content[line.content.size() - 1] != '\n'))
		{
			soft_return = true;

			const Core::String& text = text_element->GetText();
			Core::String orphan;
			for (int i = 1; i >= 0; --i)
			{
				int index = line_begin + line.content_length + i;
				if (index >= (int) text.size())
					continue;

				if (text[index] != ' ')
				{
					orphan.clear();
					continue;
				}

				int next_index = index + 1;
				if (!orphan.empty() ||
					next_index >= (int) text.size() ||
					text[next_index] != ' ')
					orphan += ' ';
			}

			if (!orphan.empty())
			{
				line.content += orphan;
				line.content_length += (int) orphan.size();
				line_width += Core::ElementUtilities::GetStringWidth(text_element, orphan);
			}
		}


		// Now that we have the string of characters appearing on the new line, we split it into
		// three parts; the unselected text appearing before any selected text on the line, the
		// selected text on the line, and any unselected text after the selection.
		Core::String pre_selection, selection, post_selection;
		GetLineSelection(pre_selection, selection, post_selection, line.content, line_begin);

		// The pre-selected text is placed, if there is any (if the selection starts on or before
		// the beginning of this line, then this will be empty).
		if (!pre_selection.empty())
		{
			text_element->AddLine(line_position, pre_selection);
			line_position.x += Core::ElementUtilities::GetStringWidth(text_element, pre_selection);
		}

		// If there is any selected text on this line, place it in the selected text element and
		// generate the geometry for its background.
		if (!selection.empty())
		{
			selected_text_element->AddLine(line_position, selection);
			int selection_width = Core::ElementUtilities::GetStringWidth(selected_text_element, selection);

			selection_vertices.resize(selection_vertices.size() + 4);
			selection_indices.resize(selection_indices.size() + 6);
			Core::GeometryUtilities::GenerateQuad(&selection_vertices[selection_vertices.size() - 4], &selection_indices[selection_indices.size() - 6], line_position, Rml::Core::Vector2f((float)selection_width, line_height), selection_colour, (int)selection_vertices.size() - 4);

			line_position.x += selection_width;
		}

		// If there is any unselected text after the selection on this line, place it in the
		// standard text element after the selected text.
		if (!post_selection.empty())
			text_element->AddLine(line_position, post_selection);


		// Update variables for the next line.
		line_begin += line.content_length;
		line_position.x = 0;
		line_position.y += line_height;

		// Grow the content area width-wise if this line is the longest so far, and push the
		// height out.
		content_area.x = Rml::Core::Math::Max(content_area.x, line_width + cursor_size.x);
		content_area.y = line_position.y;

		// Push a trailing '\r' token onto the back to indicate a soft return if necessary.
		if (soft_return)
		{
			line.content += '\r';
			line.extra_characters -= 1;

			if (edit_index >= line_begin)
				absolute_cursor_index += 1;
		}

		// Push the new line into our array of lines, but first check if its content length needs to be truncated to
		// dodge a trailing endline.
		if (!line.content.empty() &&
			line.content[line.content.size() - 1] == '\n')
			line.content_length -= 1;
		lines.push_back(line);
	}
	while (!last_line);

	return content_area;
}

// Generates the text cursor.
void WidgetTextInput::GenerateCursor()
{
	// Generates the cursor.
	cursor_geometry.Release();

	std::vector< Core::Vertex >& vertices = cursor_geometry.GetVertices();
	vertices.resize(4);

	std::vector< int >& indices = cursor_geometry.GetIndices();
	indices.resize(6);

	cursor_size.x = Core::ElementUtilities::GetDensityIndependentPixelRatio(text_element);
	cursor_size.y = text_element->GetLineHeight() + 2.0f;
	Core::GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Rml::Core::Vector2f(0, 0), cursor_size, parent->GetProperty< Rml::Core::Colourb >("color"));
}

void WidgetTextInput::UpdateCursorPosition()
{
	if (text_element->GetFontFaceHandle() == 0)
		return;

	cursor_position.x = (float) Core::ElementUtilities::GetStringWidth(text_element, lines[cursor_line_index].content.substr(0, cursor_character_index));
	cursor_position.y = -1.f + (float)cursor_line_index * text_element->GetLineHeight();
}

// Expand the text selection to the position of the cursor.
void WidgetTextInput::UpdateSelection(bool selecting)
{
	if (!selecting)
	{
		selection_anchor_index = edit_index;
		ClearSelection();
	}
	else
	{
		int new_begin_index;
		int new_end_index;

		if (edit_index > selection_anchor_index)
		{
			new_begin_index = selection_anchor_index;
			new_end_index = edit_index;
		}
		else
		{
			new_begin_index = edit_index;
			new_end_index = selection_anchor_index;
		}

		if (new_begin_index != selection_begin_index ||
			new_end_index - new_begin_index != selection_length)
		{
			selection_begin_index = new_begin_index;
			selection_length = new_end_index - new_begin_index;

			FormatText();
		}
	}
}

// Removes the selection of text.
void WidgetTextInput::ClearSelection()
{
	if (selection_length > 0)
	{
		selection_length = 0;
		FormatElement();
	}
}

// Deletes all selected text and removes the selection.
void WidgetTextInput::DeleteSelection()
{
	if (selection_length > 0)
	{
		const Core::String& value = GetElement()->GetAttribute< Rml::Core::String >("value", "");

		Rml::Core::String new_value = value.substr(0, selection_begin_index) + value.substr(std::min(size_t(selection_begin_index + selection_length), value.size()));
		GetElement()->SetAttribute("value", new_value);

		// Move the cursor to the beginning of the old selection.
		absolute_cursor_index = selection_begin_index;
		UpdateRelativeCursor();

		// Erase our record of the selection.
		ClearSelection();
	}
}

// Split one line of text into three parts, based on the current selection.
void WidgetTextInput::GetLineSelection(Core::String& pre_selection, Core::String& selection, Core::String& post_selection, const Core::String& line, int line_begin)
{
	// Check if we have any selection at all, and if so if the selection is on this line.
	if (selection_length <= 0 ||
		selection_begin_index + selection_length < line_begin ||
		selection_begin_index > line_begin + (int) line.size())
	{
		pre_selection = line;
		return;
	}

	int line_length = (int)line.size();
	using namespace Rml::Core::Math;

	// Split the line up into its three parts, depending on the size and placement of the selection.
	pre_selection = line.substr(0, Max(0, selection_begin_index - line_begin));
	selection = line.substr(Clamp(selection_begin_index - line_begin, 0, line_length), Max(0, selection_length + Min(0, selection_begin_index - line_begin)));
	post_selection = line.substr(Clamp(selection_begin_index + selection_length - line_begin, 0, line_length));
}

void WidgetTextInput::SetKeyboardActive(bool active)
{
	Core::SystemInterface* system = Core::GetSystemInterface();
	if (system) {
		if (active) 
		{
			system->ActivateKeyboard();
		} else 
		{
			system->DeactivateKeyboard();
		}
	}
}
	
}
}
