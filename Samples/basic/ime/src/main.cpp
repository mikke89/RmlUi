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
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <PlatformExtensions.h>
#include <RmlUi_Backend.h>
#include <RmlUi_Include_Windows.h>
#include <Shell.h>

#if !defined RMLUI_PLATFORM_WIN32
	#error "This sample works only on Windows!"
#endif

class TextInputEventHandler final : public Rml::EventListener {
public:
	TextInputEventHandler(Rml::Context* _context);
	~TextInputEventHandler();

	virtual void ProcessEvent(Rml::Event& event) override;

private:
	void OnFocus(Rml::Element* element);
	void OnBlur(Rml::Element* element);

private:
	Rml::Context* context;
};

/**
    Sample implementation for handling IME.
    You may want to define an interface and make platform-specific implementations.
 */

class TextInputMethodEditor {
public:
	TextInputMethodEditor();

	void IMEStartComposition();
	void IMEEndComposition();
	void IMECancelComposition();

	void IMESetComposition(Rml::StringView composition, bool finalize);

	void UpdateCursorPosition();

public:
	Rml::UniquePtr<TextInputMethodContext> context;

	bool composing;
	int cursor_pos;

	int composition_range_start;
	int composition_range_end;
};
static TextInputMethodEditor text_input_method_editor;

static int GetCursorPosition(HIMC context)
{
	return ImmGetCompositionString(context, GCS_CURSORPOS, nullptr, 0);
}

static Rml::String GetCompositionString(HIMC context, bool finalize)
{
	DWORD type = finalize ? GCS_RESULTSTR : GCS_COMPSTR;

	int len_bytes = ImmGetCompositionStringW(context, type, nullptr, 0);

	if (len_bytes <= 0)
		return {};

	int len_chars = len_bytes / sizeof(WCHAR);
	Rml::UniquePtr<WCHAR[]> buffer(new WCHAR[len_chars + 1]);
	ImmGetCompositionStringW(context, type, buffer.get(), len_bytes);

	int multi_byte_count = WideCharToMultiByte(CP_UTF8, 0, buffer.get(), len_chars, nullptr, 0, nullptr, nullptr);

	Rml::String str(multi_byte_count, 0);
	WideCharToMultiByte(CP_UTF8, 0, buffer.get(), -1, &str[0], multi_byte_count, nullptr, nullptr);

	return str;
}

// The original window procedure handler.
static WNDPROC wnd_proc_old = nullptr;

// Custom event handler for IME events. The rest is processed by the backend.
static LRESULT CALLBACK WindowProcedureHandler(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
	switch (message)
	{
	case WM_IME_STARTCOMPOSITION:
	{
		text_input_method_editor.IMEStartComposition();

		// Prevent the native composition window from appearing by capturing this message.
		// We handle the composition string on our own, which is the purpose of the sample.
		return 0;
	}
	break;
	case WM_IME_ENDCOMPOSITION:
	{
		text_input_method_editor.IMESetComposition(Rml::StringView(), true);
		text_input_method_editor.IMEEndComposition();

		return DefWindowProc(window_handle, message, w_param, l_param);
	}
	break;
	case WM_IME_COMPOSITION:
	{
		if (text_input_method_editor.context)
		{
			// Not every IME starts a composition.
			if (!text_input_method_editor.composing)
			{
				text_input_method_editor.IMEStartComposition();
			}

			if (!!(l_param & GCS_CURSORPOS))
			{
				if (auto context = ImmGetContext(GetActiveWindow()))
				{
					text_input_method_editor.cursor_pos = GetCursorPosition(context);
					text_input_method_editor.UpdateCursorPosition();
				}
			}

			if (!!(l_param & CS_NOMOVECARET))
			{
				text_input_method_editor.cursor_pos = -1;
			}

			if (!!(l_param & GCS_RESULTSTR))
			{
				if (auto context = ImmGetContext(GetActiveWindow()))
				{
					Rml::String composition = GetCompositionString(context, true);
					text_input_method_editor.IMESetComposition(composition, true);
					text_input_method_editor.IMEEndComposition();
				}
			}

			if (!!(l_param & GCS_COMPSTR))
			{
				if (auto context = ImmGetContext(GetActiveWindow()))
				{
					Rml::String composition = GetCompositionString(context, false);
					text_input_method_editor.IMESetComposition(composition, false);
				}
			}

			// The composition has been canceled.
			if (!l_param)
			{
				text_input_method_editor.IMECancelComposition();
			}
		}

		return DefWindowProc(window_handle, message, w_param, l_param);
	}
	break;
	case WM_IME_CHAR:
	case WM_IME_REQUEST:
	{
		// Ignore WM_IME_CHAR and WM_IME_REQUEST events; we handle the composition string ourselves.
		return 0;
	}
	break;
	}

	// All unhandled messages go back to the backend.
	return CallWindowProc(wnd_proc_old, window_handle, message, w_param, l_param);
}

static void LoadFonts()
{
	struct FontFace {
		const char* filename;
		bool fallback_face;
	};
	FontFace font_faces[] = {
		{"assets/LatoLatin-Regular.ttf", false},
		{"assets/LatoLatin-Italic.ttf", false},
		{"assets/LatoLatin-Bold.ttf", false},
		{"assets/LatoLatin-BoldItalic.ttf", false},
		{"basic/ime/data/NotoSans-Regular.ttf", false},
		{"assets/NotoEmoji-Regular.ttf", true},
		{"basic/ime/data/NotoSansKR-Regular.otf", true},
		{"basic/ime/data/NotoSansSC-Regular.otf", true},
	};

	for (const FontFace& face : font_faces)
		Rml::LoadFontFace(face.filename, face.fallback_face);
}

