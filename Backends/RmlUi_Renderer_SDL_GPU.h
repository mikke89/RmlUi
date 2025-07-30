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

#ifndef RMLUI_BACKENDS_RENDERER_SDL_GPU_H
#define RMLUI_BACKENDS_RENDERER_SDL_GPU_H

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL3/SDL.h>

class RenderInterface_SDL_GPU : public Rml::RenderInterface {
public:
	RenderInterface_SDL_GPU(SDL_GPUDevice* device, SDL_Window* window);
	void Shutdown();
	void BeginFrame(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture, uint32_t width, uint32_t height);
	void EndFrame();
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;
	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;
	void SetTransform(const Rml::Matrix4f* new_transform) override;

private:
	SDL_GPUDevice* device;
	SDL_Window* window;
	SDL_GPUGraphicsPipeline* texture_pipeline;
	SDL_GPUGraphicsPipeline* color_pipeline;
	SDL_GPUSampler* linear_sampler;
	SDL_GPUCommandBuffer* command_buffer;
	SDL_GPUTexture* swapchain_texture;
	uint32_t swapchain_width;
	uint32_t swapchain_height;
	SDL_GPURenderPass* render_pass;
	SDL_GPUCopyPass* copy_pass;
	SDL_Rect scissor;
	Rml::Matrix4f transform;
	Rml::Matrix4f proj;

	struct Command {
		virtual void Update(RenderInterface_SDL_GPU& interface) = 0;
	};

	struct EnableScissorRegionCommand : Command {
		EnableScissorRegionCommand(bool enable) : enable(enable) {}
		void Update(RenderInterface_SDL_GPU& interface) override;

		bool enable;
	};

	struct SetScissorRegionCommand : Command {
		SetScissorRegionCommand(Rml::Rectanglei region) : region(region) {}
		void Update(RenderInterface_SDL_GPU& interface) override;

		Rml::Rectanglei region;
	};

	struct RenderGeometryCommand : Command {
		RenderGeometryCommand(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) :
			handle(handle), translation(translation), texture(texture)
		{}
		void Update(RenderInterface_SDL_GPU& interface) override;

		Rml::CompiledGeometryHandle handle;
		Rml::Vector2f translation;
		Rml::TextureHandle texture;
	};

	struct ReleaseGeometryCommand : Command {
		ReleaseGeometryCommand(Rml::CompiledGeometryHandle handle) : handle(handle) {}
		void Update(RenderInterface_SDL_GPU& interface) override;

		Rml::CompiledGeometryHandle handle;
	};

	struct ReleaseTextureCommand : Command {
		ReleaseTextureCommand(Rml::TextureHandle handle) : handle(handle) {}
		void Update(RenderInterface_SDL_GPU& interface) override;

		Rml::TextureHandle handle;
	};

	struct SetTransformCommand : Command {
		SetTransformCommand(const Rml::Matrix4f* new_transform);
		void Update(RenderInterface_SDL_GPU& interface) override;

		Rml::Matrix4f transform;
		bool has_transform;
	};

	friend struct EnableScissorRegionCommand;
	friend struct SetScissorRegionCommand;
	friend struct RenderGeometryCommand;
	friend struct ReleaseGeometryCommand;
	friend struct ReleaseTextureCommand;
	friend struct SetTransformCommand;

	struct Buffer {
		SDL_GPUTransferBuffer* transfer_buffer;
		SDL_GPUBuffer* buffer;
		SDL_GPUBufferUsageFlags usage;
		int capacity;
		bool in_use;
	};

	struct GeometryView {
		Buffer* vertex_buffer;
		Buffer* index_buffer;
		int num_indices;
	};

	// List of ordered render commands
	Rml::Vector<Rml::UniquePtr<Command>> commands;

	// Sorted vertex/index buffers by capacities
	Rml::Vector<Rml::UniquePtr<Buffer>> buffers;

	void CreatePipelines();
	bool BeginCopyPass();
	bool BeginRenderPass();
	Buffer* RequestBuffer(int capacity, SDL_GPUBufferUsageFlags usage);
};

#endif
