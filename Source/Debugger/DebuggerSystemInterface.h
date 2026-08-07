#pragma once

#include "../../Include/RmlUi/Core/SystemInterfaceProxy.h"

namespace Rml {
namespace Debugger {

class ElementLog;

/**
    The log interface the debugger installs into RmlUi. This is a pass-through interface, so it holds onto the
    application's system interface and passes all the calls through.
 */

class DebuggerSystemInterface : public Rml::SystemInterfaceProxy {
public:
	/// Instances a new debugging log interface.
	/// @param[in] application_interface The upstream system interface to forward calls to.
	/// @param[in] log The logging element to send messages to.
	DebuggerSystemInterface(Rml::SystemInterface* application_interface, ElementLog* log);

	/// Intercept log messages.
	bool LogMessage(Log::Type type, const String& message) override;

private:
	ElementLog* log;
};

} // namespace Debugger
} // namespace Rml
