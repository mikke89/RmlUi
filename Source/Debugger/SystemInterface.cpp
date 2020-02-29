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

#include "SystemInterface.h"
#include "ElementLog.h"

namespace Rml {
namespace Debugger {

SystemInterface::SystemInterface(Core::SystemInterface* _application_interface, ElementLog* _log)
{
	application_interface = _application_interface;
	log = _log;
}

SystemInterface::~SystemInterface()
{
	application_interface = nullptr;
}

// Get the number of seconds elapsed since the start of the application.
double SystemInterface::GetElapsedTime()
{
	return application_interface->GetElapsedTime();
}

// Translate the input string into the translated string.
int SystemInterface::TranslateString(Core::String& translated, const Core::String& input)
{
	return application_interface->TranslateString(translated, input);
}

// Log the specified message.
bool SystemInterface::LogMessage(Core::Log::Type type, const Core::String& message)
{
	log->AddLogMessage(type, message);

	return application_interface->LogMessage(type, message);
}

// Set mouse cursor.
void SystemInterface::SetMouseCursor(const Core::String& cursor_name)
{
	application_interface->SetMouseCursor(cursor_name);
}

void SystemInterface::SetClipboardText(const Core::String& text)
{
	application_interface->SetClipboardText(text);
}

void SystemInterface::GetClipboardText(Core::String& text)
{
	application_interface->GetClipboardText(text);
}

// Activate keyboard (for touchscreen devices)
void SystemInterface::ActivateKeyboard()
{
	application_interface->ActivateKeyboard();
}
	
// Deactivate keyboard (for touchscreen devices)
void SystemInterface::DeactivateKeyboard()
{
	application_interface->DeactivateKeyboard();
}

}
}
