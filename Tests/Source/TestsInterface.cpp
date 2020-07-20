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

#include "TestsInterface.h"
#include <doctest.h>
#include <chrono>

double TestsSystemInterface::GetElapsedTime()
{
	static const auto start_time = std::chrono::steady_clock::now();

	const auto now = std::chrono::steady_clock::now();
	const double t = std::chrono::duration<double>(now - start_time).count();

	return t;
}

bool TestsSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
	bool result = Rml::SystemInterface::LogMessage(type, message);
	CHECK_MESSAGE(type >= Rml::Log::Type::LT_INFO, "RmlUi logged: " << message);
	return result;
}


void TestsRenderInterface::RenderGeometry(Rml::Vertex* /*vertices*/, int /*num_vertices*/, int* /*indices*/, int /*num_indices*/, const Rml::TextureHandle /*texture*/, const Rml::Vector2f& /*translation*/)
{
}

void TestsRenderInterface::EnableScissorRegion(bool /*enable*/)
{
}

void TestsRenderInterface::SetScissorRegion(int /*x*/, int /*y*/, int /*width*/, int /*height*/)
{
}

bool TestsRenderInterface::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& /*source*/)
{
	texture_handle = 1;
	texture_dimensions.x = 512;
	texture_dimensions.y = 256;
	return true;
}

bool TestsRenderInterface::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* /*source*/, const Rml::Vector2i& /*source_dimensions*/)
{
	texture_handle = 1;
	return true;
}

void TestsRenderInterface::ReleaseTexture(Rml::TextureHandle /*texture_handle*/)
{
}

void TestsRenderInterface::SetTransform(const Rml::Matrix4f* /*transform*/)
{
}
