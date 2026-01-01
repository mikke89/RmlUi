#pragma once

#include "../../Include/RmlUi/Core/SystemInterface.h"

namespace Rml {
namespace Debugger {

class ElementLog;

/**
    The log interface the debugger installs into RmlUi. This is a pass-through interface, so it holds onto the
    application's system interface and passes all the calls through.
 */

class DebuggerSystemInterface : public Rml::SystemInterface {
public:
	/// Instances a new debugging log interface.
	/// @param[in] log The logging element to send messages to.
	DebuggerSystemInterface(Rml::SystemInterface* application_interface, ElementLog* log);
	virtual ~DebuggerSystemInterface();

	/// Get the number of seconds elapsed since the start of the application.
	/// @return Elapsed time, in seconds.
	double GetElapsedTime() override;

	/// Translate the input string into the translated string.
	/// @param[out] translated Translated string ready for display.
	/// @param[in] input String as received from XML.
	/// @return Number of translations that occured.
	int TranslateString(String& translated, const String& input) override;

	/// Joins the path of an RML or RCSS file with the path of a resource specified within the file.
	/// @param[out] translated_path The joined path.
	/// @param[in] document_path The path of the source document (including the file name).
	/// @param[in] path The path of the resource specified in the document.
	void JoinPath(String& translated_path, const String& document_path, const String& path) override;

	/// Log the specified message.
	/// @param[in] type Type of log message, ERROR, WARNING, etc.
	/// @param[in] message Message to log.
	/// @return True to continue execution, false to break into the debugger.
	bool LogMessage(Log::Type type, const String& message) override;

	/// Set mouse cursor.
	/// @param[in] cursor_name Cursor name to activate.
	void SetMouseCursor(const String& cursor_name) override;

	/// Set clipboard text.
	/// @param[in] text Text to apply to clipboard.
	void SetClipboardText(const String& text) override;

	/// Get clipboard text.
	/// @param[out] text Retrieved text from clipboard.
	void GetClipboardText(String& text) override;

	/// Activate keyboard (for touchscreen devices).
	void ActivateKeyboard(Rml::Vector2f caret_position, float line_height) override;

	/// Deactivate keyboard (for touchscreen devices).
	void DeactivateKeyboard() override;

private:
	Rml::SystemInterface* application_interface;
	ElementLog* log;
};

} // namespace Debugger
} // namespace Rml
