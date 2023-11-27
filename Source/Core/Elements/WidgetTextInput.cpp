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

#include "WidgetTextInput.h"
#include "../../../Include/RmlUi/Core/ComputedValues.h"
#include "../../../Include/RmlUi/Core/Context.h"
#include "../../../Include/RmlUi/Core/Core.h"
#include "../../../Include/RmlUi/Core/ElementScroll.h"
#include "../../../Include/RmlUi/Core/ElementText.h"
#include "../../../Include/RmlUi/Core/ElementUtilities.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControl.h"
#include "../../../Include/RmlUi/Core/Factory.h"
#include "../../../Include/RmlUi/Core/FontEngineInterface.h"
#include "../../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../../Include/RmlUi/Core/Input.h"
#include "../../../Include/RmlUi/Core/Math.h"
#include "../../../Include/RmlUi/Core/StringUtilities.h"
#include "../../../Include/RmlUi/Core/SystemInterface.h"
#include "../Clock.h"
#include "ElementTextSelection.h"
#include <algorithm>
#include <limits.h>

namespace Rml {

static constexpr float CURSOR_BLINK_TIME = 0.7f;

enum class CharacterClass { Word, Punctuation, Newline, Whitespace, Undefined };
static CharacterClass GetCharacterClass(char c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || ((unsigned char)c >= 128))
		return CharacterClass::Word;
	if ((c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~'))
		return CharacterClass::Punctuation;
	if (c == '\n')
		return CharacterClass::Newline;
	return CharacterClass::Whitespace;
}

static int ConvertCharacterOffsetToByteOffset(const String& value, int character_offset)
{
	if (character_offset >= (int)value.size())
		return (int)value.size();

	int character_count = 0;
	for (auto it = StringIteratorU8(value); it; ++it)
	{
		character_count += 1;
		if (character_count > character_offset)
			return (int)it.offset();
	}
	return (int)value.size();
}

static int ConvertByteOffsetToCharacterOffset(const String& value, int byte_offset)
{
	int character_count = 0;
	for (auto it = StringIteratorU8(value); it; ++it)
	{
		if (it.offset() >= byte_offset)
			break;
		character_count += 1;
	}
	return character_count;
}

// Clamps the value to the given maximum number of unicode code points. Returns true if the value was changed.
static bool ClampValue(String& value, int max_length)
{
	if (max_length >= 0)
	{
		int max_byte_length = ConvertCharacterOffsetToByteOffset(value, max_length);
		if (max_byte_length < (int)value.size())
		{
			value.erase((size_t)max_byte_length);
			return true;
		}
	}
	return false;
}

WidgetTextInput::WidgetTextInput(ElementFormControl* _parent) :
	internal_dimensions(0, 0), scroll_offset(0, 0), cursor_position(0, 0), cursor_size(0, 0)
{
	keyboard_showed = false;

	parent = _parent;
	parent->SetProperty(PropertyId::WhiteSpace, Property(Style::WhiteSpace::Pre));
	parent->SetProperty(PropertyId::OverflowX, Property(Style::Overflow::Hidden));
	parent->SetProperty(PropertyId::OverflowY, Property(Style::Overflow::Hidden));
	parent->SetProperty(PropertyId::Drag, Property(Style::Drag::Drag));
	parent->SetProperty(PropertyId::WordBreak, Property(Style::WordBreak::BreakWord));
	parent->SetProperty(PropertyId::TextTransform, Property(Style::TextTransform::None));
	parent->SetClientArea(BoxArea::Content);

	parent->AddEventListener(EventId::Keydown, this, true);
	parent->AddEventListener(EventId::Textinput, this, true);
	parent->AddEventListener(EventId::Focus, this, true);
	parent->AddEventListener(EventId::Blur, this, true);
	parent->AddEventListener(EventId::Mousedown, this, true);
	parent->AddEventListener(EventId::Dblclick, this, true);
	parent->AddEventListener(EventId::Drag, this, true);

	ElementPtr unique_text = Factory::InstanceElement(parent, "#text", "#text", XMLAttributes());
	text_element = rmlui_dynamic_cast<ElementText*>(unique_text.get());
	ElementPtr unique_selected_text = Factory::InstanceElement(parent, "#text", "#text", XMLAttributes());
	selected_text_element = rmlui_dynamic_cast<ElementText*>(unique_selected_text.get());
	if (text_element)
	{
		text_element->SuppressAutoLayout();
		parent->AppendChild(std::move(unique_text), false);

		selected_text_element->SuppressAutoLayout();
		parent->AppendChild(std::move(unique_selected_text), false);
	}

	// Create the dummy selection element.
	ElementPtr unique_selection = Factory::InstanceElement(parent, "#selection", "selection", XMLAttributes());
	if (ElementTextSelection* text_selection_element = rmlui_dynamic_cast<ElementTextSelection*>(unique_selection.get()))
	{
		selection_element = text_selection_element;
		text_selection_element->SetWidget(this);
		parent->AppendChild(std::move(unique_selection), false);
	}

	absolute_cursor_index = 0;
	cursor_wrap_down = false;
	ideal_cursor_position_to_the_right_of_cursor = true;
	cancel_next_drag = false;
	force_formatting_on_next_layout = false;

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
	parent->RemoveEventListener(EventId::Keydown, this, true);
	parent->RemoveEventListener(EventId::Textinput, this, true);
	parent->RemoveEventListener(EventId::Focus, this, true);
	parent->RemoveEventListener(EventId::Blur, this, true);
	parent->RemoveEventListener(EventId::Mousedown, this, true);
	parent->RemoveEventListener(EventId::Dblclick, this, true);
	parent->RemoveEventListener(EventId::Drag, this, true);

	// This widget might be parented by an input element, which may now be constructing a completely different type.
	// Thus, remove all properties set by this widget so they don't affect the new type.
	parent->RemoveProperty(PropertyId::WhiteSpace);
	parent->RemoveProperty(PropertyId::OverflowX);
	parent->RemoveProperty(PropertyId::OverflowY);
	parent->RemoveProperty(PropertyId::Drag);
	parent->RemoveProperty(PropertyId::WordBreak);
	parent->RemoveProperty(PropertyId::TextTransform);

	// Remove all the children added by the text widget.
	parent->RemoveChild(text_element);
	parent->RemoveChild(selected_text_element);
	parent->RemoveChild(selection_element);
}

void WidgetTextInput::SetValue(String value)
{
	const size_t initial_size = value.size();
	SanitizeValue(value);

	if (initial_size != value.size())
	{
		parent->SetAttribute("value", value);
		DispatchChangeEvent();
	}
	else
	{
		TransformValue(value);
		RMLUI_ASSERTMSG(value.size() == initial_size, "TransformValue must not change the text length.");

		text_element->SetText(value);

		FormatElement();
		UpdateCursorPosition(true);
	}
}

void WidgetTextInput::TransformValue(String& /*value*/) {}

void WidgetTextInput::SetMaxLength(int _max_length)
{
	if (max_length != _max_length)
	{
		max_length = _max_length;

		String value = GetValue();
		if (ClampValue(value, max_length))
			GetElement()->SetAttribute("value", value);
	}
}

int WidgetTextInput::GetMaxLength() const
{
	return max_length;
}

int WidgetTextInput::GetLength() const
{
	size_t result = StringUtilities::LengthUTF8(GetValue());
	return (int)result;
}

void WidgetTextInput::Select()
{
	SetSelectionRange(0, INT_MAX);
}

void WidgetTextInput::SetSelectionRange(int selection_start, int selection_end)
{
	if (!IsFocused())
		return;

	const String& value = GetValue();
	const int byte_start = ConvertCharacterOffsetToByteOffset(value, selection_start);
	const int byte_end = ConvertCharacterOffsetToByteOffset(value, selection_end);
	const bool is_selecting = (byte_start != byte_end);

	cursor_wrap_down = true;
	absolute_cursor_index = byte_end;

	bool selection_changed = false;
	if (is_selecting)
	{
		selection_anchor_index = byte_start;
		selection_changed = UpdateSelection(true);
	}
	else
	{
		selection_changed = UpdateSelection(false);
	}

	UpdateCursorPosition(true);
	ShowCursor(true, true);

	if (selection_changed)
		FormatText();
}

void WidgetTextInput::GetSelection(int* selection_start, int* selection_end, String* selected_text) const
{
	const String& value = GetValue();
	if (selection_start)
		*selection_start = ConvertByteOffsetToCharacterOffset(value, selection_begin_index);
	if (selection_end)
		*selection_end = ConvertByteOffsetToCharacterOffset(value, selection_begin_index + selection_length);
	if (selected_text)
		*selected_text = value.substr(Math::Min((size_t)selection_begin_index, (size_t)value.size()), (size_t)selection_length);
}

void WidgetTextInput::UpdateSelectionColours()
{
	// Determine what the colour of the selected text is. If our 'selection' element has the 'color'
	// attribute set, then use that. Otherwise, use the inverse of our own text colour.
	Colourb colour;
	const Property* colour_property = selection_element->GetLocalProperty("color");
	if (colour_property != nullptr)
		colour = colour_property->Get<Colourb>();
	else
	{
		colour = parent->GetComputedValues().color();
		colour.red = 255 - colour.red;
		colour.green = 255 - colour.green;
		colour.blue = 255 - colour.blue;
	}

	// Set the computed text colour on the element holding the selected text.
	selected_text_element->SetProperty(PropertyId::Color, Property(colour, Unit::COLOUR));

	// If the 'background-color' property has been set on the 'selection' element, use that as the
	// background colour for the selected text. Otherwise, use the inverse of the selected text
	// colour.
	colour_property = selection_element->GetLocalProperty("background-color");
	if (colour_property != nullptr)
		selection_colour = colour_property->Get<Colourb>();
	else
		selection_colour = Colourb(255 - colour.red, 255 - colour.green, 255 - colour.blue, colour.alpha);

	// Color may have changed, so we update the cursor geometry.
	GenerateCursor();
}

void WidgetTextInput::OnUpdate()
{
	if (cursor_timer > 0)
	{
		double current_time = Clock::GetElapsedTime();
		cursor_timer -= float(current_time - last_update_time);
		last_update_time = current_time;

		while (cursor_timer <= 0)
		{
			cursor_timer += CURSOR_BLINK_TIME;
			cursor_visible = !cursor_visible;
		}

		if (parent->IsVisible(true))
		{
			if (Context* ctx = parent->GetContext())
				ctx->RequestNextUpdate(cursor_timer);
		}
	}
}

void WidgetTextInput::OnResize()
{
	GenerateCursor();

	Vector2f text_position = parent->GetBox().GetPosition(BoxArea::Content);
	text_element->SetOffset(text_position, parent);
	selected_text_element->SetOffset(text_position, parent);

	ForceFormattingOnNextLayout();
}

void WidgetTextInput::OnRender()
{
	ElementUtilities::SetClippingRegion(text_element);

	Vector2f text_translation = parent->GetAbsoluteOffset() - Vector2f(parent->GetScrollLeft(), parent->GetScrollTop());
	selection_geometry.Render(text_translation);

	if (cursor_visible && !parent->IsDisabled())
	{
		cursor_geometry.Render(text_translation + cursor_position);
	}
}

void WidgetTextInput::OnLayout()
{
	if (force_formatting_on_next_layout)
	{
		internal_dimensions = parent->GetBox().GetSize(BoxArea::Content);
		FormatElement();
		UpdateCursorPosition(true);
		force_formatting_on_next_layout = false;
	}

	parent->SetScrollLeft(scroll_offset.x);
	parent->SetScrollTop(scroll_offset.y);
}

Element* WidgetTextInput::GetElement() const
{
	return parent;
}

bool WidgetTextInput::IsFocused() const
{
	return cursor_timer > 0;
}

void WidgetTextInput::DispatchChangeEvent(bool linebreak)
{
	Dictionary parameters;
	parameters["value"] = GetAttributeValue();
	parameters["linebreak"] = Variant(linebreak);
	GetElement()->DispatchEvent(EventId::Change, parameters);
}

void WidgetTextInput::ProcessEvent(Event& event)
{
	if (parent->IsDisabled())
		return;

	switch (event.GetId())
	{
	case EventId::Keydown:
	{
		Input::KeyIdentifier key_identifier = (Input::KeyIdentifier)event.GetParameter<int>("key_identifier", 0);
		bool numlock = event.GetParameter<int>("num_lock_key", 0) > 0;
		bool shift = event.GetParameter<int>("shift_key", 0) > 0;
		bool ctrl = event.GetParameter<int>("ctrl_key", 0) > 0;
		bool alt = event.GetParameter<int>("alt_key", 0) > 0;
		bool selection_changed = false;
		bool out_of_bounds = false;

		switch (key_identifier)
		{
			// clang-format off
		case Input::KI_NUMPAD4: if (numlock) break; //-fallthrough
		case Input::KI_LEFT:    selection_changed = MoveCursorHorizontal(ctrl ? CursorMovement::PreviousWord : CursorMovement::Left, shift, out_of_bounds); break;

		case Input::KI_NUMPAD6: if (numlock) break; //-fallthrough
		case Input::KI_RIGHT:   selection_changed = MoveCursorHorizontal(ctrl ? CursorMovement::NextWord : CursorMovement::Right, shift, out_of_bounds); break;

		case Input::KI_NUMPAD8: if (numlock) break; //-fallthrough
		case Input::KI_UP:      selection_changed = MoveCursorVertical(-1, shift, out_of_bounds); break;

		case Input::KI_NUMPAD2: if (numlock) break; //-fallthrough
		case Input::KI_DOWN:    selection_changed = MoveCursorVertical(1, shift, out_of_bounds); break;

		case Input::KI_NUMPAD7: if (numlock) break; //-fallthrough
		case Input::KI_HOME:    selection_changed = MoveCursorHorizontal(ctrl ? CursorMovement::Begin : CursorMovement::BeginLine, shift, out_of_bounds); break;

		case Input::KI_NUMPAD1: if (numlock) break; //-fallthrough
		case Input::KI_END:     selection_changed = MoveCursorHorizontal(ctrl ? CursorMovement::End : CursorMovement::EndLine, shift, out_of_bounds); break;

		case Input::KI_NUMPAD9: if (numlock) break; //-fallthrough
		case Input::KI_PRIOR:   selection_changed = MoveCursorVertical(-int(internal_dimensions.y / parent->GetLineHeight()) + 1, shift, out_of_bounds); break;

		case Input::KI_NUMPAD3: if (numlock) break; //-fallthrough
		case Input::KI_NEXT:    selection_changed = MoveCursorVertical(int(internal_dimensions.y / parent->GetLineHeight()) - 1, shift, out_of_bounds); break;

		case Input::KI_BACK:
		{
			CursorMovement direction = (ctrl ? CursorMovement::PreviousWord : CursorMovement::Left);
			DeleteCharacters(direction);
			ShowCursor(true);
		}
		break;

		case Input::KI_DECIMAL:	if (numlock) break; //-fallthrough
		case Input::KI_DELETE:
		{
			CursorMovement direction = (ctrl ? CursorMovement::NextWord : CursorMovement::Right);
			DeleteCharacters(direction);
			ShowCursor(true);
		}
		break;
			// clang-format on

		case Input::KI_NUMPADENTER:
		case Input::KI_RETURN:
		{
			LineBreak();
		}
		break;

		case Input::KI_A:
		{
			if (ctrl)
				Select();
		}
		break;

		case Input::KI_C:
		{
			if (ctrl && selection_length > 0)
				CopySelection();
		}
		break;

		case Input::KI_X:
		{
			if (ctrl && selection_length > 0)
			{
				CopySelection();
				DeleteSelection();
				DispatchChangeEvent();
				ShowCursor(true);
			}
		}
		break;

		case Input::KI_V:
		{
			if (ctrl && !alt)
			{
				String clipboard_text;
				GetSystemInterface()->GetClipboardText(clipboard_text);

				AddCharacters(clipboard_text);
				ShowCursor(true);
			}
		}
		break;

		// Ignore tabs so input fields can be navigated through with keys.
		case Input::KI_TAB: return;

		default: break;
		}

		if (!out_of_bounds || selection_changed)
			event.StopPropagation();
		if (selection_changed)
			FormatText();
	}
	break;

	case EventId::Textinput:
	{
		// Only process the text if no modifier keys are pressed.
		if (event.GetParameter<int>("ctrl_key", 0) == 0 && event.GetParameter<int>("alt_key", 0) == 0 && event.GetParameter<int>("meta_key", 0) == 0)
		{
			String text = event.GetParameter("text", String{});
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
			parent->SetPseudoClass("focus-visible", true);
			if (UpdateSelection(false))
				FormatElement();
			ShowCursor(true, false);
		}
	}
	break;
	case EventId::Blur:
	{
		if (event.GetTargetElement() == parent)
		{
			if (ClearSelection())
				FormatElement();
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
		//-fallthrough
	case EventId::Mousedown:
	{
		if (event.GetTargetElement() == parent)
		{
			Vector2f mouse_position = Vector2f(event.GetParameter<float>("mouse_x", 0), event.GetParameter<float>("mouse_y", 0));
			mouse_position -= text_element->GetAbsoluteOffset();

			const int cursor_line_index = CalculateLineIndex(mouse_position.y);
			const int cursor_character_index = CalculateCharacterIndex(cursor_line_index, mouse_position.x);

			SetCursorFromRelativeIndices(cursor_line_index, cursor_character_index);

			MoveCursorToCharacterBoundaries(false);
			UpdateCursorPosition(true);

			if (UpdateSelection(event == EventId::Drag || event.GetParameter<int>("shift_key", 0) > 0))
				FormatText();

			const bool move_to_cursor = (event == EventId::Drag);
			ShowCursor(true, move_to_cursor);
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

	default: break;
	}
}

bool WidgetTextInput::AddCharacters(String string)
{
	SanitizeValue(string);

	if (selection_length > 0)
		DeleteSelection();

	if (max_length >= 0)
		ClampValue(string, Math::Max(max_length - GetLength(), 0));

	if (string.empty())
		return false;

	String value = GetAttributeValue();
	value.insert(std::min<size_t>((size_t)absolute_cursor_index, value.size()), string);

	absolute_cursor_index += (int)string.size();
	parent->SetAttribute("value", value);

	if (UpdateSelection(false))
		FormatText();

	DispatchChangeEvent();
	return true;
}

bool WidgetTextInput::DeleteCharacters(CursorMovement direction)
{
	bool out_of_bounds;
	// We set a selection of characters according to direction, and then delete it.
	// If we already have a selection, we delete that first.
	if (selection_length <= 0)
		MoveCursorHorizontal(direction, true, out_of_bounds);

	if (selection_length > 0)
	{
		DeleteSelection();
		DispatchChangeEvent();

		return true;
	}

	return false;
}

void WidgetTextInput::CopySelection()
{
	const String& value = GetValue();
	const String snippet = value.substr(Math::Min((size_t)selection_begin_index, (size_t)value.size()), (size_t)selection_length);
	GetSystemInterface()->SetClipboardText(snippet);
}

bool WidgetTextInput::MoveCursorHorizontal(CursorMovement movement, bool select, bool& out_of_bounds)
{
	out_of_bounds = false;

	const String& value = GetValue();

	int cursor_line_index = 0, cursor_character_index = 0;
	GetRelativeCursorIndices(cursor_line_index, cursor_character_index);

	// By default the cursor wraps down when located on softbreaks. This may be overridden by setting the cursor using relative indices.
	cursor_wrap_down = true;

	// Whether to seek forward or back to align to utf8 boundaries later.
	bool seek_forward = false;

	switch (movement)
	{
	case CursorMovement::Begin: absolute_cursor_index = 0; break;
	case CursorMovement::BeginLine: SetCursorFromRelativeIndices(cursor_line_index, 0); break;
	case CursorMovement::PreviousWord:
	{
		// First skip whitespace, then skip all characters of the same class as the first non-whitespace character.
		CharacterClass skip_character_class = CharacterClass::Whitespace;
		const char* p_rend = value.data();
		const char* p_rbegin = p_rend + absolute_cursor_index;
		const char* p = p_rbegin - 1;
		for (; p > p_rend; --p)
		{
			const CharacterClass character_class = GetCharacterClass(*p);
			if (character_class != skip_character_class)
			{
				if (skip_character_class == CharacterClass::Whitespace)
					skip_character_class = character_class;
				else
					break;
			}
		}
		if (p != p_rend)
			++p;
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
	{
		// First skip all characters of the same class as the first character, then skip any whitespace.
		CharacterClass skip_character_class = CharacterClass::Undefined;
		const char* p_begin = value.data() + absolute_cursor_index;
		const char* p_end = value.data() + value.size();
		const char* p = p_begin;
		for (; p < p_end; ++p)
		{
			const CharacterClass character_class = GetCharacterClass(*p);
			if (skip_character_class == CharacterClass::Undefined)
				skip_character_class = character_class;

			if (character_class != skip_character_class)
			{
				if (character_class == CharacterClass::Whitespace)
					skip_character_class = CharacterClass::Whitespace;
				else
					break;
			}
		}
		absolute_cursor_index += int(p - p_begin);
	}
	break;
	case CursorMovement::EndLine: SetCursorFromRelativeIndices(cursor_line_index, lines[cursor_line_index].editable_length); break;
	case CursorMovement::End: absolute_cursor_index = INT_MAX; break;
	}

	const int unclamped_absolute_cursor_index = absolute_cursor_index;
	absolute_cursor_index = Math::Clamp(absolute_cursor_index, 0, (int)GetValue().size());
	out_of_bounds = (unclamped_absolute_cursor_index != absolute_cursor_index);

	MoveCursorToCharacterBoundaries(seek_forward);
	UpdateCursorPosition(true);

	bool selection_changed = UpdateSelection(select);
	ShowCursor(true);

	return selection_changed;
}

bool WidgetTextInput::MoveCursorVertical(int distance, bool select, bool& out_of_bounds)
{
	int cursor_line_index = 0, cursor_character_index = 0;
	out_of_bounds = false;
	GetRelativeCursorIndices(cursor_line_index, cursor_character_index);

	cursor_line_index += distance;

	if (cursor_line_index < 0)
	{
		out_of_bounds = true;
		cursor_line_index = 0;
		cursor_character_index = 0;
	}
	else if (cursor_line_index >= (int)lines.size())
	{
		out_of_bounds = true;
		cursor_line_index = (int)lines.size() - 1;
		cursor_character_index = (int)lines[cursor_line_index].editable_length;
	}
	else
		cursor_character_index = CalculateCharacterIndex(cursor_line_index, ideal_cursor_position);

	SetCursorFromRelativeIndices(cursor_line_index, cursor_character_index);

	MoveCursorToCharacterBoundaries(false);
	UpdateCursorPosition(false);

	bool selection_changed = UpdateSelection(select);
	ShowCursor(true);

	return selection_changed;
}

void WidgetTextInput::MoveCursorToCharacterBoundaries(bool forward)
{
	const String& value = GetValue();
	absolute_cursor_index = Math::Min(absolute_cursor_index, (int)value.size());

	const char* p_begin = value.data();
	const char* p_end = p_begin + value.size();
	const char* p_cursor = p_begin + absolute_cursor_index;
	const char* p = p_cursor;

	if (forward)
		p = StringUtilities::SeekForwardUTF8(p_cursor, p_end);
	else
		p = StringUtilities::SeekBackwardUTF8(p_cursor, p_begin);

	if (p != p_cursor)
		absolute_cursor_index += int(p - p_cursor);
}

void WidgetTextInput::ExpandSelection()
{
	const String& value = GetValue();
	const char* const p_begin = value.data();
	const char* const p_end = p_begin + value.size();
	const char* const p_index = p_begin + absolute_cursor_index;

	// The first character encountered defines the character class to expand.
	CharacterClass expanding_character_class = CharacterClass::Undefined;

	auto character_is_wrong_type = [&expanding_character_class](const char* p) -> bool {
		const CharacterClass character_class = GetCharacterClass(*p);
		if (expanding_character_class == CharacterClass::Undefined)
			expanding_character_class = character_class;
		else if (character_class != expanding_character_class)
			return true;
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

	if (ideal_cursor_position_to_the_right_of_cursor)
	{
		p_right = search_right();
		p_left = search_left();
	}
	else
	{
		p_left = search_left();
		p_right = search_right();
	}

	cursor_wrap_down = true;
	absolute_cursor_index -= int(p_index - p_left);
	MoveCursorToCharacterBoundaries(false);
	bool selection_changed = UpdateSelection(false);

	absolute_cursor_index += int(p_right - p_left);
	MoveCursorToCharacterBoundaries(true);
	selection_changed |= UpdateSelection(true);

	if (selection_changed)
		FormatText();

	UpdateCursorPosition(true);
}

const String& WidgetTextInput::GetValue() const
{
	return text_element->GetText();
}

String WidgetTextInput::GetAttributeValue() const
{
	return parent->GetAttribute("value", String());
}

void WidgetTextInput::GetRelativeCursorIndices(int& out_cursor_line_index, int& out_cursor_character_index) const
{
	int line_begin = 0;

	for (size_t i = 0; i < lines.size(); i++)
	{
		const int cursor_relative_line_end = absolute_cursor_index - (line_begin + lines[i].editable_length);

		// Test if the absolute index is located on the editable part of this line, otherwise we wrap down to the next line. We may have additional
		// characters after the editable length, such as the newline character '\n'. We also wrap down if the cursor is located to the right of any
		// such characters.
		if (cursor_relative_line_end <= 0)
		{
			const bool soft_wrapped_line = (lines[i].editable_length == lines[i].size);

			// If we are located exactly on a soft break (due to word wrapping) then the cursor wrap state determines whether or not we wrap down.
			if (cursor_relative_line_end == 0 && soft_wrapped_line && cursor_wrap_down && (int)i + 1 < (int)lines.size())
			{
				out_cursor_line_index = (int)i + 1;
				out_cursor_character_index = 0;
			}
			else
			{
				out_cursor_line_index = (int)i;
				out_cursor_character_index = Math::Max(absolute_cursor_index - line_begin, 0);
			}
			return;
		}

		line_begin += lines[i].size;
	}

	// We shouldn't ever get here; this means we actually couldn't find where the absolute cursor said it was. So we'll
	// just set the relative cursors to the very end of the text field.
	out_cursor_line_index = (int)lines.size() - 1;
	out_cursor_character_index = lines[out_cursor_line_index].editable_length;
}

void WidgetTextInput::SetCursorFromRelativeIndices(int cursor_line_index, int cursor_character_index)
{
	RMLUI_ASSERT(cursor_line_index < (int)lines.size())

	absolute_cursor_index = cursor_character_index;

	for (int i = 0; i < cursor_line_index; i++)
		absolute_cursor_index += lines[i].size;

	// Don't wrap down if we're located at the end of the line.
	cursor_wrap_down = !(cursor_character_index >= lines[cursor_line_index].editable_length);
}

int WidgetTextInput::CalculateLineIndex(float position) const
{
	float line_height = parent->GetLineHeight();
	int line_index = int(position / line_height);
	return Math::Clamp(line_index, 0, (int)(lines.size() - 1));
}

float WidgetTextInput::GetAlignmentSpecificTextOffset(const char* p_begin, int line_index) const
{
	const float client_width = parent->GetClientWidth();
	const float total_width = (float)ElementUtilities::GetStringWidth(text_element, String(p_begin, lines[line_index].editable_length));
	auto text_align = GetElement()->GetComputedValues().text_align();

	// offset position depending on text align
	switch (text_align)
	{
	case Style::TextAlign::Right: return Math::Max(0.0f, (client_width - total_width));
	case Style::TextAlign::Center: return Math::Max(0.0f, ((client_width - total_width) / 2));
	default: break;
	}

	return 0;
}

int WidgetTextInput::CalculateCharacterIndex(int line_index, float position)
{
	int prev_offset = 0;
	float prev_line_width = 0;

	ideal_cursor_position_to_the_right_of_cursor = true;

	const char* p_begin = GetValue().data() + lines[line_index].value_offset;

	position -= GetAlignmentSpecificTextOffset(p_begin, line_index);

	for (auto it = StringIteratorU8(p_begin, p_begin, p_begin + lines[line_index].editable_length); it;)
	{
		++it;
		const int offset = (int)it.offset();

		const float line_width = (float)ElementUtilities::GetStringWidth(text_element, String(p_begin, (size_t)offset));
		if (line_width > position)
		{
			if (position - prev_line_width < line_width - position)
			{
				return prev_offset;
			}
			else
			{
				ideal_cursor_position_to_the_right_of_cursor = false;
				return offset;
			}
		}

		prev_line_width = line_width;
		prev_offset = offset;
	}

	return prev_offset;
}

void WidgetTextInput::ShowCursor(bool show, bool move_to_cursor)
{
	if (show)
	{
		cursor_visible = true;
		cursor_timer = CURSOR_BLINK_TIME;
		last_update_time = GetSystemInterface()->GetElapsedTime();

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

		SetKeyboardActive(true);
		keyboard_showed = true;
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

void WidgetTextInput::FormatElement()
{
	using namespace Style;
	ElementScroll* scroll = parent->GetElementScroll();
	float width = parent->GetBox().GetSize(BoxArea::Padding).x;

	const Overflow x_overflow_property = parent->GetComputedValues().overflow_x();
	const Overflow y_overflow_property = parent->GetComputedValues().overflow_y();
	const bool word_wrap = (parent->GetComputedValues().white_space() == WhiteSpace::Prewrap);

	if (x_overflow_property == Overflow::Scroll)
		scroll->EnableScrollbar(ElementScroll::HORIZONTAL, width);
	else
		scroll->DisableScrollbar(ElementScroll::HORIZONTAL);

	if (y_overflow_property == Overflow::Scroll)
		scroll->EnableScrollbar(ElementScroll::VERTICAL, width);
	else
		scroll->DisableScrollbar(ElementScroll::VERTICAL);

	// If the formatting produces scrollbars we need to format again later, this constraint enables early exit for the first formatting round.
	const float formatting_height_constraint = (y_overflow_property == Overflow::Auto ? parent->GetClientHeight() : FLT_MAX);

	// Format the text and determine its total area.
	Vector2f content_area = FormatText(formatting_height_constraint);

	// If we're set to automatically generate horizontal scrollbars, check for that now.
	if (!word_wrap && x_overflow_property == Overflow::Auto && content_area.x > parent->GetClientWidth())
		scroll->EnableScrollbar(ElementScroll::HORIZONTAL, width);

	// Now check for vertical overflow. If we do turn on the scrollbar, this will cause a reflow.
	if (y_overflow_property == Overflow::Auto && content_area.y > parent->GetClientHeight())
	{
		scroll->EnableScrollbar(ElementScroll::VERTICAL, width);
		content_area = FormatText();

		if (!word_wrap && x_overflow_property == Overflow::Auto && content_area.x > parent->GetClientWidth())
			scroll->EnableScrollbar(ElementScroll::HORIZONTAL, width);
	}

	parent->SetScrollableOverflowRectangle(content_area);
	scroll->FormatScrollbars();
}

Vector2f WidgetTextInput::FormatText(float height_constraint)
{
	Vector2f content_area(0, 0);

	const FontFaceHandle font_handle = parent->GetFontFaceHandle();
	if (!font_handle)
		return content_area;

	// Clear the old lines, and all the lines in the text elements.
	lines.clear();
	text_element->ClearLines();
	selected_text_element->ClearLines();

	// Clear the selection background geometry, and get the vertices and indices so the new geo can
	// be generated.
	selection_geometry.Release(true);
	Vector<Vertex>& selection_vertices = selection_geometry.GetVertices();
	Vector<int>& selection_indices = selection_geometry.GetIndices();

	// Determine the line-height of the text element.
	const float line_height = parent->GetLineHeight();
	const float font_baseline = GetFontEngineInterface()->GetFontMetrics(font_handle).ascent;
	// When the selection contains endlines we expand the selection area by this width.
	const int endline_selection_width = int(0.4f * parent->GetComputedValues().font_size());

	const float client_width = parent->GetClientWidth();
	int line_begin = 0;
	Vector2f line_position(0, font_baseline);
	bool last_line = false;

	auto text_align = GetElement()->GetComputedValues().text_align();

	struct Segment {
		Vector2f position;
		int width;
		String content;
		bool selected;
		int line_index;
	};

	Vector<Segment> segments;

	// Keep generating lines until all the text content is placed.
	do
	{
		if (client_width <= 0.f)
		{
			lines.push_back(Line{});
			break;
		}

		Line line = {};
		line.value_offset = line_begin;
		float line_width;
		String line_content;

		// Generate the next line.
		last_line = text_element->GenerateLine(line_content, line.size, line_width, line_begin, client_width - cursor_size.x, 0, false, false, false);

		// If this line terminates in a soft-return (word wrap), then the line may be leaving a space or two behind as an orphan. If so, we must
		// append the orphan onto the line even though it will push the line outside of the input field's bounds.
		String orphan;
		if (!last_line && (line_content.empty() || line_content.back() != '\n'))
		{
			const String& text = GetValue();
			for (int i = 1; i >= 0; --i)
			{
				int index = line_begin + line.size + i;
				if (index >= (int)text.size())
					continue;

				if (text[index] != ' ')
				{
					orphan.clear();
					continue;
				}

				int next_index = index + 1;
				if (!orphan.empty() || next_index >= (int)text.size() || text[next_index] != ' ')
					orphan += ' ';
			}
		}

		if (!orphan.empty())
		{
			line_content += orphan;
			line.size += (int)orphan.size();
			line_width += ElementUtilities::GetStringWidth(text_element, orphan);
		}

		// visually remove trailing space if right aligned
		if (!last_line && text_align == Style::TextAlign::Right && !line_content.empty() && line_content.back() == ' ')
		{
			line_content.pop_back();
			line_width -= ElementUtilities::GetStringWidth(text_element, " ");
		}

		// Now that we have the string of characters appearing on the new line, we split it into
		// three parts; the unselected text appearing before any selected text on the line, the
		// selected text on the line, and any unselected text after the selection.
		String pre_selection, selection, post_selection;
		GetLineSelection(pre_selection, selection, post_selection, line_content, line_begin);

		// The pre-selected text is placed, if there is any (if the selection starts on or before
		// the beginning of this line, then this will be empty).
		if (!pre_selection.empty())
		{
			const int width = ElementUtilities::GetStringWidth(text_element, pre_selection);
			segments.push_back({line_position, width, pre_selection, false, (int)lines.size()});
			line_position.x += width;
		}

		// Return the extra kerning that would result in joining two strings.
		auto GetKerningBetween = [this](const String& left, const String& right) -> float {
			if (left.empty() || right.empty())
				return 0.0f;
			// We could join the whole string, and compare the result of the joined width to the individual widths of each string. Instead, we take
			// the two neighboring characters from each string and compare the string width with and without kerning, which should be much faster.
			const Character left_back = StringUtilities::ToCharacter(StringUtilities::SeekBackwardUTF8(&left.back(), &left.front()));
			const String right_front_u8 =
				right.substr(0, size_t(StringUtilities::SeekForwardUTF8(right.c_str() + 1, right.c_str() + right.size()) - right.c_str()));
			const int width_kerning = ElementUtilities::GetStringWidth(text_element, right_front_u8, left_back);
			const int width_no_kerning = ElementUtilities::GetStringWidth(text_element, right_front_u8, Character::Null);
			return float(width_kerning - width_no_kerning);
		};

		// Check if the editable length needs to be truncated to dodge a trailing endline.
		line.editable_length = (int)line_content.size();
		if (!line_content.empty() && line_content.back() == '\n')
			line.editable_length -= 1;

		// If there is any selected text on this line, place it in the selected text element and
		// generate the geometry for its background.
		if (!selection.empty())
		{
			line_position.x += GetKerningBetween(pre_selection, selection);
			const int selection_width = ElementUtilities::GetStringWidth(selected_text_element, selection);
			segments.push_back({line_position, selection_width, selection, true, (int)lines.size()});

			line_position.x += selection_width;
		}

		// If there is any unselected text after the selection on this line, place it in the
		// standard text element after the selected text.
		if (!post_selection.empty())
		{
			line_position.x += GetKerningBetween(selection, post_selection);
			const int width = ElementUtilities::GetStringWidth(text_element, post_selection);
			segments.push_back({line_position, width, post_selection, false, (int)lines.size()});
		}

		// Update variables for the next line.
		line_begin += line.size;
		line_position.x = 0;
		line_position.y += line_height;

		// Grow the content area width-wise if this line is the longest so far, and push the height out.
		content_area.x = Math::Max(content_area.x, line_width + cursor_size.x);
		content_area.y = line_position.y - font_baseline;

		// Finally, push the new line into our array of lines.
		lines.push_back(std::move(line));

	} while (!last_line && content_area.y <= height_constraint);

	// Clamp the cursor to a valid range.
	absolute_cursor_index = Math::Min(absolute_cursor_index, (int)GetValue().size());

	// Transform segments according to text alignment
	for (auto& it : segments)
	{
		auto const& line = lines[it.line_index];
		const char* p_begin = GetValue().data() + line.value_offset;
		float offset = GetAlignmentSpecificTextOffset(p_begin, it.line_index);

		it.position.x += offset;

		if (it.selected)
		{
			const bool selection_contains_endline = (selection_begin_index + selection_length > line_begin + lines[it.line_index].editable_length);
			const Vector2f selection_size(float(it.width + (selection_contains_endline ? endline_selection_width : 0)), line_height);

			selection_vertices.resize(selection_vertices.size() + 4);
			selection_indices.resize(selection_indices.size() + 6);
			GeometryUtilities::GenerateQuad(&selection_vertices[selection_vertices.size() - 4], &selection_indices[selection_indices.size() - 6],
				it.position - Vector2f(0, font_baseline), selection_size, selection_colour, (int)selection_vertices.size() - 4);

			selected_text_element->AddLine(it.position, it.content);
		}
		else
			text_element->AddLine(it.position, it.content);
	}

	return content_area;
}

void WidgetTextInput::GenerateCursor()
{
	// Generates the cursor.
	cursor_geometry.Release();

	Vector<Vertex>& vertices = cursor_geometry.GetVertices();
	vertices.resize(4);

	Vector<int>& indices = cursor_geometry.GetIndices();
	indices.resize(6);

	cursor_size.x = Math::Round(ElementUtilities::GetDensityIndependentPixelRatio(text_element));
	cursor_size.y = text_element->GetLineHeight() + 2.0f;

	Colourb color = parent->GetComputedValues().color();

	if (const Property* property = parent->GetProperty(PropertyId::CaretColor))
	{
		if (property->unit == Unit::COLOUR)
			color = property->Get<Colourb>();
	}

	GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), cursor_size, color);
}

void WidgetTextInput::ForceFormattingOnNextLayout()
{
	force_formatting_on_next_layout = true;
}

void WidgetTextInput::UpdateCursorPosition(bool update_ideal_cursor_position)
{
	if (text_element->GetFontFaceHandle() == 0 || lines.empty())
		return;

	int cursor_line_index = 0, cursor_character_index = 0;
	GetRelativeCursorIndices(cursor_line_index, cursor_character_index);

	auto const& line = lines[cursor_line_index];
	const char* p_begin = GetValue().data() + line.value_offset;

	cursor_position.x = (float)ElementUtilities::GetStringWidth(text_element, String(p_begin, cursor_character_index));
	cursor_position.y = -1.f + (float)cursor_line_index * text_element->GetLineHeight();

	cursor_position.x += GetAlignmentSpecificTextOffset(p_begin, cursor_line_index);

	if (update_ideal_cursor_position)
		ideal_cursor_position = cursor_position.x;
}

bool WidgetTextInput::UpdateSelection(bool selecting)
{
	bool selection_changed = false;
	if (!selecting)
	{
		selection_anchor_index = selection_begin_index = absolute_cursor_index;
		selection_changed = ClearSelection();
	}
	else
	{
		int new_begin_index;
		int new_end_index;

		if (absolute_cursor_index > selection_anchor_index)
		{
			new_begin_index = selection_anchor_index;
			new_end_index = absolute_cursor_index;
		}
		else
		{
			new_begin_index = absolute_cursor_index;
			new_end_index = selection_anchor_index;
		}

		if (new_begin_index != selection_begin_index || new_end_index - new_begin_index != selection_length)
		{
			selection_begin_index = new_begin_index;
			selection_length = new_end_index - new_begin_index;

			selection_changed = true;
		}
	}

	return selection_changed;
}

bool WidgetTextInput::ClearSelection()
{
	if (selection_length > 0)
	{
		selection_length = 0;
		return true;
	}
	return false;
}

void WidgetTextInput::DeleteSelection()
{
	if (selection_length > 0)
	{
		String new_value = GetAttributeValue();
		const size_t selection_begin = std::min((size_t)selection_begin_index, (size_t)new_value.size());
		new_value.erase(selection_begin, (size_t)selection_length);

		// Move the cursor to the beginning of the old selection.
		absolute_cursor_index = selection_begin_index;

		GetElement()->SetAttribute("value", new_value);

		// Erase our record of the selection.
		if (UpdateSelection(false))
			FormatText();
	}
}

void WidgetTextInput::GetLineSelection(String& pre_selection, String& selection, String& post_selection, const String& line, int line_begin) const
{
	const int selection_end = selection_begin_index + selection_length;

	// Check if we have any selection at all, and if so if the selection is on this line.
	if (selection_length <= 0 || selection_end < line_begin || selection_begin_index > line_begin + (int)line.size())
	{
		pre_selection = line;
		return;
	}

	const int line_length = (int)line.size();
	using namespace Math;

	// Split the line up into its three parts, depending on the size and placement of the selection.
	pre_selection = line.substr(0, Max(0, selection_begin_index - line_begin));
	selection =
		line.substr(Clamp(selection_begin_index - line_begin, 0, line_length), Max(0, selection_length + Min(0, selection_begin_index - line_begin)));
	post_selection = line.substr(Clamp(selection_end - line_begin, 0, line_length));
}

void WidgetTextInput::SetKeyboardActive(bool active)
{
	if (SystemInterface* system = GetSystemInterface())
	{
		if (active)
		{
			// Activate the keyboard and submit the cursor position and line height to enable clients to adjust the input method editor (IME). Note
			// that the cursor is extended by one pixel along the top and bottom, we reverse this extension here.
			const Vector2f element_offset = parent->GetAbsoluteOffset() - scroll_offset;
			const Vector2f absolute_cursor_position = element_offset + cursor_position + Vector2f(0, 1);
			const float line_height = cursor_size.y - 2.f;
			system->ActivateKeyboard(absolute_cursor_position, line_height);
		}
		else
		{
			system->DeactivateKeyboard();
		}
	}
}

} // namespace Rml
