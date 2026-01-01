#include "RenderManagerAccess.h"
#include "../../Include/RmlUi/Core/Texture.h"
#include "TextureDatabase.h"

namespace Rml {

Vector2i RenderManagerAccess::GetDimensions(RenderManager* render_manager, TextureFileIndex texture)
{
	return render_manager->texture_database->file_database.GetDimensions(render_manager->render_interface, texture);
}

Vector2i RenderManagerAccess::GetDimensions(RenderManager* render_manager, StableVectorIndex callback_texture)
{
	return render_manager->texture_database->callback_database.GetDimensions(render_manager, render_manager->render_interface, callback_texture);
}

void RenderManagerAccess::Render(RenderManager* render_manager, const Geometry& geometry, Vector2f translation, Texture texture,
	const CompiledShader& shader)
{
	render_manager->Render(geometry, translation, texture, shader);
}

void RenderManagerAccess::GetTextureSourceList(RenderManager* render_manager, StringList& source_list)
{
	render_manager->GetTextureSourceList(source_list);
}

const Mesh& RenderManagerAccess::GetMesh(RenderManager* render_manager, const Geometry& geometry)
{
	return render_manager->GetMesh(geometry);
}

bool RenderManagerAccess::ReleaseTexture(RenderManager* render_manager, const String& texture_source)
{
	return render_manager->ReleaseTexture(texture_source);
}

void RenderManagerAccess::ReleaseAllTextures(RenderManager* render_manager)
{
	render_manager->ReleaseAllTextures();
}

void RenderManagerAccess::ReleaseAllCompiledGeometry(RenderManager* render_manager)
{
	render_manager->ReleaseAllCompiledGeometry();
}

} // namespace Rml
