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

#ifndef RMLUI_TESTS_TESTSINTERFACE_H
#define RMLUI_TESTS_TESTSINTERFACE_H

#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <Shell.h>

class TestsSystemInterface : public Rml::SystemInterface {
public:
	~TestsSystemInterface();

	double GetElapsedTime() override;

	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

	// Checks and clears previously logged messages, then sets the number of expected
	// warnings and errors until the next call.
	void SetNumExpectedWarnings(int num_expected_warnings);

	void SetTime(double t);

private:
	double elapsed_time = 0.0;

	int num_logged_warnings = 0;
	int num_expected_warnings = 0;

	Rml::StringList warnings;
};

class TestsRenderInterface : public Rml::RenderInterface {
public:
	struct Counters {
		size_t compile_geometry;
		size_t render_geometry;
		size_t release_geometry;
		size_t load_texture;
		size_t generate_texture;
		size_t release_texture;
		size_t enable_scissor;
		size_t set_scissor;
		size_t enable_clip_mask;
		size_t render_to_clip_mask;
		size_t set_transform;
		size_t compile_filter;
		size_t release_filter;
		size_t compile_shader;
		size_t render_shader;
		size_t release_shader;
	};

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle handle) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void EnableClipMask(bool enable) override;
	void RenderToClipMask(Rml::ClipMaskOperation mask_operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void ReleaseFilter(Rml::CompiledFilterHandle filter) override;

	Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
		Rml::TextureHandle texture) override;
	void ReleaseShader(Rml::CompiledShaderHandle shader) override;

	const Counters& GetCounters() const { return counters; }
	void ResetCounters();
	const Counters& GetCountersFromPreviousReset() const { return counters_from_previous_reset; }

	void ExpectCompileGeometry(Rml::Vector<Rml::Mesh> meshes);

	void Reset();

private:
	void VerifyMeshes();

	Counters counters = {};
	Counters counters_from_previous_reset = {};
	Rml::Vector<Rml::Mesh> meshes;
	bool meshes_set = false;
};

#endif
