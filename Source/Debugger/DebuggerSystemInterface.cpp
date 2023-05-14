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

#include "DebuggerSystemInterface.h"
#include "ElementLog.h"

namespace Rml {
namespace Debugger {

DebuggerSystemInterface::DebuggerSystemInterface(Rml::SystemInterface* _application_interface, ElementLog* _log)
{
	application_interface = _application_interface;
	log = _log;
}

DebuggerSystemInterface::~DebuggerSystemInterface()
{
	application_interface = nullptr;
}

double DebuggerSystemInterface::GetElapsedTime()
{
	return application_interface->GetElapsedTime();
}

int DebuggerSystemInterface::TranslateString(String& translated, const String& input)
{
	return application_interface->TranslateString(translated, input);
}

void DebuggerSystemInterface::JoinPath(String& translated_path, const String& document_path, const String& path)
{
	application_interface->JoinPath(translated_path, document_path, path);
}

bool DebuggerSystemInterface::LogMessage(Log::Type type, const String& message)
{
	log->AddLogMessage(type, message);

	return application_interface->LogMessage(type, message);
}

void DebuggerSystemInterface::SetMouseCursor(const String& cursor_name)
{
	application_interface->SetMouseCursor(cursor_name);
}

void DebuggerSystemInterface::SetClipboardText(const String& text)
{
	application_interface->SetClipboardText(text);
}

void DebuggerSystemInterface::GetClipboardText(String& text)
{
	application_interface->GetClipboardText(text);
}

void DebuggerSystemInterface::ActivateKeyboard(Rml::Vector2f caret_position, float line_height)
{
	application_interface->ActivateKeyboard(caret_position, line_height);
}

void DebuggerSystemInterface::DeactivateKeyboard()
{
	application_interface->DeactivateKeyboard();
}

} // namespace Debugger
} // namespace Rml
