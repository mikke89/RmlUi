#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementInstancer.h"
#include "../../Include/RmlUi/Core/Factory.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/Plugin.h"
#include "../../Include/RmlUi/SVG/ElementSVG.h"
#include "DecoratorSVG.h"
#include "SVGCache.h"
#include "XMLNodeHandlerSVG.h"

namespace Rml {
namespace SVG {

	class SVGPlugin : public Plugin {
	public:
		void OnInitialise() override
		{
			SVGCache::Initialize();

			element_instancer = MakeUnique<ElementInstancerGeneric<ElementSVG>>();
			Factory::RegisterElementInstancer("svg", element_instancer.get());

			decorator_instancer = MakeUnique<DecoratorSVGInstancer>();
			Factory::RegisterDecoratorInstancer("svg", decorator_instancer.get());

			XMLParser::RegisterNodeHandler("svg", MakeShared<XMLNodeHandlerSVG>());
			XMLParser::RegisterPersistentCDATATag("svg");

			Log::Message(Log::LT_INFO, "SVG plugin initialised.");
		}

		void OnShutdown() override
		{
			delete this;
			SVGCache::Shutdown();
		}

		int GetEventClasses() override { return Plugin::EVT_BASIC; }

	private:
		UniquePtr<ElementInstancerGeneric<ElementSVG>> element_instancer;
		UniquePtr<DecoratorSVGInstancer> decorator_instancer;
	};

	void Initialise()
	{
		RegisterPlugin(new SVGPlugin);
	}

} // namespace SVG
} // namespace Rml
