/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2025 The RmlUi Team, and contributors
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

#ifndef RMLUI_CORE_BOXSHADOWHASH_H
#define RMLUI_CORE_BOXSHADOWHASH_H

#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/NumericValue.h"
#include "../../Include/RmlUi/Core/RenderBox.h"
#include "../../Include/RmlUi/Core/Types.h"
#include "../../Include/RmlUi/Core/Unit.h"
#include "../../Include/RmlUi/Core/Utilities.h"
#include "BoxShadowCache.h"
#include "GeometryBoxShadow.h"

namespace std {

template <>
struct hash<::Rml::Unit> {
	using utype = underlying_type_t<::Rml::Unit>;
	size_t operator()(const ::Rml::Unit& t) const noexcept
	{
		hash<utype> h;
		return h(static_cast<utype>(t));
	}
};

template <>
struct hash<::Rml::Vector2i> {
	size_t operator()(const ::Rml::Vector2i& v) const noexcept
	{
		using namespace ::Rml::Utilities;
		size_t seed = hash<int>{}(v.x);
		HashCombine(seed, v.y);
		return seed;
	}
};
template <>
struct hash<::Rml::Vector2f> {
	size_t operator()(const ::Rml::Vector2f& v) const noexcept
	{
		using namespace ::Rml::Utilities;
		size_t seed = hash<float>{}(v.x);
		HashCombine(seed, v.y);
		return seed;
	}
};
template <>
struct hash<::Rml::Colourb> {
	size_t operator()(const ::Rml::Colourb& v) const noexcept { return static_cast<size_t>(hash<uint32_t>{}(reinterpret_cast<const uint32_t&>(v))); }
};
template <>
struct hash<::Rml::ColourbPremultiplied> {
	size_t operator()(const ::Rml::ColourbPremultiplied& v) const noexcept
	{
		return static_cast<size_t>(hash<uint32_t>{}(reinterpret_cast<const uint32_t&>(v)));
	}
};

template <>
struct hash<::Rml::NumericValue> {
	size_t operator()(const ::Rml::NumericValue& v) const noexcept
	{
		using namespace ::Rml::Utilities;
		size_t seed = hash<float>{}(v.number);
		HashCombine(seed, v.unit);
		return seed;
	}
};

template <>
struct hash<::Rml::BoxShadow> {
	size_t operator()(const ::Rml::BoxShadow& s) const noexcept
	{
		using namespace ::Rml;
		using namespace ::Rml::Utilities;
		size_t seed = std::hash<ColourbPremultiplied>{}(s.color);

		HashCombine(seed, s.offset_x);
		HashCombine(seed, s.offset_y);
		HashCombine(seed, s.blur_radius);
		HashCombine(seed, s.spread_distance);
		HashCombine(seed, s.inset);
		return seed;
	}
};

template <>
struct hash<::Rml::RenderBox> {
	size_t operator()(const ::Rml::RenderBox& box) const noexcept
	{
		using namespace ::Rml::Utilities;
		static auto HashArray4 = [](const ::Rml::Array<float, 4>& arr) -> size_t {
			size_t seed = 0;
			for (const auto& v : arr)
				HashCombine(seed, v);
			return seed;
		};

		size_t seed = 0;
		HashCombine(seed, box.GetFillSize());
		HashCombine(seed, box.GetBorderOffset());
		HashCombine(seed, HashArray4(box.GetBorderRadius()));
		HashCombine(seed, HashArray4(box.GetBorderWidths()));
		return seed;
	}
};

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
