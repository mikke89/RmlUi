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

#include "TextInputMethodContext.h"
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Core/Platform.h>

TextInputMethodContext::~TextInputMethodContext() = default;

class RmlBaseTextInputMethodContext : public TextInputMethodContext {
public:
	RmlBaseTextInputMethodContext(Rml::ElementFormControl* input);
	virtual ~RmlBaseTextInputMethodContext();

	virtual void SetCursorPosition(int position) override;
	virtual void SetText(Rml::StringView text, int start, int end) override;
	virtual void GetScreenBounds(Rml::Vector2f& position, Rml::Vector2f& size) const override;

protected:
	Rml::ElementFormControl* GetControl() const;

private:
	Rml::ObserverPtr<Rml::Element> focused_input;
};

RmlBaseTextInputMethodContext::RmlBaseTextInputMethodContext(Rml::ElementFormControl* input) : focused_input(input->GetObserverPtr()) {}

RmlBaseTextInputMethodContext::~RmlBaseTextInputMethodContext() = default;

void RmlBaseTextInputMethodContext::SetCursorPosition(int position)
{
	SetSelectionRange(position, position);
}

static int ConvertCharacterOffsetToByteOffset(const Rml::String& value, int character_offset)
{
	if (character_offset >= (int)value.size())
		return (int)value.size();

	int character_count = 0;
	for (auto it = Rml::StringIteratorU8(value); it; ++it)
	{
		character_count += 1;
		if (character_count > character_offset)
			return (int)it.offset();
	}
	return (int)value.size();
}

void RmlBaseTextInputMethodContext::SetText(Rml::StringView text, int start, int end)
{
	Rml::String value = GetControl()->GetValue();

	start = ConvertCharacterOffsetToByteOffset(value, start);
	end = ConvertCharacterOffsetToByteOffset(value, end);

	RMLUI_ASSERTMSG(end >= start, "Invalid end character offset.");
	value.replace(start, end - start, text.begin(), text.size());

	int max_length = GetControl()->GetAttribute<int>("maxlength", 0);
	int max_byte = ConvertCharacterOffsetToByteOffset(value, max_length);

	if (max_byte > 0 && (int)value.length() > max_byte)
	{
		value = value.substr(0, max_byte);
	}

	GetControl()->SetValue(value);
}

void RmlBaseTextInputMethodContext::GetScreenBounds(Rml::Vector2f& position, Rml::Vector2f& size) const
{
	position = GetControl()->GetAbsoluteOffset(Rml::BoxArea::Border);
	size = GetControl()->GetBox().GetSize(Rml::BoxArea::Border);
}

Rml::ElementFormControl* RmlBaseTextInputMethodContext::GetControl() const
{
	return rmlui_static_cast<Rml::ElementFormControl*>(focused_input.get());
}

class RmlInputTextInputMethodContext final : public RmlBaseTextInputMethodContext {
public:
	RmlInputTextInputMethodContext(Rml::ElementFormControlInput* _input);

	virtual void GetSelectionRange(int& start, int& end) const override;
	virtual void SetSelectionRange(int start, int end) override;
	virtual void UpdateCompositionRange(int start, int end) override;

private:
	Rml::ElementFormControlInput* input;
};

RmlInputTextInputMethodContext::RmlInputTextInputMethodContext(Rml::ElementFormControlInput* _input) :
	RmlBaseTextInputMethodContext(_input), input(_input)
{}

void RmlInputTextInputMethodContext::GetSelectionRange(int& start, int& end) const
{
	input->GetSelection(&start, &end, nullptr);
}

void RmlInputTextInputMethodContext::SetSelectionRange(int start, int end)
{
	input->SetSelectionRange(start, end);
}

void RmlInputTextInputMethodContext::UpdateCompositionRange(int start, int end)
{
	input->SetIMERange(start, end);
}

Rml::UniquePtr<TextInputMethodContext> CreateTextInputMethodContext(Rml::ElementFormControlInput* input)
{
	return Rml::MakeUnique<RmlInputTextInputMethodContext>(input);
}

class RmlTextAreaTextInputMethodContext final : public RmlBaseTextInputMethodContext {
public:
	RmlTextAreaTextInputMethodContext(Rml::ElementFormControlTextArea* _text_area);

	virtual void GetSelectionRange(int& start, int& end) const override;
	virtual void SetSelectionRange(int start, int end) override;
	virtual void UpdateCompositionRange(int start, int end) override;

private:
	Rml::ElementFormControlTextArea* text_area;
};

RmlTextAreaTextInputMethodContext::RmlTextAreaTextInputMethodContext(Rml::ElementFormControlTextArea* _text_area) :
	RmlBaseTextInputMethodContext(_text_area), text_area(_text_area)
{}

void RmlTextAreaTextInputMethodContext::GetSelectionRange(int& start, int& end) const
{
	text_area->GetSelection(&start, &end, nullptr);
}

void RmlTextAreaTextInputMethodContext::SetSelectionRange(int start, int end)
{
	text_area->SetSelectionRange(start, end);
}

void RmlTextAreaTextInputMethodContext::UpdateCompositionRange(int start, int end)
{
	text_area->SetIMERange(start, end);
}

Rml::UniquePtr<TextInputMethodContext> CreateTextInputMethodContext(Rml::ElementFormControlTextArea* text_area)
{
	return Rml::MakeUnique<RmlTextAreaTextInputMethodContext>(text_area);
}
