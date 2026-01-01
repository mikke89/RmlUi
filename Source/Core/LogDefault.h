#pragma once

#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

/**
    Provides a platform-dependent default implementation for message logging.
 */

class LogDefault {
public:
	static bool LogMessage(Log::Type type, const String& message);
};

} // namespace Rml
