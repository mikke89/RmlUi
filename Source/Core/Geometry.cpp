#include "../../Include/RmlUi/Core/Geometry.h"
#include "RenderManagerAccess.h"

namespace Rml {

Geometry::Geometry(RenderManager* render_manager, StableVectorIndex resource_handle) : UniqueRenderResource(render_manager, resource_handle) {}

void Geometry::Render(Vector2f translation, Texture texture, const CompiledShader& shader) const
{
	if (resource_handle == StableVectorIndex::Invalid)
		return;

	translation = translation.Round();

	RenderManagerAccess::Render(render_manager, *this, translation, texture, shader);
}

Mesh Geometry::Release(ReleaseMode mode)
{
	if (resource_handle == StableVectorIndex::Invalid)
		return Mesh();

	Mesh mesh = RenderManagerAccess::ReleaseResource(render_manager, *this);
	Clear();
	if (mode == ReleaseMode::ClearMesh)
	{
		mesh.vertices.clear();
		mesh.indices.clear();
	}
	return mesh;
}

const Mesh& Geometry::GetMesh() const
{
	RMLUI_ASSERT(resource_handle != StableVectorIndex::Invalid);
	return RenderManagerAccess::GetMesh(render_manager, *this);
}

} // namespace Rml
