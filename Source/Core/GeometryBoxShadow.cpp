#include "GeometryBoxShadow.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/CompiledFilterShader.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/RenderManager.h"

namespace Rml {

BoxShadowGeometryInfo GeometryBoxShadow::Resolve(Element* element, const CornerSizes& border_radius, ColourbPremultiplied background_color,
	const Array<ColourbPremultiplied, 4>& border_colors, float opacity)
{
	RMLUI_ZoneScoped;

	// Find the box-shadow texture dimension and offset required to cover all box-shadows and element boxes combined.
	Vector2f element_offset_in_texture;
	Vector2i texture_dimensions;

	const Property* p_box_shadow = element->GetLocalProperty(PropertyId::BoxShadow);
	RMLUI_ASSERT(p_box_shadow->value.GetType() == Variant::BOXSHADOWLIST);
	BoxShadowList shadow_list = p_box_shadow->value.Get<BoxShadowList>();

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
			const RenderBox box = element->GetRenderBox(BoxArea::Border, i);
			texture_region = texture_region.Join(Rectanglef::FromPositionSize(box.GetBorderOffset(), box.GetFillSize()));
		}

		texture_region = texture_region.Extend(-extend_min, extend_max);
		Math::ExpandToPixelGrid(texture_region);

		element_offset_in_texture = -texture_region.TopLeft();
		texture_dimensions = Vector2i(texture_region.Size());
	}

	// Since we can reuse textures across multiple box shadows with the same properties,
	// we need to copy the element's box shadow list and the background and border geometry.
	RenderBoxList padding_render_boxes{};
	RenderBoxList border_render_boxes{};

	for (int i = 0; i < element->GetNumBoxes(); i++)
	{
		padding_render_boxes.push_back(element->GetRenderBox(BoxArea::Padding, i));
		border_render_boxes.push_back(element->GetRenderBox(BoxArea::Border, i));
	}

	// Finally, create cache information
	BoxShadowGeometryInfo geometry_info;
	geometry_info.background_color = background_color;
	geometry_info.border_colors = border_colors;
	geometry_info.border_radius = border_radius;
	geometry_info.texture_dimensions = texture_dimensions;
	geometry_info.element_offset_in_texture = element_offset_in_texture;
	geometry_info.padding_render_boxes = std::move(padding_render_boxes);
	geometry_info.border_render_boxes = std::move(border_render_boxes);
	geometry_info.shadow_list = std::move(shadow_list);
	geometry_info.opacity = opacity;
	return geometry_info;
}

