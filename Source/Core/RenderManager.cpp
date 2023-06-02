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

#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"

namespace Rml {

RenderManager::RenderManager() : render_interface(GetRenderInterface())
{
	RMLUI_ASSERT(render_interface);
}

void RenderManager::BeginRender()
{
#ifdef RMLUI_DEBUG
	const RenderState default_state;
	RMLUI_ASSERT(state.clip_mask_list == default_state.clip_mask_list);
	RMLUI_ASSERT(state.scissor_region == state.scissor_region);
	RMLUI_ASSERT(state.transform == state.transform);
#endif
}

void RenderManager::SetViewport(Vector2i dimensions)
{
	viewport_dimensions = dimensions;
}

Vector2i RenderManager::GetViewport() const
{
	return viewport_dimensions;
}

void RenderManager::DisableScissorRegion()
{
	SetScissorRegion(Rectanglei::MakeInvalid());
}

void RenderManager::SetScissorRegion(Rectanglei new_region)
{
	const bool old_scissor_enable = state.scissor_region.Valid();
	const bool new_scissor_enable = new_region.Valid();

	if (new_scissor_enable != old_scissor_enable)
		render_interface->EnableScissorRegion(new_scissor_enable);

	if (new_scissor_enable)
	{
		new_region.Intersect(Rectanglei::FromSize(viewport_dimensions));

		if (new_region != state.scissor_region)
			render_interface->SetScissorRegion(new_region.Left(), new_region.Top(), new_region.Width(), new_region.Height());
	}

	state.scissor_region = new_region;
}

void RenderManager::DisableClipMask()
{
	if (!state.clip_mask_list.empty())
	{
		state.clip_mask_list.clear();
		ApplyClipMask(state.clip_mask_list);
	}
}

void RenderManager::SetClipMask(ClipMaskOperation operation, Geometry* geometry, Vector2f translation)
{
	RMLUI_ASSERT(geometry);
	state.clip_mask_list = {ClipMaskGeometry{operation, geometry, translation, nullptr}};
	ApplyClipMask(state.clip_mask_list);
}

void RenderManager::SetClipMask(ClipMaskGeometryList in_clip_elements)
{
	if (state.clip_mask_list != in_clip_elements)
	{
		state.clip_mask_list = std::move(in_clip_elements);
		ApplyClipMask(state.clip_mask_list);
	}
}

void RenderManager::SetTransform(const Matrix4f* p_new_transform)
{
	static const Matrix4f identity_transform = Matrix4f::Identity();
	const Matrix4f& new_transform = (p_new_transform ? *p_new_transform : identity_transform);

	if (state.transform != new_transform)
	{
		render_interface->SetTransform(p_new_transform);
		state.transform = new_transform;
	}
}

void RenderManager::ApplyClipMask(const ClipMaskGeometryList& clip_elements)
{
	const bool clip_mask_enabled = !clip_elements.empty();
	render_interface->EnableClipMask(clip_mask_enabled);

	if (clip_mask_enabled)
	{
		const Matrix4f initial_transform = state.transform;

		for (const ClipMaskGeometry& element_clip : clip_elements)
		{
			SetTransform(element_clip.transform);
			element_clip.geometry->RenderToClipMask(element_clip.operation, element_clip.absolute_offset);
		}

		// Apply the initially set transform in case it was changed.
		SetTransform(&initial_transform);
	}
}

void RenderManager::SetState(const RenderState& next)
{
	SetScissorRegion(next.scissor_region);

	SetClipMask(next.clip_mask_list);

	SetTransform(&next.transform);
}

void RenderManager::ResetState()
{
	SetState(RenderState{});
}

} // namespace Rml
