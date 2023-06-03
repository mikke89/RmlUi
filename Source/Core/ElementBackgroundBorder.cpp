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

#include "ElementBackgroundBorder.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Context.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"

namespace Rml {

ElementBackgroundBorder::ElementBackgroundBorder() {}

void ElementBackgroundBorder::Render(Element* element)
{
	if (background_dirty || border_dirty)
	{
		for (auto& background : backgrounds)
			background.second.geometry.Release(true);

		GenerateGeometry(element);

		background_dirty = false;
		border_dirty = false;
	}

	Geometry* shadow_geometry = GetGeometry(BackgroundType::BoxShadow);
	if (shadow_geometry && *shadow_geometry)
		shadow_geometry->Render(element->GetAbsoluteOffset(BoxArea::Border));
	else if (Geometry* geometry = GetGeometry(BackgroundType::BackgroundBorder))
		geometry->Render(element->GetAbsoluteOffset(BoxArea::Border));
}

void ElementBackgroundBorder::DirtyBackground()
{
	background_dirty = true;
}

void ElementBackgroundBorder::DirtyBorder()
{
	border_dirty = true;
}

Geometry* ElementBackgroundBorder::GetClipGeometry(Element* element, BoxArea clip_area)
{
	BackgroundType type = {};
	switch (clip_area)
	{
	case Rml::BoxArea::Border: type = BackgroundType::ClipBorder; break;
	case Rml::BoxArea::Padding: type = BackgroundType::ClipPadding; break;
	case Rml::BoxArea::Content: type = BackgroundType::ClipContent; break;
	default: RMLUI_ERROR; return nullptr;
	}

	Geometry& geometry = GetOrCreateBackground(type).geometry;
	if (!geometry)
	{
		const Box& box = element->GetBox();
		const Vector4f border_radius = element->GetComputedValues().border_radius();
		GeometryUtilities::GenerateBackground(&geometry, box, {}, border_radius, Colourb(255), clip_area);
	}

	return &geometry;
}

Geometry* ElementBackgroundBorder::GetGeometry(BackgroundType type)
{
	auto it = backgrounds.find(type);
	if (it != backgrounds.end())
		return &it->second.geometry;
	return nullptr;
}

ElementBackgroundBorder::Background& ElementBackgroundBorder::GetOrCreateBackground(BackgroundType type)
{
	auto it = backgrounds.find(type);
	if (it != backgrounds.end())
		return it->second;

	return backgrounds.emplace(type, Background{}).first->second;
}

void ElementBackgroundBorder::GenerateGeometry(Element* element)
{
	const ComputedValues& computed = element->GetComputedValues();

	Colourb background_color = computed.background_color();
	Colourb border_colors[4] = {
		computed.border_top_color(),
		computed.border_right_color(),
		computed.border_bottom_color(),
		computed.border_left_color(),
	};
	const Vector4f border_radius = computed.border_radius();
	const bool has_box_shadow = computed.has_box_shadow();

	if (!has_box_shadow)
	{
		// Apply opacity except if we have a box shadow. In the latter case the background is rendered opaquely into the box-shadow texture, while
		// opacity is applied to the entire box-shadow texture when that is rendered.
		const float opacity = computed.opacity();
		if (opacity < 1.f)
		{
			background_color.alpha = (byte)(opacity * (float)background_color.alpha);

			for (int i = 0; i < 4; ++i)
				border_colors[i].alpha = (byte)(opacity * (float)border_colors[i].alpha);
		}
	}

	Geometry& geometry = GetOrCreateBackground(BackgroundType::BackgroundBorder).geometry;
	RMLUI_ASSERT(!geometry);

	for (int i = 0; i < element->GetNumBoxes(); i++)
	{
		Vector2f offset;
		const Box& box = element->GetBox(i, offset);
		GeometryUtilities::GenerateBackgroundBorder(&geometry, box, offset, border_radius, background_color, border_colors);
	}

	if (has_box_shadow)
	{
		const Property* p_box_shadow = element->GetLocalProperty(PropertyId::BoxShadow);
		RMLUI_ASSERT(p_box_shadow->value.GetType() == Variant::BOXSHADOWLIST);
		BoxShadowList shadow_list = p_box_shadow->value.Get<BoxShadowList>();

		GenerateBoxShadow(element, std::move(shadow_list), border_radius, computed.opacity());
	}
}

void ElementBackgroundBorder::GenerateBoxShadow(Element* element, BoxShadowList shadow_list, const Vector4f border_radius, const float opacity)
{
	// Find the box-shadow texture dimension and offset required to cover all box-shadows and element boxes combined.
	Vector2f element_offset_in_texture;
	Vector2i texture_dimensions;

	// Resolve all lengths to px units.
	for (BoxShadow& shadow : shadow_list)
	{
		shadow.blur_radius = NumericValue(element->ResolveLength(shadow.blur_radius), Unit::PX);
		shadow.spread_distance = NumericValue(element->ResolveLength(shadow.spread_distance), Unit::PX);
		shadow.offset_x = NumericValue(element->ResolveLength(shadow.offset_x), Unit::PX);
		shadow.offset_y = NumericValue(element->ResolveLength(shadow.offset_y), Unit::PX);
	}

	{
		Vector2f extend_min;
		Vector2f extend_max;

		// Extend the render-texture to encompass box-shadow blur and spread.
		for (const BoxShadow& shadow : shadow_list)
		{
			if (!shadow.inset)
			{
				const float extend = 1.5f * shadow.blur_radius.number + shadow.spread_distance.number;
				const Vector2f offset = {shadow.offset_x.number, shadow.offset_y.number};
				extend_min = Math::Min(extend_min, offset - Vector2f(extend));
				extend_max = Math::Max(extend_max, offset + Vector2f(extend));
			}
		}

		Rectanglef texture_region;

		// Extend the render-texture further to cover all the element's boxes.
		for (int i = 0; i < element->GetNumBoxes(); i++)
		{
			Vector2f offset;
			const Box& box = element->GetBox(i, offset);
			texture_region.Join(Rectanglef::FromPositionSize(offset, box.GetSize(BoxArea::Border)));
		}

		texture_region.ExtendTopLeft(-extend_min);
		texture_region.ExtendBottomRight(extend_max);
		Math::ExpandToPixelGrid(texture_region);

		element_offset_in_texture = -texture_region.TopLeft();
		texture_dimensions = Vector2i(texture_region.Size());
	}

	Geometry& background_border_geometry = *GetGeometry(BackgroundType::BackgroundBorder);

	// Callback for generating the box-shadow texture. Using a callback ensures that the texture can be regenerated at any time, for example if the
	// device loses its GPU context and the client calls Rml::ReleaseTextures().
	auto p_callback = [&background_border_geometry, element, border_radius, texture_dimensions, element_offset_in_texture, shadow_list](
						  RenderInterface* render_interface, const String& /*name*/, TextureHandle& out_handle, Vector2i& out_dimensions) -> bool {
		Context* context = element->GetContext();
		if (!context)
		{
			RMLUI_ERROR;
			return false;
		}

		Geometry geometry_padding;        // Render geometry for inner box-shadow.
		Geometry geometry_padding_border; // Clipping mask for outer box-shadow.

		bool has_inner_shadow = false;
		bool has_outer_shadow = false;
		for (const BoxShadow& shadow : shadow_list)
		{
			if (shadow.inset)
				has_inner_shadow = true;
			else
				has_outer_shadow = true;
		}

		// Generate the geometry for all the element's boxes and extend the render-texture further to cover all of them.
		for (int i = 0; i < element->GetNumBoxes(); i++)
		{
			Vector2f offset;
			const Box& box = element->GetBox(i, offset);

			if (has_inner_shadow)
				GeometryUtilities::GenerateBackground(&geometry_padding, box, offset, border_radius, Colourb(255), BoxArea::Padding);
			if (has_outer_shadow)
				GeometryUtilities::GenerateBackground(&geometry_padding_border, box, offset, border_radius, Colourb(255), BoxArea::Border);
		}

		RenderManager& render_manager = context->GetRenderManager();
		const RenderState initial_render_state = render_manager.GetState();
		render_manager.ResetState();
		render_manager.SetScissorRegion(Rectanglei::FromSize(texture_dimensions));

		render_interface->PushLayer(LayerFill::Clear);

		background_border_geometry.Render(element_offset_in_texture);

		for (int shadow_index = (int)shadow_list.size() - 1; shadow_index >= 0; shadow_index--)
		{
			const BoxShadow& shadow = shadow_list[shadow_index];
			const Vector2f shadow_offset = {shadow.offset_x.number, shadow.offset_y.number};
			const bool inset = shadow.inset;
			const float spread_distance = shadow.spread_distance.number;
			const float blur_radius = shadow.blur_radius.number;

			Vector4f spread_radii = border_radius;
			for (int i = 0; i < 4; i++)
			{
				float& radius = spread_radii[i];
				float spread_factor = (inset ? -1.f : 1.f);
				if (radius < spread_distance)
				{
					const float ratio_minus_one = (radius / spread_distance) - 1.f;
					spread_factor *= 1.f + ratio_minus_one * ratio_minus_one * ratio_minus_one;
				}
				radius = Math::Max(radius + spread_factor * spread_distance, 0.f);
			}

			Geometry shadow_geometry;

			// Generate the shadow geometry. For outer box-shadows it is rendered normally, while for inner box-shadows it is used as a clipping mask.
			for (int i = 0; i < element->GetNumBoxes(); i++)
			{
				Vector2f offset;
				Box box = element->GetBox(i, offset);
				const float signed_spread_distance = (inset ? -spread_distance : spread_distance);
				offset -= Vector2f(signed_spread_distance);

				for (int j = 0; j < Box::num_edges; j++)
				{
					BoxEdge edge = (BoxEdge)j;
					const float new_size = box.GetEdge(BoxArea::Padding, edge) + signed_spread_distance;
					box.SetEdge(BoxArea::Padding, edge, new_size);
				}

				GeometryUtilities::GenerateBackground(&shadow_geometry, box, offset, spread_radii, shadow.color,
					inset ? BoxArea::Padding : BoxArea::Border);
			}

			CompiledFilterHandle blur = {};
			if (blur_radius > 0.5f)
			{
				blur = render_interface->CompileFilter("blur", Dictionary{{"radius", Variant(blur_radius)}});
				if (blur)
					render_interface->PushLayer(LayerFill::Clear);
			}

			if (inset)
			{
				render_manager.SetClipMask(ClipMaskOperation::SetInverse, &shadow_geometry, shadow_offset + element_offset_in_texture);

				for (Rml::Vertex& vertex : geometry_padding.GetVertices())
					vertex.colour = shadow.color;

				geometry_padding.Release();
				geometry_padding.Render(element_offset_in_texture);

				render_manager.SetClipMask(ClipMaskOperation::Set, &geometry_padding, element_offset_in_texture);
			}
			else
			{
				render_manager.SetClipMask(ClipMaskOperation::SetInverse, &geometry_padding_border, element_offset_in_texture);
				shadow_geometry.Render(shadow_offset + element_offset_in_texture);
			}

			if (blur)
			{
				render_interface->PopLayer(BlendMode::Blend, {blur});
				render_interface->ReleaseCompiledFilter(blur);
			}
		}

		TextureHandle shadow_texture = render_interface->SaveLayerAsTexture(texture_dimensions);
		if (!shadow_texture)
			return false;

		render_interface->PopLayer(BlendMode::Discard, {});

		render_manager.SetState(initial_render_state);

		out_dimensions = texture_dimensions;
		out_handle = shadow_texture;

		return true;
	};

	// Generate the geometry for the box-shadow texture.
	Background& shadow_background = GetOrCreateBackground(BackgroundType::BoxShadow);
	Geometry& shadow_geometry = shadow_background.geometry;
	Texture& shadow_texture = shadow_background.texture;
	RMLUI_ASSERT(!shadow_geometry);

	Vector<Vertex>& vertices = shadow_geometry.GetVertices();
	Vector<int>& indices = shadow_geometry.GetIndices();
	vertices.resize(4);
	indices.resize(6);
	const byte alpha = byte(opacity * 255.f);
	GeometryUtilities::GenerateQuad(vertices.data(), indices.data(), -element_offset_in_texture, Vector2f(texture_dimensions), Colourb(255, alpha));

	shadow_texture.Set("box-shadow", p_callback);
	shadow_geometry.SetTexture(&shadow_texture);
}

} // namespace Rml
