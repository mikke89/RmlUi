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
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "GeometryBoxShadow.h"

namespace Rml {

ElementBackgroundBorder::ElementBackgroundBorder() {}

void ElementBackgroundBorder::Render(Element* element)
{
	if (background_dirty || border_dirty)
	{
		for (auto& background : backgrounds)
		{
			if (background.first != BackgroundType::BackgroundBorder)
				background.second.geometry.Release();
		}

		GenerateGeometry(element);

		background_dirty = false;
		border_dirty = false;
	}

	Background* shadow = GetBackground(BackgroundType::BoxShadow);
	if (shadow && shadow->geometry)
		shadow->geometry.Render(element->GetAbsoluteOffset(BoxArea::Border), shadow->texture);
	else if (Background* background = GetBackground(BackgroundType::BackgroundBorder))
		background->geometry.Render(element->GetAbsoluteOffset(BoxArea::Border));
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
		Mesh mesh = geometry.Release(Geometry::ReleaseMode::ClearMesh);
		const Box& box = element->GetBox();
		const Vector4f border_radius = element->GetComputedValues().border_radius();
		MeshUtilities::GenerateBackground(mesh, box, {}, border_radius, ColourbPremultiplied(255), clip_area);
		if (RenderManager* render_manager = element->GetRenderManager())
			geometry = render_manager->MakeGeometry(std::move(mesh));
	}

	return &geometry;
}

ElementBackgroundBorder::Background* ElementBackgroundBorder::GetBackground(BackgroundType type)
{
	auto it = backgrounds.find(type);
	if (it != backgrounds.end())
		return &it->second;
	return nullptr;
}

ElementBackgroundBorder::Background& ElementBackgroundBorder::GetOrCreateBackground(BackgroundType type)
{
	auto it = backgrounds.find(type);
	if (it != backgrounds.end())
		return it->second;

	Background& background = backgrounds[type];
	return background;
}

void ElementBackgroundBorder::GenerateGeometry(Element* element)
{
	RenderManager* render_manager = element->GetRenderManager();
	if (!render_manager)
		return;

	const ComputedValues& computed = element->GetComputedValues();
	const bool has_box_shadow = computed.has_box_shadow();
	const float opacity = computed.opacity();

	// Apply opacity except if we have a box shadow. In the latter case the background is rendered opaquely into the box-shadow texture, while
	// opacity is applied to the entire box-shadow texture when that is rendered.
	bool apply_opacity = (!has_box_shadow && opacity < 1.f);

	auto ConvertColor = [=](Colourb color) {
		if (apply_opacity)
			return color.ToPremultiplied(opacity);
		else
			return color.ToPremultiplied();
	};

	ColourbPremultiplied background_color = ConvertColor(computed.background_color());
	ColourbPremultiplied border_colors[4] = {
		ConvertColor(computed.border_top_color()),
		ConvertColor(computed.border_right_color()),
		ConvertColor(computed.border_bottom_color()),
		ConvertColor(computed.border_left_color()),
	};
	const Vector4f border_radius = computed.border_radius();

	Geometry& geometry = GetOrCreateBackground(BackgroundType::BackgroundBorder).geometry;
	Mesh mesh = geometry.Release(Geometry::ReleaseMode::ClearMesh);

	for (int i = 0; i < element->GetNumBoxes(); i++)
	{
		Vector2f offset;
		const Box& box = element->GetBox(i, offset);
		MeshUtilities::GenerateBackgroundBorder(mesh, box, offset, border_radius, background_color, border_colors);
	}
	geometry = render_manager->MakeGeometry(std::move(mesh));

	if (has_box_shadow)
	{
		Geometry& background_border_geometry = geometry;

		const Property* p_box_shadow = element->GetLocalProperty(PropertyId::BoxShadow);
		RMLUI_ASSERT(p_box_shadow->value.GetType() == Variant::BOXSHADOWLIST);
		BoxShadowList shadow_list = p_box_shadow->value.Get<BoxShadowList>();

		// Generate the geometry for the box-shadow texture.
		Background& shadow_background = GetOrCreateBackground(BackgroundType::BoxShadow);
		Geometry& shadow_geometry = shadow_background.geometry;
		CallbackTexture& shadow_texture = shadow_background.texture;

		GeometryBoxShadow::Generate(shadow_geometry, shadow_texture, *render_manager, element, background_border_geometry, std::move(shadow_list),
			border_radius, computed.opacity());
	}
}

} // namespace Rml
