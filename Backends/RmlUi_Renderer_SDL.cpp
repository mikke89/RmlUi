#include "RmlUi_Renderer_SDL.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Types.h>

#if SDL_MAJOR_VERSION >= 3
	#include <SDL3_image/SDL_image.h>
#else
	#include <SDL_image.h>
#endif

#if SDL_MAJOR_VERSION == 2 && !(SDL_VIDEO_RENDER_OGL)
	#error "Only the OpenGL SDL backend is supported."
#endif

static void SetRenderClipRect(SDL_Renderer* renderer, const SDL_Rect* rect)
{
#if SDL_MAJOR_VERSION >= 3
	SDL_SetRenderClipRect(renderer, rect);
#else
	SDL_RenderSetClipRect(renderer, rect);
#endif
}
static void SetRenderViewport(SDL_Renderer* renderer, const SDL_Rect* rect)
{
#if SDL_MAJOR_VERSION >= 3
	SDL_SetRenderViewport(renderer, rect);
#else
	SDL_RenderSetViewport(renderer, rect);
#endif
}

RenderInterface_SDL::RenderInterface_SDL(SDL_Renderer* renderer) : renderer(renderer)
{
	// RmlUi serves vertex colors and textures with premultiplied alpha, set the blend mode accordingly.
	// Equivalent to glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
	blend_mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE,
		SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
}

void RenderInterface_SDL::BeginFrame()
{
	SetRenderViewport(renderer, nullptr);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawBlendMode(renderer, blend_mode);
}

void RenderInterface_SDL::EndFrame() {}

Rml::CompiledGeometryHandle RenderInterface_SDL::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	GeometryView* data = new GeometryView{vertices, indices};
	return reinterpret_cast<Rml::CompiledGeometryHandle>(data);
}

void RenderInterface_SDL::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	delete reinterpret_cast<GeometryView*>(geometry);
}

void RenderInterface_SDL::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	const GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);
	const Rml::Vertex* vertices = geometry->vertices.data();
	const size_t num_vertices = geometry->vertices.size();
	const int* indices = geometry->indices.data();
	const size_t num_indices = geometry->indices.size();

	Rml::UniquePtr<SDL_Vertex[]> sdl_vertices{new SDL_Vertex[num_vertices]};

	for (size_t i = 0; i < num_vertices; i++)
	{
		sdl_vertices[i].position = {vertices[i].position.x + translation.x, vertices[i].position.y + translation.y};
		sdl_vertices[i].tex_coord = {vertices[i].tex_coord.x, vertices[i].tex_coord.y};

		const auto& color = vertices[i].colour;
#if SDL_MAJOR_VERSION >= 3
		sdl_vertices[i].color = {color.red / 255.f, color.green / 255.f, color.blue / 255.f, color.alpha / 255.f};
#else
		sdl_vertices[i].color = {color.red, color.green, color.blue, color.alpha};
#endif
	}

	SDL_Texture* sdl_texture = (SDL_Texture*)texture;

	SDL_RenderGeometry(renderer, sdl_texture, sdl_vertices.get(), (int)num_vertices, indices, (int)num_indices);
}

void RenderInterface_SDL::EnableScissorRegion(bool enable)
{
	if (enable)
		SetRenderClipRect(renderer, &rect_scissor);
	else
		SetRenderClipRect(renderer, nullptr);

	scissor_region_enabled = enable;
}

void RenderInterface_SDL::SetScissorRegion(Rml::Rectanglei region)
{
	rect_scissor.x = region.Left();
	rect_scissor.y = region.Top();
	rect_scissor.w = region.Width();
	rect_scissor.h = region.Height();

	if (scissor_region_enabled)
		SetRenderClipRect(renderer, &rect_scissor);
}

Rml::TextureHandle RenderInterface_SDL::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
		return {};

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	using Rml::byte;
	Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	const size_t i_ext = source.rfind('.');
	Rml::String extension = (i_ext == Rml::String::npos ? Rml::String() : source.substr(i_ext + 1));

#if SDL_MAJOR_VERSION >= 3
	auto CreateSurface = [&]() { return IMG_LoadTyped_IO(SDL_IOFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str()); };
	auto GetSurfaceFormat = [](SDL_Surface* surface) { return surface->format; };
	auto ConvertSurface = [](SDL_Surface* surface, SDL_PixelFormat format) { return SDL_ConvertSurface(surface, format); };
	auto DestroySurface = [](SDL_Surface* surface) { SDL_DestroySurface(surface); };
#else
	auto CreateSurface = [&]() { return IMG_LoadTyped_RW(SDL_RWFromMem(buffer.get(), int(buffer_size)), 1, extension.c_str()); };
	auto GetSurfaceFormat = [](SDL_Surface* surface) { return surface->format->format; };
	auto ConvertSurface = [](SDL_Surface* surface, Uint32 format) { return SDL_ConvertSurfaceFormat(surface, format, 0); };
	auto DestroySurface = [](SDL_Surface* surface) { SDL_FreeSurface(surface); };
#endif

	SDL_Surface* surface = CreateSurface();
	if (!surface)
		return {};

	texture_dimensions = {surface->w, surface->h};

	if (GetSurfaceFormat(surface) != SDL_PIXELFORMAT_RGBA32 && GetSurfaceFormat(surface) != SDL_PIXELFORMAT_BGRA32)
	{
		SDL_Surface* converted_surface = ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		DestroySurface(surface);
		if (!converted_surface)
			return {};

		surface = converted_surface;
	}

	// Convert colors to premultiplied alpha, which is necessary for correct alpha compositing.
	const size_t pixels_byte_size = surface->w * surface->h * 4;
	byte* pixels = static_cast<byte*>(surface->pixels);
	for (size_t i = 0; i < pixels_byte_size; i += 4)
	{
		const byte alpha = pixels[i + 3];
		for (size_t j = 0; j < 3; ++j)
			pixels[i + j] = byte(int(pixels[i + j]) * int(alpha) / 255);
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	texture_dimensions = Rml::Vector2i(surface->w, surface->h);
	DestroySurface(surface);

	if (texture)
		SDL_SetTextureBlendMode(texture, blend_mode);

	return (Rml::TextureHandle)texture;
}

Rml::TextureHandle RenderInterface_SDL::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	RMLUI_ASSERT(source.data() && source.size() == size_t(source_dimensions.x * source_dimensions.y * 4));

#if SDL_MAJOR_VERSION >= 3
	auto CreateSurface = [&]() {
		return SDL_CreateSurfaceFrom(source_dimensions.x, source_dimensions.y, SDL_PIXELFORMAT_RGBA32, (void*)source.data(), source_dimensions.x * 4);
	};
	auto DestroySurface = [](SDL_Surface* surface) { SDL_DestroySurface(surface); };
#else
	auto CreateSurface = [&]() {
		return SDL_CreateRGBSurfaceWithFormatFrom((void*)source.data(), source_dimensions.x, source_dimensions.y, 32, source_dimensions.x * 4,
			SDL_PIXELFORMAT_RGBA32);
	};
	auto DestroySurface = [](SDL_Surface* surface) { SDL_FreeSurface(surface); };
#endif

	SDL_Surface* surface = CreateSurface();

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_SetTextureBlendMode(texture, blend_mode);

	DestroySurface(surface);
	return (Rml::TextureHandle)texture;
}

void RenderInterface_SDL::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	SDL_DestroyTexture((SDL_Texture*)texture_handle);
}
