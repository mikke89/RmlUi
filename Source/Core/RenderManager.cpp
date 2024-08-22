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
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "TextureDatabase.h"

namespace Rml {

RenderManager::RenderManager(RenderInterface* render_interface) : render_interface(render_interface), texture_database(MakeUnique<TextureDatabase>())
{
	RMLUI_ASSERT(render_interface);

	constexpr size_t reserve_geometry = 256;
	geometry_list.reserve(reserve_geometry);
}

RenderManager::~RenderManager()
{
	struct ResourceCount {
		const char* name;
		int count;
	};
	ResourceCount elements[] = {
		{"Geometry", (int)geometry_list.size()},
		{"CompiledFilter", compiled_filter_count},
		{"CompiledShader", compiled_shader_count},
		{"CallbackTexture", (int)texture_database->callback_database.size()},
	};

	for (auto& element : elements)
	{
		if (element.count != 0)
		{
			Log::Message(Log::LT_ERROR, "Leaking %s detected (%d). Ensure that all RmlUi resources have been released by the end of Rml::Shutdown.",
				element.name, element.count);
		}
	}

	ReleaseAllTextures();
}

void RenderManager::PrepareRender()
{
#ifdef RMLUI_DEBUG
	const RenderState default_state;
	RMLUI_ASSERT(state.clip_mask_list == default_state.clip_mask_list);
	RMLUI_ASSERT(state.scissor_region == default_state.scissor_region);
	RMLUI_ASSERT(state.transform == default_state.transform);
	RMLUI_ASSERTMSG(render_stack.empty(), "Unbalanced render stack detected, ensure every PushLayer call has a corresponding call to PopLayer.");
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

Geometry RenderManager::MakeGeometry(Mesh&& mesh)
{
	return Geometry(this, InsertGeometry(std::move(mesh)));
}

Texture RenderManager::LoadTexture(const String& source, const String& document_path)
{
	String path;
	if (source.size() > 0 && source[0] == '?')
		path = source;
	else
		GetSystemInterface()->JoinPath(path, StringUtilities::Replace(document_path, '|', ':'), source);

	return Texture(this, texture_database->file_database.LoadTexture(render_interface, path));
}

CallbackTexture RenderManager::MakeCallbackTexture(CallbackTextureFunction callback)
{
	return CallbackTexture(this, texture_database->callback_database.CreateTexture(std::move(callback)));
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
		new_region = new_region.Intersect(Rectanglei::FromSize(viewport_dimensions));

		if (new_region != state.scissor_region)
			render_interface->SetScissorRegion(new_region);
	}

	state.scissor_region = new_region;
}

Rectanglei RenderManager::GetScissorRegion() const
{
	return state.scissor_region;
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
	RMLUI_ASSERT(geometry && geometry->render_manager == this);
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
			RMLUI_ASSERT(element_clip.geometry->render_manager == this);
			SetTransform(element_clip.transform);
			if (CompiledGeometryHandle handle = GetCompiledGeometryHandle(element_clip.geometry->resource_handle))
				render_interface->RenderToClipMask(element_clip.operation, handle, element_clip.absolute_offset);
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

StableVectorIndex RenderManager::InsertGeometry(Mesh&& mesh)
{
	return geometry_list.insert(GeometryData{std::move(mesh), CompiledGeometryHandle{}});
}

CompiledGeometryHandle RenderManager::GetCompiledGeometryHandle(StableVectorIndex index)
{
	if (index == StableVectorIndex::Invalid)
		return {};

	GeometryData& geometry = geometry_list[index];
	if (!geometry.handle && !geometry.mesh.indices.empty())
	{
		geometry.handle = render_interface->CompileGeometry(geometry.mesh.vertices, geometry.mesh.indices);

		if (!geometry.handle)
			Log::Message(Log::LT_ERROR, "Got empty compiled geometry.");
	}
	return geometry.handle;
}

void RenderManager::Render(const Geometry& geometry, Vector2f translation, Texture texture, const CompiledShader& shader)
{
	RMLUI_ASSERT(geometry);
	if (geometry.render_manager != this || (shader && shader.render_manager != this) || (texture && texture.render_manager != this))
	{
		RMLUI_ERRORMSG("Trying to render geometry with resources constructed in different render managers.");
		return;
	}

	if (CompiledGeometryHandle geometry_handle = GetCompiledGeometryHandle(geometry.resource_handle))
	{
		TextureHandle texture_handle = {};
		if (texture.file_index != TextureFileIndex::Invalid)
			texture_handle = texture_database->file_database.GetHandle(render_interface, texture.file_index);
		else if (texture.callback_index != StableVectorIndex::Invalid)
			texture_handle = texture_database->callback_database.GetHandle(this, render_interface, texture.callback_index);

		if (shader)
			render_interface->RenderShader(shader.resource_handle, geometry_handle, translation, texture_handle);
		else
			render_interface->RenderGeometry(geometry_handle, translation, texture_handle);
	}
}

void RenderManager::GetTextureSourceList(StringList& source_list) const
{
	texture_database->file_database.GetSourceList(source_list);
}

bool RenderManager::ReleaseTexture(const String& texture_source)
{
	return texture_database->file_database.ReleaseTexture(render_interface, texture_source);
}

void RenderManager::ReleaseAllTextures()
{
	texture_database->callback_database.ReleaseAllTextures(render_interface);
	texture_database->file_database.ReleaseAllTextures(render_interface);
}

void RenderManager::ReleaseAllCompiledGeometry()
{
	geometry_list.for_each([this](GeometryData& data) {
		if (data.handle)
		{
			render_interface->ReleaseGeometry(data.handle);
			data.handle = {};
		}
	});
}

CompiledFilter RenderManager::CompileFilter(const String& name, const Dictionary& parameters)
{
	if (CompiledFilterHandle handle = render_interface->CompileFilter(name, parameters))
	{
		compiled_filter_count += 1;
		return CompiledFilter(this, handle);
	}

	return CompiledFilter();
}

CompiledShader RenderManager::CompileShader(const String& name, const Dictionary& parameters)
{
	if (CompiledShaderHandle handle = render_interface->CompileShader(name, parameters))
	{
		compiled_shader_count += 1;
		return CompiledShader(this, handle);
	}

	return CompiledShader();
}

LayerHandle RenderManager::PushLayer()
{
	const LayerHandle layer = render_interface->PushLayer();
	render_stack.push_back(layer);
	return layer;
}

void RenderManager::CompositeLayers(LayerHandle source, LayerHandle destination, BlendMode blend_mode, Span<const CompiledFilterHandle> filters)
{
	RMLUI_ASSERT(source == 0 || std::find(render_stack.begin(), render_stack.end(), source) != render_stack.end());
	RMLUI_ASSERT(destination == 0 || std::find(render_stack.begin(), render_stack.end(), destination) != render_stack.end());
	render_interface->CompositeLayers(source, destination, blend_mode, filters);
}

void RenderManager::PopLayer()
{
	RMLUI_ASSERT(!render_stack.empty());
	render_interface->PopLayer();
	render_stack.pop_back();
}

LayerHandle RenderManager::GetTopLayer() const
{
	return render_stack.empty() ? LayerHandle{} : render_stack.back();
}

LayerHandle RenderManager::GetNextLayer() const
{
	RMLUI_ASSERT(!render_stack.empty());
	return render_stack.size() < 2 ? LayerHandle{} : render_stack[render_stack.size() - 2];
}

CompiledFilter RenderManager::SaveLayerAsMaskImage()
{
	if (CompiledFilterHandle handle = render_interface->SaveLayerAsMaskImage())
	{
		compiled_filter_count += 1;
		return CompiledFilter(this, handle);
	}
	return CompiledFilter();
}

void RenderManager::ReleaseResource(const CallbackTexture& texture)
{
	RMLUI_ASSERT(texture.render_manager == this && texture.resource_handle != texture.InvalidHandle());

	texture_database->callback_database.ReleaseTexture(render_interface, texture.resource_handle);
}

Mesh RenderManager::ReleaseResource(const Geometry& geometry)
{
	RMLUI_ASSERT(geometry.render_manager == this && geometry.resource_handle != geometry.InvalidHandle());

	GeometryData& data = geometry_list[geometry.resource_handle];
	if (data.handle)
	{
		render_interface->ReleaseGeometry(data.handle);
		data.handle = {};
	}
	Mesh result = std::exchange(data.mesh, Mesh());
	geometry_list.erase(geometry.resource_handle);
	return result;
}

void RenderManager::ReleaseResource(const CompiledFilter& filter)
{
	RMLUI_ASSERT(filter.render_manager == this && filter.resource_handle != filter.InvalidHandle());

	render_interface->ReleaseFilter(filter.resource_handle);
	compiled_filter_count -= 1;
}

void RenderManager::ReleaseResource(const CompiledShader& shader)
{
	RMLUI_ASSERT(shader.render_manager == this && shader.resource_handle != shader.InvalidHandle());

	render_interface->ReleaseShader(shader.resource_handle);
	compiled_shader_count -= 1;
}

} // namespace Rml
