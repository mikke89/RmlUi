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

#include "BoxShadowCache.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/DecorationTypes.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/Texture.h"
#include "../../Include/RmlUi/Core/Utilities.h"
#include "../Core/ControlledLifetimeResource.h"
#include "ElementBackgroundBorder.h"
#include "GeometryBoxShadow.h"
#include <algorithm>

namespace Rml {

struct BoxShadowCacheData {
	StableUnorderedMap<BoxShadowGeometryInfo, WeakPtr<BoxShadowData>> handles;
};

static void ReleaseHandle(BoxShadowData* handle);

BoxShadowData::BoxShadowData(CallbackTexture&& texture, Geometry&& geometry, const BoxShadowGeometryInfo& geometry_info) :
	geometry(std::move(geometry)), texture(std::move(texture)), cache_key(geometry_info)
{}

BoxShadowData::~BoxShadowData()
{
	ReleaseHandle(this);
}

static ControlledLifetimeResource<BoxShadowCacheData> shadow_cache_data;

void BoxShadowCache::Initialize()
{
	shadow_cache_data.Initialize();
}

void BoxShadowCache::Shutdown()
{
	shadow_cache_data.Shutdown();
}

static SharedPtr<BoxShadowData> GetOrCreateBoxShadow(RenderManager& render_manager, const BoxShadowGeometryInfo& info,
	Geometry& background_border_geometry)
{
	auto it_handle = shadow_cache_data->handles.find(info);
	if (it_handle == shadow_cache_data->handles.end())
	{
		SharedPtr<BoxShadowData> result = it_handle->second.lock();
		RMLUI_ASSERTMSG(result, "Failed to lock handle in SVG cache");
		return result;
	}
	const auto iterator_inserted = shadow_cache_data->handles.emplace(info, WeakPtr<BoxShadowData>());
	RMLUI_ASSERTMSG(iterator_inserted.second, "Could not insert entry into the SVG cache handle map, duplicate key.");
	const BoxShadowGeometryInfo& inserted_key = iterator_inserted.first->first;
	WeakPtr<BoxShadowData>& inserted_weak_data_pointer = iterator_inserted.first->second;

	Geometry shadow_geometry = render_manager.MakeGeometry(background_border_geometry.Release(Geometry::ReleaseMode::ClearMesh));
	CallbackTexture shadow_texture;
	GeometryBoxShadow::GenerateTexture(shadow_texture, render_manager, inserted_key);
	auto shadow_handle = MakeShared<BoxShadowData>(std::move(shadow_texture), std::move(shadow_geometry), inserted_key);
	inserted_weak_data_pointer = shadow_handle;
	return shadow_handle;
}

static void ReleaseHandle(BoxShadowData* handle)
{
	// There are no longer any users of the cache entry uniquely identified by the handle address. Start from the
	// tip (i.e. per-color data) and remove that entry from its parent. Move up the cache ancestry and erase any
	// entries that no longer have any children.
	auto& handles = shadow_cache_data->handles;
	const BoxShadowGeometryInfo& key = handle->cache_key;

	auto it_handle = handles.find(key);
	RMLUI_ASSERT(it_handle != handles.cend());

	handles.erase(it_handle);
}

SharedPtr<BoxShadowData> BoxShadowCache::GetHandle(Element* element, Geometry& background_border_geometry)
{
	RenderManager* render_manager = element->GetRenderManager();
	if (!render_manager)
		return {};

	const ComputedValues& computed = element->GetComputedValues();
	const ColourbPremultiplied colour = computed.image_color().ToPremultiplied(computed.opacity());

	ColourbPremultiplied background_color = computed.background_color().ToPremultiplied();
	Array<ColourbPremultiplied, 4> border_colors = {
		computed.border_top_color().ToPremultiplied(),
		computed.border_right_color().ToPremultiplied(),
		computed.border_bottom_color().ToPremultiplied(),
		computed.border_left_color().ToPremultiplied(),
	};
	const CornerSizes border_radius = computed.border_radius();
	BoxShadowGeometryInfo geom_info = GeometryBoxShadow::Resolve(element, border_radius, background_color, border_colors);
	return GetOrCreateBoxShadow(*render_manager, geom_info, background_border_geometry);
}
} // namespace Rml