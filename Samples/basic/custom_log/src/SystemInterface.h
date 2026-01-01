#pragma once

#include <RmlUi/Core/SystemInterface.h>

/**
    Sample custom RmlUi system interface for writing log messages to file.
 */

class SystemInterface : public Rml::SystemInterface {
public:
	SystemInterface();
	virtual ~SystemInterface();

	/// Log the specified message.
	/// @param[in] type Type of log message, ERROR, WARNING, etc.
	/// @param[in] message Message to log.
	/// @return True to continue execution, false to break into the debugger.
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

private:
	FILE* fp;
};