void GeometryBoxShadow::GenerateTexture(CallbackTexture& out_shadow_texture, Geometry& out_background_border_geometry, RenderManager& render_manager,
	const BoxShadowGeometryInfo& info)
{
	RMLUI_ZoneScoped;

	Mesh mesh = out_background_border_geometry.Release(Geometry::ReleaseMode::ClearMesh);
	for (size_t i = 0; i < info.padding_render_boxes.size(); i++)
		MeshUtilities::GenerateBackgroundBorder(mesh, info.padding_render_boxes[i], info.background_color, info.border_colors.data());
	out_background_border_geometry = render_manager.MakeGeometry(std::move(mesh));

	// Callback for generating the box-shadow texture. Using a callback ensures that the texture can be regenerated at any time, for example if the
	// device loses its GPU context and the client calls Rml::ReleaseTextures().
	auto texture_callback = [&info, &out_background_border_geometry](const CallbackTextureInterface& texture_interface) -> bool {
		RMLUI_ASSERT(info.border_render_boxes.size() == info.padding_render_boxes.size());
		RMLUI_ZoneScopedN("BoxShadow::GenerateTexture::Callback");
		size_t num_boxes = info.border_render_boxes.size();

		RenderManager& render_manager = texture_interface.GetRenderManager();

		Mesh mesh_padding;        // Render geometry for inner box-shadow.
		Mesh mesh_padding_border; // Clipping mask for outer box-shadow.

		bool has_inner_shadow = false;
		bool has_outer_shadow = false;
		for (const BoxShadow& shadow : info.shadow_list)
		{
			if (shadow.inset)
				has_inner_shadow = true;
			else
				has_outer_shadow = true;
		}

		// Generate the geometry for all the element's boxes and extend the render-texture further to cover all of them.
		for (size_t i = 0; i < num_boxes; i++)
		{
			ColourbPremultiplied white(255);
			if (has_inner_shadow)
				MeshUtilities::GenerateBackground(mesh_padding, info.padding_render_boxes[i], white);
			if (has_outer_shadow)
				MeshUtilities::GenerateBackground(mesh_padding_border, info.border_render_boxes[i], white);
		}

		const RenderState initial_render_state = render_manager.GetState();
		render_manager.ResetState();
		render_manager.SetScissorRegion(Rectanglei::FromSize(info.texture_dimensions));

		// The scissor region will be clamped to the current window size, check the resulting scissor region.
		const Rectanglei scissor_region = render_manager.GetScissorRegion();
		if (scissor_region.Width() <= 0 || scissor_region.Height() <= 0)
		{
			// The window may become zero-sized for example when minimized. Just skip the texture generation for now, we
			// expect to be called again later when the window is restored.
			render_manager.SetState(initial_render_state);
			return false;
		}
		if (scissor_region != Rectanglei::FromSize(info.texture_dimensions))
		{
			Log::Message(Log::LT_INFO,
				"The desired box-shadow texture dimensions (%d, %d) are larger than the current window region (%d, %d). "
				"Results may be clipped.",
				info.texture_dimensions.x, info.texture_dimensions.y, scissor_region.Width(), scissor_region.Height());
		}

		render_manager.PushLayer();
		out_background_border_geometry.Render(info.element_offset_in_texture);

		for (int shadow_index = (int)info.shadow_list.size() - 1; shadow_index >= 0; shadow_index--)
		{
			const BoxShadow& shadow = info.shadow_list[shadow_index];
			const Vector2f shadow_offset = {shadow.offset_x.number, shadow.offset_y.number};
			const bool inset = shadow.inset;
			const float spread_distance = shadow.spread_distance.number;
			const float blur_radius = shadow.blur_radius.number;

			CornerSizes spread_radii = info.border_radius;
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

			Mesh mesh_shadow;

			// Generate the shadow geometry. For outer box-shadows it is rendered normally, while for inset box-shadows it is used as a clipping mask.
			for (size_t i = 0; i < num_boxes; i++)
			{
				const float signed_spread_distance = (inset ? -spread_distance : spread_distance);
				RenderBox render_box = (inset ? info.padding_render_boxes : info.border_render_boxes)[i];
				render_box.SetFillSize(Math::Max(render_box.GetFillSize() + Vector2f(2.f * signed_spread_distance), Vector2f{0.001f}));
				render_box.SetBorderRadius(spread_radii);
				render_box.SetBorderOffset(render_box.GetBorderOffset() - Vector2f(signed_spread_distance));
				MeshUtilities::GenerateBackground(mesh_shadow, render_box, shadow.color);
			}

			CompiledFilter blur;
			if (blur_radius >= 0.5f)
			{
				blur = render_manager.CompileFilter("blur", Dictionary{{"sigma", Variant(0.5f * blur_radius)}});
				if (blur)
					render_manager.PushLayer();
			}

			Geometry geometry_shadow = render_manager.MakeGeometry(std::move(mesh_shadow));

			if (inset)
			{
				render_manager.SetClipMask(ClipMaskOperation::SetInverse, &geometry_shadow, shadow_offset + info.element_offset_in_texture);

				for (Rml::Vertex& vertex : mesh_padding.vertices)
					vertex.colour = shadow.color;

				// @performance: Don't need to copy the mesh if this is the last use of it.
				Geometry geometry_padding = render_manager.MakeGeometry(Mesh(mesh_padding));
				geometry_padding.Render(info.element_offset_in_texture);

				render_manager.SetClipMask(ClipMaskOperation::Set, &geometry_padding, info.element_offset_in_texture);
			}
			else
			{
				Mesh mesh = mesh_padding_border;
				Geometry geometry_padding_border = render_manager.MakeGeometry(std::move(mesh));
				render_manager.SetClipMask(ClipMaskOperation::SetInverse, &geometry_padding_border, info.element_offset_in_texture);
				geometry_shadow.Render(shadow_offset + info.element_offset_in_texture);
			}

			if (blur)
			{
				FilterHandleList filters;
				blur.AddHandleTo(filters);
				render_manager.CompositeLayers(render_manager.GetTopLayer(), render_manager.GetNextLayer(), BlendMode::Blend, filters);
				render_manager.PopLayer();
				blur.Release();
			}
		}

		texture_interface.SaveLayerAsTexture();

		render_manager.PopLayer();
		render_manager.SetState(initial_render_state);

		return true;
	};

	out_shadow_texture = render_manager.MakeCallbackTexture(std::move(texture_callback));
}
} // namespace Rml
