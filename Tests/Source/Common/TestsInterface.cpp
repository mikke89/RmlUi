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

#include "TestsInterface.h"
#include "TypesToString.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/StringUtilities.h>
#include <doctest.h>

TestsSystemInterface::~TestsSystemInterface()
{
	SetNumExpectedWarnings(0);
}

double TestsSystemInterface::GetElapsedTime()
{
	return elapsed_time;
}

bool TestsSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
	static const char* message_type_str[Rml::Log::Type::LT_MAX] = {"Always", "Error", "Assert", "Warning", "Info", "Debug"};
	const bool result = Rml::SystemInterface::LogMessage(type, message);

	if (type <= Rml::Log::Type::LT_WARNING)
	{
		const Rml::String warning = "RmlUi " + Rml::String(message_type_str[type]) + ": " + message;

		if (num_expected_warnings > 0)
		{
			num_logged_warnings += 1;
			warnings.push_back(warning);
		}
		else
		{
			FAIL_CHECK(warning);
		}
	}

	return result;
}

void TestsSystemInterface::SetNumExpectedWarnings(int in_num_expected_warnings)
{
	if (num_expected_warnings > 0)
	{
		// Check and clear previous warnings
		if (num_logged_warnings != num_expected_warnings)
		{
			Rml::String str = "Got unexpected number of warnings: \n";
			Rml::StringUtilities::JoinString(str, warnings, '\n');
			if (warnings.empty())
				str += "(no warnings logged)";
			CHECK_MESSAGE(num_logged_warnings == num_expected_warnings, str);
		}

		num_expected_warnings = 0;
		num_logged_warnings = 0;
		warnings.clear();
	}
	num_expected_warnings = in_num_expected_warnings;
}

void TestsSystemInterface::SetTime(double t)
{
	elapsed_time = t;
}

Rml::CompiledGeometryHandle TestsRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	counters.compile_geometry += 1;

	if (meshes_set)
	{
		INFO("Got vertices:\n", vertices);
		INFO("Got indices:\n", indices);
		REQUIRE_MESSAGE(!meshes.empty(), "No CompileGeometry expected, but one was passed to us");

		Rml::Mesh mesh = std::move(meshes.front());
		meshes.erase(meshes.begin());
		INFO("Expected mesh:\n", mesh);

		CHECK(mesh.vertices.size() == vertices.size());
		CHECK(mesh.indices.size() == indices.size());

		for (size_t i = 0; i < mesh.vertices.size(); i++)
		{
			CHECK(mesh.vertices[i].position == vertices[i].position);
			CHECK(mesh.vertices[i].colour == vertices[i].colour);
			CHECK(mesh.vertices[i].tex_coord == vertices[i].tex_coord);
		}

		for (size_t i = 0; i < mesh.indices.size(); i++)
		{
			CHECK(mesh.indices[i] == indices[i]);
		}
	}

	return Rml::CompiledGeometryHandle(counters.compile_geometry);
}

void TestsRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle /*geometry*/, Rml::Vector2f /*translation*/, Rml::TextureHandle /*texture*/)
{
	counters.render_geometry += 1;
}

void TestsRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle /*geometry*/)
{
	counters.release_geometry += 1;
}

void TestsRenderInterface::EnableScissorRegion(bool /*enable*/)
{
	counters.enable_scissor += 1;
}

void TestsRenderInterface::SetScissorRegion(Rml::Rectanglei /*region*/)
{
	counters.set_scissor += 1;
}

void TestsRenderInterface::EnableClipMask(bool /*enable*/)
{
	counters.enable_clip_mask += 1;
}

void TestsRenderInterface::RenderToClipMask(Rml::ClipMaskOperation /*mask_operation*/, Rml::CompiledGeometryHandle /*geometry*/,
	Rml::Vector2f /*translation*/)
{
	counters.render_to_clip_mask += 1;
}

Rml::TextureHandle TestsRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& /*source*/)
{
	counters.load_texture += 1;
	texture_dimensions.x = 512;
	texture_dimensions.y = 256;
	return 1;
}

Rml::TextureHandle TestsRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> /*source*/, Rml::Vector2i /*source_dimensions*/)
{
	counters.generate_texture += 1;
	return 1;
}

void TestsRenderInterface::ReleaseTexture(Rml::TextureHandle /*texture_handle*/)
{
	counters.release_texture += 1;
}

void TestsRenderInterface::SetTransform(const Rml::Matrix4f* /*transform*/)
{
	counters.set_transform += 1;
}

Rml::CompiledFilterHandle TestsRenderInterface::CompileFilter(const Rml::String& /*name*/, const Rml::Dictionary& /*parameters*/)
{
	counters.compile_filter += 1;
	return 1;
}

void TestsRenderInterface::ReleaseFilter(Rml::CompiledFilterHandle /*filter*/)
{
	counters.release_filter += 1;
}

Rml::CompiledShaderHandle TestsRenderInterface::CompileShader(const Rml::String& /*name*/, const Rml::Dictionary& /*parameters*/)
{
	counters.compile_shader += 1;
	return 1;
}

void TestsRenderInterface::RenderShader(Rml::CompiledShaderHandle /*shader*/, Rml::CompiledGeometryHandle /*geometry*/, Rml::Vector2f /*translation*/,
	Rml::TextureHandle /*texture*/)
{
	counters.render_shader += 1;
}

void TestsRenderInterface::ReleaseShader(Rml::CompiledShaderHandle /*shader*/)
{
	counters.release_shader += 1;
}
void TestsRenderInterface::ResetCounters()
{
	counters_from_previous_reset = std::exchange(counters, Counters());
}

void TestsRenderInterface::ExpectCompileGeometry(Rml::Vector<Rml::Mesh> in_meshes)
{
	VerifyMeshes();
	meshes = std::move(in_meshes);
	meshes_set = true;
}

void TestsRenderInterface::Reset()
{
	VerifyMeshes();
	meshes_set = false;
	ResetCounters();
}
void TestsRenderInterface::VerifyMeshes()
{
	if (!meshes.empty())
	{
		// Use FAIL instead of REQUIRE here as this may be executed outside the context of a doctest test case.
		FAIL("CompileGeometry: Expected meshes not passed to us");
	}
}