int APIENTRY WinMain(HINSTANCE /*instance_handle*/, HINSTANCE /*previous_instance_handle*/, char* /*command_line*/, int /*command_show*/)
{
	const int window_width = 1024;
	const int window_height = 768;

	// Initializes the shell which provides common functionality used by the included samples.
	if (!Shell::Initialize())
		return -1;

	// Constructs the system and render interfaces, creates a window, and attaches the renderer.
	if (!Backend::Initialize("IME Sample", window_width, window_height, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Override the backend window procedure handler with a custom one.
	wnd_proc_old = (WNDPROC)SetWindowLongPtr(GetActiveWindow(), GWLP_WNDPROC, (LONG_PTR)WindowProcedureHandler);

	// Install the custom interfaces constructed by the backend before initializing RmlUi.
	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	// RmlUi initialisation.
	Rml::Initialise();

	// Create the main RmlUi context.
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
	if (!context)
	{
		Rml::Shutdown();
		Backend::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rml::Debugger::Initialise(context);

	// Load required fonts with support for most character sets.
	LoadFonts();

	// Register a custom event handler to handle focus and blur events.
	auto text_input_event_handler = Rml::MakeUnique<TextInputEventHandler>(context);

	// Load and show the demo document.
	Rml::ElementDocument* document = context->LoadDocument("basic/ime/data/ime.rml");
	if (document)
		document->Show();

	bool running = true;
	while (running)
	{
		running = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

		context->Update();

		Backend::BeginFrame();
		context->Render();
		Backend::PresentFrame();
	}

	text_input_event_handler.reset();

	// Restore the window procedure handler.
	SetWindowLongPtr(GetActiveWindow(), GWLP_WNDPROC, (LONG_PTR)wnd_proc_old);

	Rml::Shutdown();

	Backend::Shutdown();
	Shell::Shutdown();

	return 0;
}

TextInputEventHandler::TextInputEventHandler(Rml::Context* _context) : context(_context)
{
	context->AddEventListener("focus", this, true);
	context->AddEventListener("blur", this, true);
}

TextInputEventHandler::~TextInputEventHandler()
{
	context->RemoveEventListener("blur", this, true);
	context->RemoveEventListener("focus", this, true);
}

void TextInputEventHandler::ProcessEvent(Rml::Event& event)
{
	switch (event.GetId())
	{
	case Rml::EventId::Focus: OnFocus(event.GetTargetElement()); break;
	case Rml::EventId::Blur: OnBlur(event.GetTargetElement()); break;
	default: break; // Ignore other events.
	}
}

void TextInputEventHandler::OnFocus(Rml::Element* element)
{
	if (auto input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(element))
	{
		Rml::String type = input->GetAttribute("type", Rml::String());
		// Ensure that the input control is a text area.
		if (type == "text" || type == "password")
			text_input_method_editor.context = CreateTextInputMethodContext(input);
	}
	else if (auto text_area = rmlui_dynamic_cast<Rml::ElementFormControlTextArea*>(element))
		text_input_method_editor.context = CreateTextInputMethodContext(text_area);
}

void TextInputEventHandler::OnBlur(Rml::Element* /*element*/)
{
	text_input_method_editor.context.reset();
}

TextInputMethodEditor::TextInputMethodEditor() : composing(false), cursor_pos(-1), composition_range_start(0), composition_range_end(0) {}

void TextInputMethodEditor::IMEStartComposition()
{
	composing = true;
}

void TextInputMethodEditor::IMEEndComposition()
{
	if (context)
		context->UpdateCompositionRange(0, 0);

	composing = false;

	composition_range_start = 0;
	composition_range_end = 0;
}

void TextInputMethodEditor::IMECancelComposition()
{
	RMLUI_ASSERT(context);

	// Purge the current composition string.
	context->SetText(Rml::StringView(), composition_range_start, composition_range_end);

	IMEEndComposition();
}

void TextInputMethodEditor::IMESetComposition(Rml::StringView composition, bool finalize)
{
	RMLUI_ASSERT(context);

	// Retrieve the composition range if it is missing.
	if (composition_range_start == 0 && composition_range_end == 0)
		context->GetSelectionRange(composition_range_start, composition_range_end);

	int start = composition_range_start;
	int end = composition_range_end;

	context->SetText(composition, start, end);

	size_t length = Rml::StringUtilities::LengthUTF8(composition);
	composition_range_end = start + (int)length;

	// If the composition is final, move the cursor to the end of the string.
	if (finalize)
		cursor_pos = composition_range_end - composition_range_start;

	UpdateCursorPosition();

	// Update the composition range only if the cursor can be moved around. Editors working with a single
	// character (e.g., Hangul IME) should have no visual feedback; they use a selection range instead.
	if (cursor_pos != -1)
		context->UpdateCompositionRange(composition_range_start, composition_range_end);
}

void TextInputMethodEditor::UpdateCursorPosition()
{
	RMLUI_ASSERT(context);

	// Cursor position update happens before a composition is set; ignore this event.
	if (composition_range_start == 0 && composition_range_end == 0)
		return;

	if (cursor_pos != -1)
	{
		int position = composition_range_start + cursor_pos;
		context->SetCursorPosition(position);
	}
	else
	{
		// If Imm reports no cursor position, select the entire composition string for a better UX.
		context->SetSelectionRange(composition_range_start, composition_range_end);
	}
}
