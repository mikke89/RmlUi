/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_GEOMETRYBOXSHADOW_H
#define RMLUI_CORE_GEOMETRYBOXSHADOW_H

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
	/// Generate the texture and geometry for a box shadow.
	/// @param[in] element The element to resolve.
	/// @param[in] border_radius The border radius of the element.
	/// @param[in] background_color The background colour of the element.
	/// @param[in] border_colors The border colours of the element.
	/// @param[in] opacity The computed opacity of the element.
	static BoxShadowGeometryInfo Resolve(Element* element, const CornerSizes& border_radius, ColourbPremultiplied background_color,
		const Array<ColourbPremultiplied, 4>& border_colors, float opacity);

	/// Generate the texture and geometry for a box shadow.
	/// @param[out] out_shadow_texture The target texture, assumes pointer stability during the lifetime of the shadow geometry.
	/// @param[in] render_manager The render manager to generate the shadow for.
	/// @param[in] shadow_geometry_info The resolved box shadow geometry of a given element.
	///	@see GeometryBoxShadow::Resolve()
	static void GenerateTexture(CallbackTexture& out_shadow_texture, Geometry& background_border_geometry, RenderManager& render_manager,
		const BoxShadowGeometryInfo& shadow_geometry_info);
};

} // namespace Rml

namespace std {
template <>
struct hash<::Rml::BoxShadowGeometryInfo> {
	size_t operator()(const ::Rml::BoxShadowGeometryInfo& in) const noexcept
	{
		using namespace ::Rml::Utilities;
		size_t seed = size_t(849128392);

		HashCombine(seed, in.background_color);
		for (const auto& v : in.border_colors)
		{
			HashCombine(seed, v);
		}

		for (const auto& v : in.border_radius)
		{
			HashCombine(seed, v);
		}

		HashCombine(seed, in.texture_dimensions);
		HashCombine(seed, in.element_offset_in_texture);

		for (const auto& v : in.padding_render_boxes)
		{
			HashCombine(seed, v);
		}
		for (const auto& v : in.border_render_boxes)
		{
			HashCombine(seed, v);
		}
		for (const ::Rml::BoxShadow& v : in.shadow_list)
		{
			HashCombine(seed, v);
		}
		HashCombine(seed, in.opacity);
		return seed;
	}
};
} // namespace std
#endif
