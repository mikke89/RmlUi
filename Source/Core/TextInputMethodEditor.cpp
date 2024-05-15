/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#include "../../Include/RmlUi/Core/TextInputMethodEditor.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/TextInputMethodContext.h"

namespace Rml {

class DefaultTextInputMethodEditor final : public TextInputMethodEditor {
public:
	DefaultTextInputMethodEditor();

	virtual bool IsNativeCompositionBlocked() const override;

	virtual void ActivateContext(SharedPtr<TextInputMethodContext> context) override;
	virtual void DeactivateContext(TextInputMethodContext* context) override;

	virtual bool IsComposing() const override;

	virtual void StartComposition() override;
	virtual void CancelComposition() override;

	virtual void SetComposition(StringView composition) override;
	virtual void ConfirmComposition(StringView composition) override;

	virtual void SetCursorPosition(int cursor_pos, bool update) override;

private:
	void EndComposition();
	void SetCompositionString(StringView composition);

	void UpdateCursorPosition();

private:
	// An actively used text input method context.
	WeakPtr<TextInputMethodContext> context;

	// A flag to mark a composition is currently active.
	bool composing;
	// Character position of the cursor in the composition string.
	int cursor_pos;

	// Composition range (character position) relative to the text input value.
	int composition_range_start;
	int composition_range_end;
};

DefaultTextInputMethodEditor::DefaultTextInputMethodEditor() : composing(false), cursor_pos(-1), composition_range_start(0), composition_range_end(0)
{}

bool DefaultTextInputMethodEditor::IsNativeCompositionBlocked() const
{
	return true;
}

void DefaultTextInputMethodEditor::ActivateContext(SharedPtr<TextInputMethodContext> _context)
{
	context = _context;
}

void DefaultTextInputMethodEditor::DeactivateContext(TextInputMethodContext* _context)
{
	if (context.lock().get() == _context)
		context.reset();
}

bool DefaultTextInputMethodEditor::IsComposing() const
{
	return composing;
}

void DefaultTextInputMethodEditor::StartComposition()
{
	RMLUI_ASSERT(!composing);
	composing = true;
}

void DefaultTextInputMethodEditor::EndComposition()
{
	if (SharedPtr<TextInputMethodContext> _context = context.lock())
		_context->SetCompositionRange(0, 0);

	RMLUI_ASSERT(composing);
	composing = false;

	composition_range_start = 0;
	composition_range_end = 0;
}

void DefaultTextInputMethodEditor::CancelComposition()
{
	RMLUI_ASSERT(IsComposing());

	if (SharedPtr<TextInputMethodContext> _context = context.lock())
	{
		// Purge the current composition string.
		_context->SetText(StringView(), composition_range_start, composition_range_end);
		// Move the cursor back to where the composition began.
		_context->SetCursorPosition(composition_range_start);
	}

	EndComposition();
}

void DefaultTextInputMethodEditor::SetComposition(StringView composition)
{
	RMLUI_ASSERT(IsComposing());

	SetCompositionString(composition);
	UpdateCursorPosition();

	// Update the composition range only if the cursor can be moved around. Editors working with a single
	// character (e.g., Hangul IME) should have no visual feedback; they use a selection range instead.
	if (cursor_pos != -1)
		if (SharedPtr<TextInputMethodContext> _context = context.lock())
			_context->SetCompositionRange(composition_range_start, composition_range_end);
}

void DefaultTextInputMethodEditor::ConfirmComposition(StringView composition)
{
	RMLUI_ASSERT(IsComposing());

	SetCompositionString(composition);

	RMLUI_ASSERT(!context.expired());
	SharedPtr<TextInputMethodContext> _context = context.lock();

	_context->SetCompositionRange(composition_range_start, composition_range_end);
	_context->CommitComposition();

	// Move the cursor to the end of the string.
	SetCursorPosition(composition_range_end - composition_range_start, true);

	EndComposition();
}

void DefaultTextInputMethodEditor::SetCursorPosition(int _cursor_pos, bool update)
{
	RMLUI_ASSERT(IsComposing());

	cursor_pos = _cursor_pos;

	if (update)
		UpdateCursorPosition();
}

void DefaultTextInputMethodEditor::SetCompositionString(StringView composition)
{
	RMLUI_ASSERT(!context.expired());
	SharedPtr<TextInputMethodContext> _context = context.lock();

	// Retrieve the composition range if it is missing.
	if (composition_range_start == 0 && composition_range_end == 0)
		_context->GetSelectionRange(composition_range_start, composition_range_end);

	_context->SetText(composition, composition_range_start, composition_range_end);

	size_t length = StringUtilities::LengthUTF8(composition);
	composition_range_end = composition_range_start + (int)length;
}

void DefaultTextInputMethodEditor::UpdateCursorPosition()
{
	// Cursor position update happens before a composition is set; ignore this event.
	if (composition_range_start == 0 && composition_range_end == 0)
		return;

	RMLUI_ASSERT(!context.expired());
	SharedPtr<TextInputMethodContext> _context = context.lock();

	if (cursor_pos != -1)
	{
		int position = composition_range_start + cursor_pos;
		_context->SetCursorPosition(position);
	}
	else
	{
		// If the API reports no cursor position, select the entire composition string for a better UX.
		_context->SetSelectionRange(composition_range_start, composition_range_end);
	}
}

UniquePtr<TextInputMethodEditor> CreateDefaultTextInputMethodEditor()
{
	return MakeUnique<DefaultTextInputMethodEditor>();
}

} // namespace Rml
