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

#ifndef RMLUI_CORE_COMPILEDFILTERSHADER_H
#define RMLUI_CORE_COMPILEDFILTERSHADER_H

#include "Header.h"
#include "UniqueRenderResource.h"

namespace Rml {

class RenderManager;

/**
    A compiled filter to be applied when compositing layers in the render manager.

    Represents a unique render resource constructed through the render manager.
 */
class RMLUICORE_API CompiledFilter final : public UniqueRenderResource<CompiledFilter, CompiledFilterHandle, CompiledFilterHandle(0)> {
public:
	CompiledFilter() = default;

	void AddHandleTo(FilterHandleList& list);

	void Release();

private:
	CompiledFilter(RenderManager* render_manager, CompiledFilterHandle resource_handle) : UniqueRenderResource(render_manager, resource_handle) {}
	friend class RenderManager;
};

/**
    A compiled shader to be used when rendering geometry.

    Represents a unique render resource constructed through the render manager.
 */
class RMLUICORE_API CompiledShader final : public UniqueRenderResource<CompiledShader, CompiledShaderHandle, CompiledShaderHandle(0)> {
public:
	CompiledShader() = default;

	void Release();

private:
	CompiledShader(RenderManager* render_manager, CompiledShaderHandle resource_handle) : UniqueRenderResource(render_manager, resource_handle) {}
	friend class RenderManager;
};

} // namespace Rml

#endif
