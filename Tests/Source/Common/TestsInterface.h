/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#include <Shell.h>
#include <RmlUi/Core/RenderInterface.h>


class TestsSystemInterface : public ShellSystemInterface
{
public:
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

	// Checks and clears previously logged messages, then sets the number of expected
	// warnings and errors until the next call.
	void SetNumExpectedWarnings(int num_expected_warnings);

private:
	int num_logged_warnings = 0;
	int num_expected_warnings = 0;

	Rml::StringList warnings;
};


class TestsRenderInterface : public Rml::RenderInterface
{
public:
	struct Counters {
		size_t render_calls;
		size_t enable_scissor;
		size_t set_scissor;
		size_t load_texture;
		size_t generate_texture;
		size_t release_texture;
		size_t set_transform;
	};

	void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(int x, int y, int width, int height) override;

	bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	const Counters& GetCounters() const {
		return counters;
	}

	void ResetCounters() {
		counters = {};
	}

private:
	Counters counters = {};
};
#endif
