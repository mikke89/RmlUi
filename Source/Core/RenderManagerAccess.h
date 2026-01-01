#pragma once

#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class CompiledFilter;
class CompiledShader;
class CallbackTexture;
class Geometry;
class Texture;

class RenderManagerAccess {
private:
	template <typename T>
	static auto ReleaseResource(RenderManager* render_manager, T& resource)
	{
		return render_manager->ReleaseResource(resource);
	}

	static Vector2i GetDimensions(RenderManager* render_manager, TextureFileIndex texture);
	static Vector2i GetDimensions(RenderManager* render_manager, StableVectorIndex callback_texture);

	static void Render(RenderManager* render_manager, const Geometry& geometry, Vector2f translation, Texture texture, const CompiledShader& shader);

	static void GetTextureSourceList(RenderManager* render_manager, StringList& source_list);
	static const Mesh& GetMesh(RenderManager* render_manager, const Geometry& geometry);

	static bool ReleaseTexture(RenderManager* render_manager, const String& texture_source);
	static void ReleaseAllTextures(RenderManager* render_manager);
	static void ReleaseAllCompiledGeometry(RenderManager* render_manager);

	friend class CompiledFilter;
	friend class CompiledShader;
	friend class CallbackTexture;
	friend class Geometry;
	friend class Texture;

	friend StringList Rml::GetTextureSourceList();
	friend bool Rml::ReleaseTexture(const String&, RenderInterface*);
	friend void Rml::ReleaseTextures(RenderInterface*);
	friend void Rml::ReleaseCompiledGeometry(RenderInterface*);
	friend void Rml::ReleaseRenderManagers();
};

} // namespace Rml
