#pragma once

#include "../../Include/RmlUi/Core/Debug.h"
#include "../../Include/RmlUi/Core/Traits.h"

namespace Rml {

template <typename T>
class ControlledLifetimeResource : NonCopyMoveable {
public:
	ControlledLifetimeResource() = default;
	~ControlledLifetimeResource() noexcept
	{
#if defined(RMLUI_PLATFORM_WIN32) && !defined(RMLUI_STATIC_LIB)
		RMLUI_ASSERTMSG(!pointer || intentionally_leaked, "Resource was not properly shut down.");
#endif
	}

	explicit operator bool() const noexcept { return pointer != nullptr; }

	void Initialize()
	{
		RMLUI_ASSERTMSG(!pointer, "Resource already initialized.");
		pointer = new T();
	}

	void InitializeIfEmpty()
	{
		if (!pointer)
			Initialize();
		else
			SetIntentionallyLeaked(false);
	}

	void Leak() { SetIntentionallyLeaked(true); }

	void Shutdown()
	{
		RMLUI_ASSERTMSG(pointer, "Shutting down resource that was not initialized, or has been shut down already.");
		RMLUI_ASSERTMSG(!intentionally_leaked, "Shutting down resource that was marked as leaked.");
		delete pointer;
		pointer = nullptr;
	}

	T* operator->()
	{
		RMLUI_ASSERTMSG(pointer, "Resource used before it was initialized, or after it was shut down.");
		return pointer;
	}

private:
#ifdef RMLUI_DEBUG
	void SetIntentionallyLeaked(bool leaked) { intentionally_leaked = leaked; }
	bool intentionally_leaked = false;
#else
	void SetIntentionallyLeaked(bool /*leaked*/) {}
#endif

	T* pointer = nullptr;
};

} // namespace Rml
