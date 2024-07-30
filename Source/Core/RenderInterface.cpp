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

#include "../../Include/RmlUi/Core/RenderInterface.h"

namespace Rml {

namespace CoreInternal {
	bool HasRenderManager(RenderInterface* render_interface);
}

RenderInterface::RenderInterface() {}

RenderInterface::~RenderInterface()
{
	// Note: We cannot automatically release render resources here, because that involves a virtual call to this interface during its destruction
	// which is illegal.
	RMLUI_ASSERTMSG(!CoreInternal::HasRenderManager(this),
		"RenderInterface is being destroyed, but it is still actively referenced and used within the RmlUi library. This may lead to use-after-free "
		"or nullptr dereference when releasing render resources. Ensure that the render interface is destroyed *after* the call to Rml::Shutdown.");
}

void RenderInterface::EnableClipMask(bool /*enable*/) {}

void RenderInterface::RenderToClipMask(ClipMaskOperation /*operation*/, CompiledGeometryHandle /*geometry*/, Vector2f /*translation*/) {}

void RenderInterface::SetTransform(const Matrix4f* /*transform*/) {}

LayerHandle RenderInterface::PushLayer()
{
	return {};
}

void RenderInterface::CompositeLayers(LayerHandle /*source*/, LayerHandle /*destination*/, BlendMode /*blend_mode*/,
	Span<const CompiledFilterHandle> /*filters*/)
{}

void RenderInterface::PopLayer() {}

TextureHandle RenderInterface::SaveLayerAsTexture()
{
	return TextureHandle{};
}

CompiledFilterHandle RenderInterface::SaveLayerAsMaskImage()
{
	return CompiledFilterHandle{};
}

CompiledFilterHandle RenderInterface::CompileFilter(const String& /*name*/, const Dictionary& /*parameters*/)
{
	return CompiledFilterHandle{};
}

void RenderInterface::ReleaseFilter(CompiledFilterHandle /*filter*/) {}

CompiledShaderHandle RenderInterface::CompileShader(const String& /*name*/, const Dictionary& /*parameters*/)
{
	return CompiledShaderHandle{};
}

void RenderInterface::RenderShader(CompiledShaderHandle /*shader*/, CompiledGeometryHandle /*geometry*/, Vector2f /*translation*/,
	TextureHandle /*texture*/)
{}

void RenderInterface::ReleaseShader(CompiledShaderHandle /*shader*/) {}

} // namespace Rml
