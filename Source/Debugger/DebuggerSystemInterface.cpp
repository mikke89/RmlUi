#include "DebuggerSystemInterface.h"
#include "ElementLog.h"

namespace Rml {
namespace Debugger {

DebuggerSystemInterface::DebuggerSystemInterface(Rml::SystemInterface* application_interface, ElementLog* log) :
	Rml::SystemInterfaceProxy(application_interface), log(log)
{}

bool DebuggerSystemInterface::LogMessage(Log::Type type, const String& message)
{
	log->AddLogMessage(type, message);
	return upstream->LogMessage(type, message);
}

} // namespace Debugger
} // namespace Rml
