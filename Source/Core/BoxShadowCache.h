#pragma once

#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {
namespace Style {
	class ComputedValues;
}
struct BoxShadowGeometryInfo;

struct BoxShadowRenderable : NonCopyMoveable {
	BoxShadowRenderable(const BoxShadowGeometryInfo& geometry_info);
	~BoxShadowRenderable();

	CallbackTexture texture;
	Geometry geometry;
	Geometry background_border_geometry;
	const BoxShadowGeometryInfo& cache_key;
};

class BoxShadowCache {
public:
	static void Initialize();
	static void Shutdown();

	/// Returns a handle to BoxShadow renderable matching the element's style - creates new data if none is found.
	/// @param[in] element Element for which to calculate and cache the box shadow.
	/// @param[in] computed The computed style values of the element.
	/// @return A handle to the BoxShadow data, with automatic reference counting.
	static SharedPtr<BoxShadowRenderable> GetHandle(Element* element, const Style::ComputedValues& computed);
};

} // namespace Rml
