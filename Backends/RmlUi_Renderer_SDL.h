#pragma once

#include <RmlUi/Core/RenderInterface.h>

#if RMLUI_SDL_VERSION_MAJOR == 3
	#include <SDL3/SDL.h>
#elif RMLUI_SDL_VERSION_MAJOR == 2
	#include <SDL.h>
#else
	#error "Unspecified RMLUI_SDL_VERSION_MAJOR. Please set this definition to the major version of the SDL library being linked to."
#endif

class RenderInterface_SDL : public Rml::RenderInterface {
public:
	RenderInterface_SDL(SDL_Renderer* renderer);

	// Sets up OpenGL states for taking rendering commands from RmlUi.
	void BeginFrame();
	void EndFrame();

	// -- Inherited from Rml::RenderInterface --

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

private:
	struct GeometryView {
		Rml::Span<const Rml::Vertex> vertices;
		Rml::Span<const int> indices;
	};

	SDL_Renderer* renderer;
	SDL_BlendMode blend_mode = {};
	SDL_Rect rect_scissor = {};
	bool scissor_region_enabled = false;
};
