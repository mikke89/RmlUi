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
