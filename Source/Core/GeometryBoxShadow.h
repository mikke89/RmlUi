#pragma once

#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/RenderBox.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {
using RenderBoxList = Vector<RenderBox>;
struct BoxShadowGeometryInfo {
	ColourbPremultiplied background_color;
	Array<ColourbPremultiplied, 4> border_colors;
	CornerSizes border_radius;
	Vector2i texture_dimensions;
	Vector2f element_offset_in_texture;
	RenderBoxList padding_render_boxes;
	RenderBoxList border_render_boxes;
	BoxShadowList shadow_list;
	float opacity;
};
inline bool operator==(const BoxShadowGeometryInfo& a, const BoxShadowGeometryInfo& b)
{
	return a.background_color == b.background_color && a.border_colors == b.border_colors && a.border_radius == b.border_radius &&
		a.texture_dimensions == b.texture_dimensions && a.element_offset_in_texture == b.element_offset_in_texture &&
		a.padding_render_boxes == b.padding_render_boxes && a.border_render_boxes == b.border_render_boxes && a.shadow_list == b.shadow_list &&
		a.opacity == b.opacity;
}
inline bool operator!=(const BoxShadowGeometryInfo& a, const BoxShadowGeometryInfo& b)
{
	return !(a == b);
}

class Geometry;
class CallbackTexture;
class RenderManager;

class GeometryBoxShadow {
public:
	/// Resolve the element's properties into its box geometry info.
	/// @param[in] element The element to resolve.
	/// @param[in] border_radius The border radius of the element.
	/// @param[in] background_color The background colour of the element.
	/// @param[in] border_colors The border colours of the element.
	/// @param[in] opacity The computed opacity of the element.
	static BoxShadowGeometryInfo Resolve(Element* element, const CornerSizes& border_radius, ColourbPremultiplied background_color,
		const Array<ColourbPremultiplied, 4>& border_colors, float opacity);

	/// Generate the texture and geometry for a box shadow and including the element's background and border.
	/// @param[out] out_shadow_texture The target texture, assumes pointer stability during the lifetime of the shadow geometry.
	/// @param[out] out_background_border_geometry The generated geometry for the element's background and border.
	/// @param[in] render_manager The render manager to generate the shadow for.
	/// @param[in] shadow_geometry_info The resolved box shadow geometry of a given element.
	///	@see GeometryBoxShadow::Resolve()
	static void GenerateTexture(CallbackTexture& out_shadow_texture, Geometry& out_background_border_geometry, RenderManager& render_manager,
		const BoxShadowGeometryInfo& shadow_geometry_info);
};

} // namespace Rml
