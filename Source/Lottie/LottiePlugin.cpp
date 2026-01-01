#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementInstancer.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Plugin.h"
#include "../../Include/RmlUi/Lottie/ElementLottie.h"

namespace Rml {
namespace Lottie {

	class LottiePlugin : public Plugin {
	public:
		void OnInitialise() override
		{
			instancer = MakeUnique<ElementInstancerGeneric<ElementLottie>>();

			Factory::RegisterElementInstancer("lottie", instancer.get());

			Log::Message(Log::LT_INFO, "Lottie plugin initialised.");
		}

		void OnShutdown() override { delete this; }

		int GetEventClasses() override { return Plugin::EVT_BASIC; }

	private:
		UniquePtr<ElementInstancerGeneric<ElementLottie>> instancer;
	};

	void Initialise()
	{
		RegisterPlugin(new LottiePlugin);
	}

} // namespace Lottie
} // namespace Rml
