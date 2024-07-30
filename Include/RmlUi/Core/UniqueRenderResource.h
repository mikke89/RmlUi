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

#ifndef RMLUI_CORE_UNIQUERENDERRESOURCE_H
#define RMLUI_CORE_UNIQUERENDERRESOURCE_H

#include "Types.h"
#include <utility>

namespace Rml {

class RenderManager;

/**
    Abstraction for a uniquely owned render resource. The underlying resource is released upon destruction.
 */
template <typename Derived, typename Handle, Handle InvalidHandleValue>
class RMLUICORE_API UniqueRenderResource {
public:
	static constexpr Handle InvalidHandle() { return InvalidHandleValue; }

	explicit operator bool() const { return resource_handle != InvalidHandleValue; }

protected:
	UniqueRenderResource() = default;
	UniqueRenderResource(RenderManager* render_manager, Handle resource_handle) : render_manager(render_manager), resource_handle(resource_handle) {}

	~UniqueRenderResource() noexcept { ReleaseInDerived(); }

	UniqueRenderResource(const UniqueRenderResource&) = delete;
	UniqueRenderResource& operator=(const UniqueRenderResource&) = delete;

	UniqueRenderResource(UniqueRenderResource&& other) noexcept { MoveFrom(other); }
	UniqueRenderResource& operator=(UniqueRenderResource&& other) noexcept
	{
		ReleaseInDerived();
		MoveFrom(other);
		return *this;
	}

	void Clear() noexcept
	{
		render_manager = nullptr;
		resource_handle = InvalidHandleValue;
	}

	RenderManager* render_manager = nullptr;
	Handle resource_handle = InvalidHandleValue;

private:
	void ReleaseInDerived() { static_cast<Derived*>(this)->Release(); }

	void MoveFrom(UniqueRenderResource& other) noexcept
	{
		render_manager = std::exchange(other.render_manager, nullptr);
		resource_handle = std::exchange(other.resource_handle, InvalidHandleValue);
	}
};

} // namespace Rml

#endif
