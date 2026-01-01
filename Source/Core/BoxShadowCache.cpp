#include "BoxShadowCache.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../Core/ControlledLifetimeResource.h"
#include "BoxShadowHash.h"
#include "GeometryBoxShadow.h"

namespace Rml {

struct BoxShadowCacheData {
	StableUnorderedMap<BoxShadowGeometryInfo, WeakPtr<BoxShadowRenderable>> handles;
};

static void ReleaseHandle(BoxShadowRenderable* handle);

BoxShadowRenderable::BoxShadowRenderable(const BoxShadowGeometryInfo& geometry_info) : cache_key(geometry_info) {}

BoxShadowRenderable::~BoxShadowRenderable()
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

static SharedPtr<BoxShadowRenderable> GetOrCreateBoxShadow(RenderManager& render_manager, const BoxShadowGeometryInfo& info)
{
	RMLUI_ZoneScoped;
	auto it_handle = shadow_cache_data->handles.find(info);
	if (it_handle != shadow_cache_data->handles.end())
	{
		SharedPtr<BoxShadowRenderable> result = it_handle->second.lock();
		RMLUI_ASSERTMSG(result, "Failed to lock handle in Box Shadow cache");
		return result;
	}

	const auto iterator_inserted = shadow_cache_data->handles.emplace(info, WeakPtr<BoxShadowRenderable>());
	RMLUI_ASSERTMSG(iterator_inserted.second, "Could not insert entry into the Box Shadow cache handle map, duplicate key.");
	const BoxShadowGeometryInfo& inserted_key = iterator_inserted.first->first;
	WeakPtr<BoxShadowRenderable>& inserted_weak_data_pointer = iterator_inserted.first->second;

	auto shadow_handle = MakeShared<BoxShadowRenderable>(inserted_key);
	GeometryBoxShadow::GenerateTexture(shadow_handle->texture, shadow_handle->background_border_geometry, render_manager, inserted_key);

	Mesh mesh;
	const byte alpha = byte(info.opacity * 255.f);
	MeshUtilities::GenerateQuad(mesh, -info.element_offset_in_texture, Vector2f(info.texture_dimensions), ColourbPremultiplied(alpha, alpha));
	shadow_handle->geometry = render_manager.MakeGeometry(std::move(mesh));

	inserted_weak_data_pointer = shadow_handle;
	return shadow_handle;
}

static void ReleaseHandle(BoxShadowRenderable* handle)
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

SharedPtr<BoxShadowRenderable> BoxShadowCache::GetHandle(Element* element, const ComputedValues& computed)
{
	RenderManager* render_manager = element->GetRenderManager();
	if (!render_manager)
		return {};

	ColourbPremultiplied background_color = computed.background_color().ToPremultiplied();
	Array<ColourbPremultiplied, 4> border_colors = {
		computed.border_top_color().ToPremultiplied(),
		computed.border_right_color().ToPremultiplied(),
		computed.border_bottom_color().ToPremultiplied(),
		computed.border_left_color().ToPremultiplied(),
	};
	const CornerSizes border_radius = computed.border_radius();
	BoxShadowGeometryInfo geom_info = GeometryBoxShadow::Resolve(element, border_radius, background_color, border_colors, computed.opacity());
	return GetOrCreateBoxShadow(*render_manager, geom_info);
}

} // namespace Rml
