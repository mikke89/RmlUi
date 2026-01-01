#pragma once

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
