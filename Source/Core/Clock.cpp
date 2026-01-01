#include "Clock.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"

namespace Rml {

RMLUICORE_API double Clock::GetElapsedTime()
{
	SystemInterface* system_interface = GetSystemInterface();
	if (system_interface != nullptr)
		return system_interface->GetElapsedTime();
	else
		return 0;
}

} // namespace Rml
